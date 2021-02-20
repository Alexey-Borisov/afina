#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
    if (Set(key, value)) { return true; }
    return add_node(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
    if (_lru_index.find(key) != _lru_index.end()) { return false; }
    return add_node(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    node_map::iterator node_iter = _lru_index.find(key);
    if (node_iter == _lru_index.end()) { return false; }
    lru_node &node = node_iter->second.get();
    if (node.key.size() + value.size() > _max_size) { return false; }
    while (_cur_size - node.value.size() + value.size() > _max_size) {
        Delete(_lru_tail->next->key);
    }
    _cur_size = _cur_size - node.value.size() + value.size();
    node.value = value;
    push_node(node_iter);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { 
    node_map::iterator node_iter = _lru_index.find(key);
    if (node_iter == _lru_index.end()) { return false; }
    lru_node &delete_node = node_iter->second.get();
    _lru_index.erase(node_iter);
    _cur_size -= delete_node.key.size() + delete_node.value.size();
    delete_node.prev->next = delete_node.next;
    delete_node.next->prev.swap(delete_node.prev);
    delete_node.prev.reset();
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { 
    node_map::iterator node_iter = _lru_index.find(key);
    if (node_iter == _lru_index.end()) { return false; }
    value = node_iter->second.get().value; 
    push_node(node_iter);
    return true;
}

bool SimpleLRU::add_node(const std::string &key, const std::string &value) {
    std::size_t node_size = key.size() + value.size();
    if (node_size > _max_size) { return false; }
    while (_cur_size + node_size > _max_size) {
        Delete(_lru_tail->next->key);
    }
    auto new_node = std::unique_ptr<lru_node>(new lru_node{key, value, _lru_head.get()});
    new_node->prev = std::move(_lru_head->prev);
    new_node->prev->next = new_node.get();
    _lru_head->prev = std::move(new_node);
    _cur_size += key.size() + value.size();
    _lru_index.emplace(std::make_pair(std::reference_wrapper<const std::string>(_lru_head->prev->key), 
                                    std::reference_wrapper<lru_node>(*_lru_head->prev)));
    return true;
}


void SimpleLRU::push_node(const node_map::iterator &node_iterator) {
    lru_node &node = node_iterator->second.get();   
    node.prev->next = node.next;
    node.prev.swap(node.next->prev);
    node.prev.swap(_lru_head->prev);
    node.next = _lru_head.get();
    node.prev->next = &node;
}

} // namespace Backend
} // namespace Afina
