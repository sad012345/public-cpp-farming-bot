#pragma once

#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"

#include "../include/Parser.h"

#include <string>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>

using json = nlohmann::json;

enum class tileState
{
    Empty,

    WaitingForScan,
    Simulated,

    QueuedCrop,
    QueuedHarvest,
    QueuedPlant
};

struct tile
{
    int PlantNum = 0;

    bool harvestable = false;

    int plantPercent = 0;
    int fruitPercent = 0;

    long long readyTime = 0;

    long long queueNumber = 0;

    tileState state = tileState::WaitingForScan;

};

struct Plant
{
    int plantGrowTime;
    int fruitGrowTime;
    bool harvestable;
};

class Game
{
    private:
        bool running = false;


        std::vector<tile> map;
        std::mutex mapLock;

        seedData seedList;
        std::mutex seedListLock;

        std::queue<json> commandQueue;
        std::mutex commandQueueLock;

        long long droneCMDNumber;
        std::mutex droneCMDNumberLock;

        long long droneACK;
        std::mutex droneACKLock;

        std::jthread mapSim;
        std::jthread mapUPD;
        std::jthread CMDHandler;
        std::jthread cUP;


    public:
        ~Game();
        void startFarmer(const std::string& scanDataListId, const std::string& commandListId,const std::string& ACKsListId,const std::string& dataListId,const std::string& key,const std::string& token);
        void stopFarmer();
};

