#include "cache.h"
#include "persistence.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

static inline std::string trim(const std::string &s)
{
    auto a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos)
        return "";
    auto b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

int main(int argc, char **argv)
{
    size_t capacity = 1000;
    if (argc >= 2)
    {
        try
        {
            capacity = std::stoul(argv[1]);
        }
        catch (...)
        {
        }
    }

    // Our cache
    LRUCache cache(capacity);

    // Persistence manager
    Persistence persistence("data/snapshot.rdb", "data/aof.log");

    // DB backing store
    std::unordered_map<std::string, std::string> db;
    persistence.load(db); // restore from snapshot + AOF

    std::cout << "redis-lite (toy) AES + Hybrid Snapshot/AOF — capacity=" << capacity << "\n";
    std::cout << "Commands: SET key value | GET key | DEL key | INFO | SAVE | EXIT\n";

    std::string line;
    while (true)
    {
        std::cout << "rl> ";
        if (!std::getline(std::cin, line))
            break;
        line = trim(line);
        if (line.empty())
            continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "SET")
        {
            std::string key, value;
            iss >> key >> value;
            if (key.empty() || value.empty())
            {
                std::cout << "ERR wrong number of args for 'SET'\n";
                continue;
            }
            cache.set(key, value);
            db[key] = value;

            persistence.appendCommand("SET " + key + " " + value);
            persistence.periodicSnapshot(db);

            std::cout << "OK\n";
        }
        else if (cmd == "GET")
        {
            std::string key;
            iss >> key;
            if (key.empty())
            {
                std::cout << "(nil)\n";
                continue;
            }
            std::string val;
            if (cache.get(key, val))
                std::cout << val << "\n";
            else
                std::cout << "(nil)\n";
        }
        else if (cmd == "DEL")
        {
            std::string key;
            iss >> key;
            if (key.empty())
            {
                std::cout << "ERR\n";
                continue;
            }
            bool removed = cache.del(key);
            if (removed)
            {
                db.erase(key);
                persistence.appendCommand("DEL " + key);
                persistence.periodicSnapshot(db);
            }
            std::cout << (removed ? "1\n" : "0\n");
        }
        else if (cmd == "INFO")
        {
            std::cout << "entries: " << cache.size() << " capacity: " << cache.capacity() << "\n";
        }
        else if (cmd == "SAVE")
        {
            persistence.saveSnapshot(db);
            std::cout << "Snapshot saved manually.\n";
        }
        else if (cmd == "EXIT" || cmd == "QUIT")
        {
            std::cout << "bye\n";
            persistence.saveSnapshot(db);
            break;
        }
        else
        {
            std::cout << "ERR unknown command\n";
        }
    }
    return 0;
}
