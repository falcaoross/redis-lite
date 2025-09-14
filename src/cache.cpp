#include "cache.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

LRUCache::LRUCache(size_t capacity,
                   const std::string &snapshot_path,
                   const std::string &aof_path,
                   const std::string &aes_key)
    : capacity_(capacity), snapshot_path_(snapshot_path),
      aof_path_(aof_path), aes_key_(aes_key), loading_(true)
{
    std::filesystem::create_directories("data");
    loadSnapshot();
    loadAOF();
    loading_ = false;
}

LRUCache::~LRUCache()
{
    saveSnapshot();
}

// -------------------- Public API --------------------
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

    lru_list_.splice(lru_list_.begin(), lru_list_, it->second.it);
    it->second.it = lru_list_.begin();
    out_value = it->second.value;
    return true;
}

bool LRUCache::del(const std::string &key)
{
    return (map_.find(key) != map_.end()) ? (del_internal(key, true), true) : false;
}

void LRUCache::saveSnapshot()
{
    std::ofstream out(snapshot_path_, std::ios::binary);
    for (auto &kv : map_)
    {
        std::string enc_key = aes_encrypt(kv.first);
        std::string enc_val = aes_encrypt(kv.second.value);

        size_t klen = enc_key.size();
        size_t vlen = enc_val.size();
        out.write(reinterpret_cast<char *>(&klen), sizeof(klen));
        out.write(enc_key.data(), klen);
        out.write(reinterpret_cast<char *>(&vlen), sizeof(vlen));
        out.write(enc_val.data(), vlen);
    }
    out.close();
}

void LRUCache::flushAOF()
{
    // Nothing fancy: we append on each op; this ensures flush
    std::ofstream out(aof_path_, std::ios::binary | std::ios::app);
    out.flush();
    out.close();
}

// -------------------- Internals --------------------
void LRUCache::set_internal(const std::string &key, const std::string &value, bool append)
{
    auto it = map_.find(key);
    if (it != map_.end())
    {
        it->second.value = value;
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second.it);
        it->second.it = lru_list_.begin();
    }
    else
    {
        if (map_.size() >= capacity_)
        {
            std::string lru_key = lru_list_.back();
            lru_list_.pop_back();
            map_.erase(lru_key);
            if (!loading_ && append)
                appendAOF_del(lru_key);
        }
        lru_list_.push_front(key);
        map_.emplace(key, Entry{value, lru_list_.begin()});
    }
    if (!loading_ && append)
        appendAOF_set(key, value);
}

void LRUCache::del_internal(const std::string &key, bool append)
{
    auto it = map_.find(key);
    if (it == map_.end())
        return;
    lru_list_.erase(it->second.it);
    map_.erase(it);
    if (!loading_ && append)
        appendAOF_del(key);
}

// -------------------- Persistence --------------------
void LRUCache::loadSnapshot()
{
    std::ifstream in(snapshot_path_, std::ios::binary);
    if (!in)
        return;

    while (in.peek() != EOF)
    {
        size_t klen = 0, vlen = 0;
        in.read(reinterpret_cast<char *>(&klen), sizeof(klen));
        std::string enc_key(klen, '\0');
        in.read(&enc_key[0], klen);

        in.read(reinterpret_cast<char *>(&vlen), sizeof(vlen));
        std::string enc_val(vlen, '\0');
        in.read(&enc_val[0], vlen);

        std::string key = aes_decrypt(enc_key);
        std::string val = aes_decrypt(enc_val);

        set_internal(key, val, false);
    }
    in.close();
}

