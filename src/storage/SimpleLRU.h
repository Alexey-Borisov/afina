#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size) {
        _lru_head = std::unique_ptr<lru_node>(new lru_node);
        _lru_head->prev = std::unique_ptr<lru_node>(new lru_node);
        _lru_tail = _lru_head->prev.get();
        _lru_tail->next = _lru_head.get();
    }
    ~SimpleLRU() {
        _lru_index.clear();
        while(_lru_tail != _lru_head.get()) {
            _lru_tail = _lru_tail->next;
            _lru_tail->prev.reset();
        }
        _lru_head.reset();
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        std::string key;
        std::string value;
        lru_node *next{};
        std::unique_ptr<lru_node> prev;
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be not greater than the _max_size
    std::size_t _max_size{};
    std::size_t _cur_size{};
    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node *_lru_tail{};

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    using node_map = std::map<std::reference_wrapper<const std::string>, 
                                std::reference_wrapper<lru_node>, std::less<std::string>>;
    node_map _lru_index;

    bool add_node(const std::string &key, const std::string &value);
    void delete_iter(const node_map::iterator &node_iterator);
    void get_iter(const node_map::iterator &node_iterator, std::string &value);
    void set_iter(const node_map::iterator &node_iterator, const std::string &value);
    bool contains(const std::string &key, node_map::iterator &node_iterator);
    bool contains(const std::string &key);
    void push_node(const node_map::iterator &node_iterator); 
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
