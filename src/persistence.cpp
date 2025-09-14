#include "persistence.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <cstring>

Persistence::Persistence(const std::string &snapshot, const std::string &aof)
    : snapshotFile(snapshot), aofFile(aof) {}

void Persistence::saveSnapshot(const std::unordered_map<std::string, std::string> &db)
{
    std::ofstream ofs(snapshotFile, std::ios::binary);
    if (!ofs.is_open())
        return;

    // Serialize DB to string
    std::ostringstream oss;
    for (auto &[k, v] : db)
    {
        oss << k << "=" << v << "\n";
    }
    std::string data = oss.str();

    // AES key/iv
    const char *rawKey = "12345678901234567890123456789012"; // 32 bytes
    const char *rawIv = "1234567890123456";                  // 16 bytes
    unsigned char key[32];
    unsigned char iv[16];
    memcpy(key, rawKey, 32);
    memcpy(iv, rawIv, 16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

    std::vector<unsigned char> encrypted(data.size() + AES_BLOCK_SIZE);
    int outlen1 = 0, outlen2 = 0;

    EVP_EncryptUpdate(ctx, encrypted.data(), &outlen1,
                      (unsigned char *)data.data(), data.size());
    EVP_EncryptFinal_ex(ctx, encrypted.data() + outlen1, &outlen2);
    EVP_CIPHER_CTX_free(ctx);

    ofs.write((char *)encrypted.data(), outlen1 + outlen2);
    ofs.close();

    std::cout << "[Snapshot] Saved encrypted snapshot.\n";

    // Reset AOF after snapshot
    std::ofstream aof(aofFile, std::ios::trunc);
    aof << "# Snapshot taken\n";
    aof.close();
}

void Persistence::appendCommand(const std::string &command)
{
    std::ofstream ofs(aofFile, std::ios::app);
    ofs << command << "\n";
    ofs.close();
    commandCount++;
}

void Persistence::load(std::unordered_map<std::string, std::string> &db)
{
    // 1. Try loading snapshot
    std::ifstream ifs(snapshotFile, std::ios::binary);
    if (ifs.is_open())
    {
        std::vector<unsigned char> encrypted((std::istreambuf_iterator<char>(ifs)),
                                             std::istreambuf_iterator<char>());
        ifs.close();

        if (!encrypted.empty())
        {
            const char *rawKey = "12345678901234567890123456789012";
            const char *rawIv = "1234567890123456";
            unsigned char key[32];
            unsigned char iv[16];
            memcpy(key, rawKey, 32);
            memcpy(iv, rawIv, 16);

            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

            std::vector<unsigned char> decrypted(encrypted.size());
            int outlen1 = 0, outlen2 = 0;

            EVP_DecryptUpdate(ctx, decrypted.data(), &outlen1,
                              encrypted.data(), encrypted.size());
            EVP_DecryptFinal_ex(ctx, decrypted.data() + outlen1, &outlen2);
            EVP_CIPHER_CTX_free(ctx);

            std::string snapshotData((char *)decrypted.data(), outlen1 + outlen2);

            std::istringstream iss(snapshotData);
            std::string line;
            while (std::getline(iss, line))
            {
                size_t eq = line.find('=');
                if (eq != std::string::npos)
                {
                    std::string keyStr = line.substr(0, eq);
                    std::string valStr = line.substr(eq + 1);
                    db[keyStr] = valStr;
                }
            }
            std::cout << "[Load] Snapshot restored.\n";
        }
    }

    // 2. Replay AOF log
    std::ifstream aof(aofFile);
    if (aof.is_open())
    {
        std::string line;
        while (std::getline(aof, line))
        {
            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string cmd, key, value;
            iss >> cmd;
            if (cmd == "SET")
            {
                iss >> key >> value;
                db[key] = value;
            }
            else if (cmd == "DEL")
            {
                iss >> key;
                db.erase(key);
            }
        }
        aof.close();
        std::cout << "[Load] AOF replayed.\n";
    }
}

void Persistence::periodicSnapshot(std::unordered_map<std::string, std::string> &db)
{
    if (commandCount >= SNAPSHOT_INTERVAL)
    {
        saveSnapshot(db);
        commandCount = 0;
    }
}
