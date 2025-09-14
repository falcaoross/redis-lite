#pragma once
#include <string>
#include <unordered_map>
#include <list>

class LRUCache
{
public:
    LRUCache(size_t capacity,
             const std::string &snapshot_path = "data/snapshot.rdb",
             const std::string &aof_path = "data/aof.log",
             const std::string &aes_key = "1234567890123456"); // 16 bytes for AES-128

    ~LRUCache();

    // Public API
    void set(const std::string &key, const std::string &value);
    bool get(const std::string &key, std::string &out_value);
    bool del(const std::string &key);

    size_t size() const noexcept;
    size_t capacity() const noexcept;

    // Persistence
    void saveSnapshot();
    void flushAOF();

private:
    struct Entry
    {
        std::string value;
        std::list<std::string>::iterator it;
    };

    size_t capacity_;
    std::list<std::string> lru_list_;
    std::unordered_map<std::string, Entry> map_;

    std::string snapshot_path_;
    std::string aof_path_;
    std::string aes_key_;
    bool loading_;

    // Internals
    void set_internal(const std::string &key, const std::string &value, bool append);
    void del_internal(const std::string &key, bool append);

    // Persistence helpers
    void loadSnapshot();
    void loadAOF();
    void appendAOF_set(const std::string &key, const std::string &value);
    void appendAOF_del(const std::string &key);

    // AES helpers
    std::string aes_encrypt(const std::string &plaintext);
    std::string aes_decrypt(const std::string &ciphertext);
};
