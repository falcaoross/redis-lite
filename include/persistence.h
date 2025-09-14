// persistence.h
#pragma once
#include <unordered_map>
#include <string>

class Persistence
{
public:
    Persistence(const std::string &snapshotFile, const std::string &aofFile);
    void saveSnapshot(const std::unordered_map<std::string, std::string> &db);
    void appendCommand(const std::string &command);
    void load(std::unordered_map<std::string, std::string> &db);

    void periodicSnapshot(std::unordered_map<std::string, std::string> &db);

private:
    std::string snapshotFile;
    std::string aofFile;
    int commandCount = 0;
    const int SNAPSHOT_INTERVAL = 5; // every 5 commands
};
