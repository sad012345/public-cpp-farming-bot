#include "../include/Game.h"
#include "../include/Trello.h"
#include "../include/Parser.h"

#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <chrono>
#include <thread>

using json = nlohmann::json;






namespace
{
    std::unordered_map<std::string, Plant> PlantDataMap = 
    {
        // { "PlantName", { plantGrowthTime, fruitGrowthTime, harvestable? } }
        //-1 means undefined, -2 means to be inserted           // the slot thing is kinda deprecated, i just left it in cuz cool
        { "Empty",             {    0,       0,    false   } }, // Slot 0
        { "Potato",            {   -2,      -1,    false   } }, // Slot 1
        { "Carrot",            {   -2,      -1,    false   } }, // Slot 2
        { "Bush",              {   -2,      -1,    false   } }, // Slot 3
        { "Tree",              {   -2,      -1,    false   } }, // Slot 4
        { "AppleTree",         {   -2,      11,     true   } }, // Slot 5
        { "Onion",             {   -2,      -1,    false   } }, // Slot 6
        { "Pumpkin",           {   -2,      -1,    false   } }, // Slot 7
        { "StrawberryBush",    {   -2,      -2,     true   } }, // Slot 8
        { "BlueberryBush",     {   -2,      -2,     true   } }, // Slot 9
        { "TomatoBush",        {   -2,      -2,     true   } }, // Slot 10
        { "GrapeTree",         {   -2,      -2,     true   } }, // Slot 11
        { "Bamboo",            {   -2,      -1,    false   } }, // Slot 12
        { "CornBush",          {   -2,      -2,     true   } }, // Slot 13
        { "CactusTree",        {   -2,      -2,     true   } }, // Slot 14
        { "PineappleTree",     {   -2,      -2,     true   } }, // Slot 15
        { "PearTree",          {   -2,      -2,     true   } }, // Slot 16
        { "PepperTree",        {   -2,      -2,     true   } }, // Slot 17
        { "BananaTree",        {  120,      -2,     true   } }, // Slot 18
        { "Watermelon",        {   -2,      -1,    false   } }, // Slot 19
        { "Mushroom",          {  160,      -1,    false   } }, // Slot 20
        { "MangoTree",         {   -2,      -2,     true   } }, // Slot 21
        { "CoconutTree",       {   -2,      -2,     true   } }, // Slot 22
        { "CacaoTree",         {   -2,      -2,     true   } }, // Slot 23
        { "LotusBush",         {   -2,      -2,     true   } }, // Slot 24
        { "KiwiTree",          {   -2,      -2,     true   } }, // Slot 25
        { "LemonTree",         {  501,      -2,     true   } }, // Slot 26
        { "Garlic",            {   -2,      -1,    false   } }, // Slot 27
        { "PomegranateTree",   {   -2,      -2,     true   } }, // Slot 28
        { "CherryBush",        {   -2,      -2,     true   } }, // Slot 29
        { "DragonTree",        {   -2,      -2,     true   } }, // Slot 30
        { "StarfruitTree",     { 2000,    1051,     true   } }, // Slot 31
        { "GoldenAppleTree",   {   -2,      -2,     true   } }, // Slot 32
        { "Glttch",            {   -1,      -1,    false   } }, // Slot 33
        { "Wheat",             {   -2,      -1,    false   } }  // Slot 34
    };
}


