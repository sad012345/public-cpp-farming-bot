#pragma once

#include <vector>
#include <string>
#include <queue>


struct PlantData
{
    int PlantNum = 0;
    int PlantPercent = 0;
    int FruitPercent = 0;
    int index = 0;
    long long time = 0;
};

struct seedData
{
    int totalSeeds = 0;
    std::queue<int> seeds;
};

struct proxyData
{
    long long id = 0;
    std::string host;
    std::string path;
    std::string method;
    int awaitResponse;
};

struct proxyResponseData
{
    std::string type;
    long long id = 0;
};


namespace Parser
{
    std::vector<PlantData> parsePlantData(const std::string& text);
    long long parseAckNum(const std::string& cardName);
    seedData parseSeedData(const std::string& cardDesc);
    proxyData parseRequest(const std::string& cardName);
    proxyResponseData parseResponse(const std::string& cardName);
}