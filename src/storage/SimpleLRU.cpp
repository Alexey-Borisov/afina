#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
    std::size_t node_size = key.size() + value.size();
    if (node_size > _max_size) {
        return false;
    }
    while (_cur_size + node_size > _max_size) {
        Delete(_lru_tail->next->key);
    }
    if (!Set(key, value)) {
        add_head_node(key, value);
    }
    return true; 
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) { 
    node_map::iterator node_iterator = _lru_index.find(key);
    if (node_iterator != _lru_index.end()) {
        return false;
    } 
    return Put(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    node_map::iterator node_iterator = _lru_index.find(key);
    if (node_iterator == _lru_index.end()) {
        return false;
    }
    node_iterator->second.get().value = value;
    push_node(node_iterator);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { 
    node_map::iterator node_iterator = _lru_index.find(key);
    if (node_iterator == _lru_index.end()) {
        return false;
    }
    lru_node &delete_node = node_iterator->second.get();
    _lru_index.erase(node_iterator);
    _cur_size -= delete_node.key.size() + delete_node.value.size();
    delete_node.prev->next = delete_node.next;
    delete_node.next->prev.swap(delete_node.prev);
    delete_node.prev.reset();
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { 
    node_map::iterator node_iterator = _lru_index.find(key);
    if (node_iterator == _lru_index.end()) {
        return false;
    } 
    value = node_iterator->second.get().value;
    push_node(node_iterator);
    return true;
}

void SimpleLRU::add_head_node(const std::string &key, const std::string &value) {
    std::unique_ptr<lru_node> new_node(new lru_node);
    new_node->key = std::move(key);
    new_node->value = std::move(value);
    _lru_head->prev->next = new_node.get();
    new_node->next = _lru_head.get();
    new_node->prev = std::move(_lru_head->prev);
    _lru_head->prev = std::move(new_node);
    _cur_size += key.size() + value.size();
    _lru_index.emplace(std::make_pair(std::reference_wrapper<std::string>(_lru_head->prev->key), 
                                    std::reference_wrapper<lru_node>(*_lru_head->prev)));
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