long long getTime()
{
    return std::chrono::duration_cast<std::chrono::seconds>
    (
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::vector<std::vector<std::string>> plantNumList;

Plant lookUp(int plantNum)
{
    for(auto plantName:plantNumList[plantNum])
    {
        if(PlantDataMap.contains(plantName)) return PlantDataMap[plantName];
    }

    return {-2, -2,false};
}

std::string lookUpSeed(int seedNum)
{
    std::cout<<"Starting seed lookUp\n";
    for(auto plantName:plantNumList[seedNum])
    {
        std::cout<<"Checking: "<<plantName<<'\n';
        if(plantName.find("Tree") == std::string::npos && plantName.find("Bush") == std::string::npos) return plantName;
        if(plantName == "Tree" || plantName == "Bush") return plantName;
    }
    throw std::runtime_error("lookUpSeed reached the end without returning a value.Attempted seed look up: "+std::to_string(seedNum));
}

long long calculateReadyTimeNClassify(tile& square,long long scanTime,const Plant& cachedPlant)
{
    using enum tileState;
    if(square.PlantNum == 0)
    {
        square.state = Empty;
        return std::numeric_limits<long long>::max();
        /*
            Having ready time at 0 kinda means action is required,
            to prevent confusion I decided to use max instead
            (the code won't check this but might as well add it)
        */
    }
    if(square.harvestable)
    {
        if(square.plantPercent != 100)
        {
            if(cachedPlant.plantGrowTime == -2)
            {
                square.state = WaitingForScan;
                return std::numeric_limits<long long>::max();
            }
            else
            {
                square.state = Simulated;
                return static_cast<long long>(cachedPlant.plantGrowTime * (1.0-square.plantPercent/100.0) + scanTime);
            }                
        }
        else if(square.fruitPercent != 100)
        {
            if(cachedPlant.fruitGrowTime == -2)
            {
                square.state = WaitingForScan;
                return std::numeric_limits<long long>::max();
            }
            else
            {
                square.state = Simulated;
                return static_cast<long long>(cachedPlant.fruitGrowTime * (1.0-square.fruitPercent/100.0) + scanTime);
            }
        }
        else
        {
            square.state = Simulated;
            return scanTime;
        }
    }
    else
    {
        if(square.plantPercent != 100)
        {
            if(cachedPlant.plantGrowTime == -2) 
            {
                square.state = WaitingForScan;
                return std::numeric_limits<long long>::max();
            }
            else
            {
                square.state = Simulated;
                return static_cast<long long>(cachedPlant.plantGrowTime * (1.0-square.plantPercent/100.0) + scanTime);
            }    
        }
        else
        {
            square.state = Simulated;
            return scanTime;
        }                           
    }
    throw std::runtime_error("Reached the end of calculate ready time");
}




void mapUpdater(std::stop_token st,std::vector<tile>& map,std::mutex& mapLock,seedData& seedList,std::mutex& seedListLock,httplib::Client& cli,const std::string& scanDataListId,const std::string& key,const std::string& token)
{
    std::vector<PlantData> processedScanData;
    processedScanData.reserve(729);

    while(!st.stop_requested())
    {
        auto scanData = Trello::getCards(cli,scanDataListId,key,token);
        

        for(auto& [cardName,cardData]:scanData)
        {
            if(cardName != "Seed data")
            {
                std::cout<<"Scanning card: "<<cardName<<" Data: "<<cardData.desc<<'\n';
                Trello::deleteCard(cli,cardData.id,key,token);
                auto plant = Parser::parsePlantData(cardData.desc);

                //for(auto& p:plant) processedScanData[p.index]=p;
                processedScanData.insert(processedScanData.end(),plant.begin(),plant.end());
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock(seedListLock);
                    seedList = Parser::parseSeedData(cardData.desc);
                }
                Trello::deleteCard(cli,cardData.id,key,token);
            } 
        }
        std::cout<<"=====Processed scan data:=====\n";
        for(auto& sD:processedScanData)
        {
            if(sD.PlantNum != 0) std::cout<<"PlantNum"<<sD.PlantNum<<'\n'<<"Plant Percent"<<sD.PlantPercent<<'\n'<<"Fruit Percent"<<sD.FruitPercent<<'\n'<<"Time"<<sD.time<<'\n'<<"Index"<<sD.index<<'\n';
        }

            {
                std::lock_guard<std::mutex> lock(mapLock);
                for(auto& plantTile:processedScanData)
                {
                    int i = plantTile.index;
                    switch(map[i].state)
                    {
                        using enum tileState;

                        case(WaitingForScan):
                        {
                            auto cachedPlant = lookUp(plantTile.PlantNum);

                            map[i].PlantNum = plantTile.PlantNum;
                            map[i].harvestable = cachedPlant.harvestable;
                            map[i].fruitPercent = plantTile.FruitPercent;
                            map[i].plantPercent = plantTile.PlantPercent;
                            map[i].readyTime = calculateReadyTimeNClassify(map[i],plantTile.time,cachedPlant);
                        }

                        break;
                        
                        default:
                            
                        break;
                    }
                }
            }
        processedScanData.clear();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void mapSimulator(std::stop_token st,std::vector<tile>&map,std::mutex& mapLock,std::queue<json>& commandQueue,std::mutex& commandQueueLock,seedData& seedList,std::mutex& seedListLock,long long& droneACK,std::mutex& droneACKLock,long long& droneCMDNumber,std::mutex& droneCMDNumberLock)
{
    json commands = json::array();
    long long loadedDroneACK = 0;
    long long loadedDroneCMDNumber = 1;

    int totalReservedSeeds = 0;
    std::unordered_map<int,int> reservedSeeds;

    while(!st.stop_requested())
    {
        {//Loads droneACK
            std::lock_guard<std::mutex> lock(droneACKLock);
            loadedDroneACK = droneACK;
        }

        {
            std::lock_guard<std::mutex> lock(mapLock);
            for(int i=0;i<729;i++)
            {
                if(commands.size() == 100)
                {
                    std::lock_guard<std::mutex> lock(commandQueueLock);
                    commandQueue.push(std::move(commands));
                    commands = json::array();
                    {
                        std::lock_guard<std::mutex> lock(droneCMDNumberLock);
                        droneCMDNumber++;
                        loadedDroneCMDNumber = droneCMDNumber;
                    }
                }

                switch(map[i].state)
                {
                    using enum tileState;
                    case(Simulated):
                    {
                        std::cout<< "Tile " << i << " plantNum: " <<map[i].PlantNum<<" state=Simulated"<< " readyTime=" << map[i].readyTime<< " now=" << getTime()<< '\n';
                        int x = i%27 - 13;
                        int y = i/27 -13;
                        if(getTime() >= map[i].readyTime)
                        {
                            if(map[i].harvestable) 
                            {
                                commands.push_back({"h",x,y});
                                map[i].state = QueuedHarvest;
                                map[i].queueNumber = loadedDroneCMDNumber;
                            }
                            else
                            {
                                commands.push_back({"c",x,y});
                                map[i].state = QueuedCrop;
                                map[i].queueNumber = loadedDroneCMDNumber;
                            }
                        }
                    } 
                    break;

                    case(Empty):
                    {
                        std::lock_guard<std::mutex> lock(seedListLock);

                        if(totalReservedSeeds >= seedList.totalSeeds || seedList.seeds.size() == 0) continue;
                        int x = i%27 - 13;
                        int y = i/27 -13;

                        //Parse seed name
                        std::string seed = lookUpSeed(seedList.seeds.front());

                        commands.push_back({"p",x,y,seed});
                        seedList.seeds.pop();

                        map[i].state = QueuedPlant;
                        map[i].queueNumber = loadedDroneCMDNumber;

                        totalReservedSeeds++;
                        reservedSeeds[loadedDroneCMDNumber] += 1;
                    }
                    break;
                    
                    //de-queue
                    case(QueuedCrop):
                    case(QueuedHarvest):
                        if(loadedDroneACK>=map[i].queueNumber)
                        {
                            map[i].state = WaitingForScan;
                            map[i].queueNumber = 0;
                        }
                    break;

                    case(QueuedPlant):
                        if(loadedDroneACK>=map[i].queueNumber)
                        {
                            map[i].state = WaitingForScan;
                            totalReservedSeeds -= reservedSeeds[map[i].queueNumber];
                            map[i].queueNumber = 0;
                        }
                    break;

                    default:

                    break;
                }
            }
        }
        
        if(!commands.empty())
        {
            std::lock_guard<std::mutex> lock(commandQueueLock);
            commandQueue.push(std::move(commands));
            commands = json::array();
            {
                std::lock_guard<std::mutex> lock(droneCMDNumberLock);
                droneCMDNumber++;
                loadedDroneCMDNumber = droneCMDNumber;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}


void commandHandler(std::stop_token st,std::queue<json>& commandQueue,std::mutex& commandQueueLock,long long& droneCMDNumber,std::mutex& droneCMDNumberLock,httplib::Client& cli,const std::string& commandListId,const std::string& key,const std::string& token)
{
    long long internaldroneCMDNumber = 1;
    while(!st.stop_requested())
    {
        std::queue<json> commands;

        {
            std::lock_guard<std::mutex> lock(commandQueueLock);
            if(!commandQueue.empty()) commands.swap(commandQueue);
        }

        while(commands.size()>0)
        {
            std::cout<<"Creating command\n";
            Trello::createCard(cli,commandListId,key,token,"DroneCMD|"+std::to_string(internaldroneCMDNumber),commands.front().dump());
            commands.pop();
            internaldroneCMDNumber++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


void cleanUp(std::stop_token st,std::mutex& droneACKLock,long long& droneACK,httplib::Client& cli,const std::string ACKsListId,const std::string commandListId,const std::string key, const std::string token)
{
    while(!st.stop_requested())
    {
        auto ACKs=Trello::getCards(cli,ACKsListId,key,token);
        {
            std::lock_guard<std::mutex> lock(droneACKLock);
            for(auto& [cardName,cardData]:ACKs)
            {
                json cardDesc = json::parse(cardData.desc);

                droneACK = std::max(droneACK,std::stoll(cardDesc["Drone"].dump()));
                //serverACK = std::max(serverACK,std::stoll(cardDesc["Server"].dump()));

                Trello::deleteCard(cli,cardData.id,key,token);
            }
        }
                

        auto commandCards = Trello::getCards(cli,commandListId,key,token);

        for(auto& [cardName,cardData]:commandCards)
        {
            long long cardDate = Parser::parseAckNum(cardName);
            if(cardDate<=droneACK) Trello::deleteCard(cli,cardData.id,key,token);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


Game::~Game()
{
    if(!running) std::cout<<"Farm is not running, ignoring request\n";
    else stopFarmer();
}

void Game::startFarmer(const std::string& scanDataListId,const std::string& commandListId,const std::string& ACKsListId,const std::string& dataListId,const std::string& key,const std::string& token)
{
    if(running) std::cout<<"Farm is running, ignoring request\n";
    else
    {
        map.clear();
        map.resize(729);
        droneCMDNumber = 1;
        droneACK = 0;
        
        running = true;
        json commands = json::array();
        {
            json tempPlantNum = json::object();
            httplib::Client cli("https://api.trello.com");
            auto data = Trello::getCards(cli,dataListId,key,token);
            for(auto& [cardName,cardData]:data)
            {
                if(cardName == "PlantNum")
                {
                    tempPlantNum = json::parse(cardData.desc);
                    Trello::deleteCard(cli,cardData.id,key,token);
                }
            }
            plantNumList.resize(tempPlantNum.size()/3 + 1);
            for(auto& [plantName,number]:tempPlantNum.items())
            {
                int n = number.get<int>();
                plantNumList[n].push_back(plantName);
            }
        }

            try
            {
                mapSim = std::jthread([this](std::stop_token st)
                {
                    mapSimulator(st,map,mapLock,commandQueue,commandQueueLock,seedList,seedListLock,droneACK,droneACKLock,droneCMDNumber,droneCMDNumberLock);
                });

                mapUPD = std::jthread([this,scanDataListId,key,token](std::stop_token st)
                {
                    httplib::Client mapUpdateClient("https://api.trello.com");
                    mapUpdater(st,map,mapLock,seedList,seedListLock,mapUpdateClient,scanDataListId,key,token);
                });

                CMDHandler = std::jthread([this,commandListId,key,token](std::stop_token st)
                {
                    httplib::Client CMDHandlerClient("https://api.trello.com");
                    commandHandler(st,commandQueue,commandQueueLock,droneCMDNumber,droneCMDNumberLock,CMDHandlerClient,commandListId,key,token);
                });

                cUP = std::jthread([this,ACKsListId,commandListId,key,token](std::stop_token st)
                {
                    httplib::Client cleanUpClient("https://api.trello.com");
                    cleanUp(st,droneACKLock,droneACK,cleanUpClient,ACKsListId,commandListId,key,token);
                });
            }
            catch(const std::exception& e)
            {
                std::cout << "ERROR: " << e.what() << "\n";
            }
        std::cout<<"Successful startup\n"; 
    }
}

void Game::stopFarmer()
{
    mapSim.request_stop();
    mapUPD.request_stop();
    CMDHandler.request_stop();
    cUP.request_stop();

    if (mapSim.joinable()) mapSim.join();
    if (mapUPD.joinable()) mapUPD.join();
    if (CMDHandler.joinable()) CMDHandler.join();
    if (cUP.joinable()) cUP.join();

    running = false;
    std::cout<<"Farmer stopped\n";
}