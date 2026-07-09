#include "../include/Parser.h"

#include "../third_party/nlohmann/json.hpp"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <queue>


using json = nlohmann::json;



std::vector<PlantData> Parser::parsePlantData(const std::string& text)
{
    std::vector<PlantData> plants;
    json data = json::parse(text);

    for(int a=0;a<2;a++)
    {
        for(int i=0;i<data[a].size()-1;i++)
        {
            long long encodedNum = data[a][i].get<long long>();
            long long timeHeader = data[a][data[a].size()-1].get<long long>();

            long long time   =    encodedNum%61;
            int plantNum     =   (encodedNum/61)%101;
            int fruitPercent =  ((encodedNum/61)/101)%101;
            int plantPercent = (((encodedNum/61)/101)/101)%101;
            int index        = (((encodedNum/61)/101)/101)/101;
            plants.push_back({plantNum,plantPercent,fruitPercent,index,time+timeHeader});
        }
    }

    return plants;
}


long long Parser::parseAckNum(const std::string& cardName)
{
    size_t pipePos = cardName.find('|');
    return std::stoll(cardName.substr(pipePos + 1));
}

seedData Parser::parseSeedData(const std::string& cardDesc)
{
    seedData returnObject;
    json seeds = json::parse(cardDesc);

    for(auto& seed:seeds[0])
    {
        for(int i=0;i<seed[1];i++) returnObject.seeds.push(seed[0].get<int>());
    }

    returnObject.totalSeeds = seeds[1];

    return returnObject;
}

proxyData Parser::parseRequest(const std::string& cardName)
{
    std::istringstream stream(cardName);
    
    proxyData ret;

    stream >> ret.id >> ret.host >> ret.path >> ret.method >> ret.awaitResponse;
    return ret;
}

proxyResponseData Parser::parseResponse(const std::string& cardName)
{
    std::istringstream stream(cardName);

    proxyResponseData ret;

    stream >> ret.type >> ret.id;
    return ret;
}

