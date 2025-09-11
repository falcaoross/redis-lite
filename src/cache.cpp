#include "cache.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

LRUCache::LRUCache(size_t capacity, const std::string &aof_path)
    : capacity_(capacity), aof_path_(aof_path), loading_(true)
{
    // ensure parent directories exist
    try
    {
        std::filesystem::path p(aof_path_);
        if (!p.parent_path().empty())
            std::filesystem::create_directories(p.parent_path());
    }
    catch (...)
    {
        // ignore filesystem errors for portability
    }

    loadAOF(); // replay any previous AOF (loading_ = true so we don't re-log)
    loading_ = false;
    // open AOF for append (binary)
    // open in append mode so writes always go to the end
    std::ofstream aof(aof_path_, std::ios::binary | std::ios::app);
    // ensure file exists; it will be reopened on append calls
    aof.close();
}

LRUCache::~LRUCache()
{
    // nothing special for now
}

size_t LRUCache::size() const noexcept { return map_.size(); }
size_t LRUCache::capacity() const noexcept { return capacity_; }

void LRUCache::set(const std::string &key, const std::string &value)
{
    set_internal(key, value, true);
}

bool LRUCache::get(const std::string &key, std::string &out_value)
{
    auto it = map_.find(key);
    if (it == map_.end())
        return false;
    // move node to front
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second.it);
    it->second.it = lru_list_.begin();
    out_value = it->second.value;
    return true;
}

bool LRUCache::del(const std::string &key)
{
    return (map_.find(key) != map_.end()) ? (del_internal(key, true), true) : false;
}

void LRUCache::saveAOF()
{
    // We already append on each op. To ensure flush we open/close and flush nothing special.
    // For safety, rewrite little no-op to ensure file exists
    std::ofstream out(aof_path_, std::ios::binary | std::ios::app);
    out.flush();
    out.close();
}

// internal helpers ----------------------------------------------------------------

void LRUCache::set_internal(const std::string &key, const std::string &value, bool append)
{
    auto it = map_.find(key);
    if (it != map_.end())
    {
        // update value & move to front
        it->second.value = value;
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second.it);
        it->second.it = lru_list_.begin();
    }
    else
    {
        // evict if necessary
        if (map_.size() >= capacity_)
        {
            std::string lru_key = lru_list_.back();
            lru_list_.pop_back();
            map_.erase(lru_key);
            if (!loading_ && append)
            {
                appendAOF_del(lru_key); // record eviction as delete in AOF
            }
        }
        lru_list_.push_front(key);
        map_.emplace(key, Entry{value, lru_list_.begin()});
    }

    if (!loading_ && append)
    {
        appendAOF_set(key, value);
    }
}

void LRUCache::del_internal(const std::string &key, bool append)
{
    auto it = map_.find(key);
    if (it == map_.end())
        return;
    lru_list_.erase(it->second.it);
    map_.erase(it);
    if (!loading_ && append)
    {
        appendAOF_del(key);
    }
}

// AOF append formats:
// SET: "S <keylen> <vallen>\n" <key bytes><value bytes>\n
// DEL: "D <keylen>\n" <key bytes>\n
void LRUCache::appendAOF_set(const std::string &key, const std::string &value)
{
    std::ofstream out(aof_path_, std::ios::binary | std::ios::app);
    if (!out)
        return;
    out << 'S' << ' ' << key.size() << ' ' << value.size() << '\n';
    out.write(key.data(), key.size());
    out.write(value.data(), value.size());
    out.put('\n');
    out.flush();
    out.close();
}

void LRUCache::appendAOF_del(const std::string &key)
{
    std::ofstream out(aof_path_, std::ios::binary | std::ios::app);
    if (!out)
        return;
    out << 'D' << ' ' << key.size() << '\n';
    out.write(key.data(), key.size());
    out.put('\n');
    out.flush();
    out.close();
}

void LRUCache::loadAOF()
{
    std::ifstream in(aof_path_, std::ios::binary);
    if (!in)
        return; // no file yet

    std::string header;
    while (std::getline(in, header))
    {
        if (header.empty())
            continue;
        std::istringstream hs(header);
        char op;
        hs >> op;
        if (op == 'S')
        {
            size_t klen = 0, vlen = 0;
            hs >> klen >> vlen;
            if (klen == 0 && vlen == 0)
                continue;
            std::string key;
            key.resize(klen);
            std::string val;
            val.resize(vlen);
            in.read(&key[0], static_cast<std::streamsize>(klen));
            in.read(&val[0], static_cast<std::streamsize>(vlen));
            // consume trailing newline if present
            if (in.peek() == '\n')
                in.get();
            // set without appending
            set_internal(key, val, false);
        }
        else if (op == 'D')
        {
            size_t klen = 0;
            hs >> klen;
            if (klen == 0)
                continue;
            std::string key;
            key.resize(klen);
            in.read(&key[0], static_cast<std::streamsize>(klen));
            if (in.peek() == '\n')
                in.get();
            del_internal(key, false);
        }
        else
        {
            // unknown op -> skip line
            continue;
        }
    }
    in.close();
}
