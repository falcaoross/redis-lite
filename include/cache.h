#pragma once
#include <string>
#include <unordered_map>
#include <list>

class LRUCache
{
public:
    LRUCache(size_t capacity, const std::string &aof_path = "data/aof.log");
    ~LRUCache();

    // Public API
    void set(const std::string &key, const std::string &value);
    bool get(const std::string &key, std::string &out_value);
    bool del(const std::string &key);

    size_t size() const noexcept;
    size_t capacity() const noexcept;

    // Force a snapshot (writes nothing fancy—just flushes AOF)
    void saveAOF();

private:
    struct Entry
    {
        std::string value;
        std::list<std::string>::iterator it;
    };

    size_t capacity_;
    std::list<std::string> lru_list_; // front = most recent
    std::unordered_map<std::string, Entry> map_;

    std::string aof_path_;
    bool loading_;

    // persistence helpers
    void loadAOF();
    void appendAOF_set(const std::string &key, const std::string &value);
    void appendAOF_del(const std::string &key);

    // internals: append flag decides whether to log to AOF
    void set_internal(const std::string &key, const std::string &value, bool append);
    void del_internal(const std::string &key, bool append);
};