void LRUCache::loadAOF()
{
    std::ifstream in(aof_path_, std::ios::binary);
    if (!in)
        return;

    while (in.peek() != EOF)
    {
        char op;
        in.read(&op, 1);
        if (op == 'S')
        {
            size_t klen = 0, vlen = 0;
            in.read(reinterpret_cast<char *>(&klen), sizeof(klen));
            std::string enc_key(klen, '\0');
            in.read(&enc_key[0], klen);
            in.read(reinterpret_cast<char *>(&vlen), sizeof(vlen));
            std::string enc_val(vlen, '\0');
            in.read(&enc_val[0], vlen);
            std::string key = aes_decrypt(enc_key);
            std::string val = aes_decrypt(enc_val);
            set_internal(key, val, false);
        }
        else if (op == 'D')
        {
            size_t klen = 0;
            in.read(reinterpret_cast<char *>(&klen), sizeof(klen));
            std::string enc_key(klen, '\0');
            in.read(&enc_key[0], klen);
            std::string key = aes_decrypt(enc_key);
            del_internal(key, false);
        }
    }
    in.close();
}

void LRUCache::appendAOF_set(const std::string &key, const std::string &value)
{
    std::ofstream out(aof_path_, std::ios::binary | std::ios::app);
    std::string enc_key = aes_encrypt(key);
    std::string enc_val = aes_encrypt(value);
    char op = 'S';
    out.write(&op, 1);
    size_t klen = enc_key.size();
    size_t vlen = enc_val.size();
    out.write(reinterpret_cast<char *>(&klen), sizeof(klen));
    out.write(enc_key.data(), klen);
    out.write(reinterpret_cast<char *>(&vlen), sizeof(vlen));
    out.write(enc_val.data(), vlen);
    out.close();
}

void LRUCache::appendAOF_del(const std::string &key)
{
    std::ofstream out(aof_path_, std::ios::binary | std::ios::app);
    std::string enc_key = aes_encrypt(key);
    char op = 'D';
    out.write(&op, 1);
    size_t klen = enc_key.size();
    out.write(reinterpret_cast<char *>(&klen), sizeof(klen));
    out.write(enc_key.data(), klen);
    out.close();
}

// -------------------- AES Encryption --------------------
std::string LRUCache::aes_encrypt(const std::string &plaintext)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return "";

    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    unsigned char iv[16];
    RAND_bytes(iv, sizeof(iv));

    EVP_EncryptInit_ex(ctx, cipher, nullptr,
                       reinterpret_cast<const unsigned char *>(aes_key_.data()), iv);

    std::vector<unsigned char> outbuf(plaintext.size() + 16);
    int outlen1 = 0;
    EVP_EncryptUpdate(ctx, outbuf.data(), &outlen1,
                      reinterpret_cast<const unsigned char *>(plaintext.data()), plaintext.size());

    int outlen2 = 0;
    EVP_EncryptFinal_ex(ctx, outbuf.data() + outlen1, &outlen2);

    EVP_CIPHER_CTX_free(ctx);

    std::string result(reinterpret_cast<char *>(iv), sizeof(iv));
    result += std::string(reinterpret_cast<char *>(outbuf.data()), outlen1 + outlen2);
    return result;
}

std::string LRUCache::aes_decrypt(const std::string &ciphertext)
{
    if (ciphertext.size() < 16)
        return "";
    const unsigned char *iv = reinterpret_cast<const unsigned char *>(ciphertext.data());
    const unsigned char *encdata = reinterpret_cast<const unsigned char *>(ciphertext.data() + 16);
    size_t encsize = ciphertext.size() - 16;

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return "";

    const EVP_CIPHER *cipher = EVP_aes_128_cbc();
    EVP_DecryptInit_ex(ctx, cipher, nullptr,
                       reinterpret_cast<const unsigned char *>(aes_key_.data()), iv);

    std::vector<unsigned char> outbuf(encsize + 16);
    int outlen1 = 0;
    EVP_DecryptUpdate(ctx, outbuf.data(), &outlen1, encdata, encsize);

    int outlen2 = 0;
    EVP_DecryptFinal_ex(ctx, outbuf.data() + outlen1, &outlen2);

    EVP_CIPHER_CTX_free(ctx);

    return std::string(reinterpret_cast<char *>(outbuf.data()), outlen1 + outlen2);
}
