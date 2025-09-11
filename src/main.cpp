#include "cache.h"
#include <iostream>
#include <sstream>
#include <algorithm>

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
    LRUCache cache(capacity, "data/aof.log");
    std::cout << "redis-lite (toy) — capacity=" << capacity << "\n";
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
        // uppercase command
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

        if (cmd == "SET")
        {
            std::string key;
            iss >> key;
            if (key.empty())
            {
                std::cout << "ERR wrong number of args for 'SET'\n";
                continue;
            }
            // get value = rest of line after key
            auto pos = line.find(key);
            if (pos == std::string::npos)
            {
                std::cout << "ERR\n";
                continue;
            }
            pos += key.size();
            std::string value = trim(line.substr(pos));
            cache.set(key, value);
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
            std::cout << (removed ? "1\n" : "0\n");
        }
        else if (cmd == "INFO")
        {
            std::cout << "entries: " << cache.size() << " capacity: " << cache.capacity() << "\n";
        }
        else if (cmd == "SAVE")
        {
            cache.saveAOF();
            std::cout << "AOF flushed\n";
        }
        else if (cmd == "EXIT" || cmd == "QUIT")
        {
            std::cout << "bye\n";
            break;
        }
        else
        {
            std::cout << "ERR unknown command\n";
        }
    }
    return 0;
}
