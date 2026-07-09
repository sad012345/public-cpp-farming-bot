#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"

#include "../include/Trello.h"
#include "../include/Parser.h"
#include "../include/Game.h"
#include "../include/Proxy.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <cstdio>

using json = nlohmann::json;


void statusCheck(const httplib::Request& req,httplib::Response& res)
{
    res.set_content("Alive!","text/plain");
}

void reset(httplib::Client& cli,const std::string boardId,const std::string& key,const std::string& token)
{
    auto lists=Trello::getLists(cli,boardId,key,token);


    if(!lists.contains("Scan data")) Trello::createList(cli,boardId,key,token,"Scan data");
    else Trello::clearCards(cli,lists["Scan data"],key,token);

    if(!lists.contains("Command to drone")) Trello::createList(cli,boardId,key,token,"Command to drone");
    else Trello::clearCards(cli,lists["Command to drone"],key,token);

    if(!lists.contains("Command to server")) Trello::createList(cli,boardId,key,token,"Command to server");
    else Trello::clearCards(cli,lists["Command to server"],key,token);

    if(!lists.contains("ACKs")) Trello::createList(cli,boardId,key,token,"ACKs");
    else Trello::clearCards(cli,lists["ACKs"],key,token);

    if(!lists.contains("Data")) Trello::createList(cli,boardId,key,token,"Data");
    else Trello::clearCards(cli,lists["Data"],key,token);

    if(!lists.contains("Proxy response")) Trello::createList(cli,boardId,key,token,"Proxy response");
    else Trello::clearCards(cli,lists["Proxy response"],key,token);

    if(!lists.contains("Proxy request")) Trello::createList(cli,boardId,key,token,"Proxy request");
    else Trello::clearCards(cli,lists["Proxy request"],key,token);
}









int main()
{
    std::cout<<std::unitbuf;
    /*
    freopen("server.log","a",stdout);
    freopen("server.log","a",stderr); 
    */

    std::cout<<"====================New instance "<<std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
    <<"====================\n";





    const char* apiKeyEnv=std::getenv("APIKEY");
    const char* apiTokenEnv=std::getenv("APITOKEN");
    
    const char* apiKeyEnv=std::getenv("APIKEY");
    const char* apiTokenEnv=std::getenv("APITOKEN");
    const char* boardIdEnv = std::getenv("BOARDID");


    //Hard code your api stuff here
    const std::string apiKey = apiKeyEnv ? std::string(apiKeyEnv) : "";
    const std::string apiToken = apiTokenEnv ? std::string(apiTokenEnv) : "";
    const std::string boardId = boardIdEnv ? std::string(boardIdEnv) : "";


    if(apiKey.size()==0 || apiToken.size()==0)
    {
        std::cout<<"Missing auth env";
        return 1;
    }
    


    std::thread([]()//Starts "server"
    {
        const char* port_env = std::getenv("PORT");
        int port = port_env ? std::stoi(port_env) : 8080;
        httplib::Server farmingServer;
        farmingServer.Get("/",statusCheck);
        std::cout<<"Server running on port: "<<port<<"\n";
        if(!farmingServer.listen("0.0.0.0",port)) std::cerr<<"Server failed to start";
    }).detach();



    httplib::Client cli("https://api.trello.com");
    reset(cli,boardId,apiKey,apiToken);

    auto lists=Trello::getLists(cli,boardId,apiKey,apiToken);

    Game farmer;
    Proxy proxy;

    std::cout<<"Server ready!\n";

    while(true)//Listen to server commands
    {
        auto serverCMD = Trello::getCards(cli,lists["Command to server"],apiKey,apiToken);
        for(auto [cardName,cardData]:serverCMD)
        {
            if(cardData.desc == "Start farm")
            {
                std::cout<<"Starting farmer\n";
                farmer.startFarmer(lists["Scan data"],lists["Command to drone"],lists["ACKs"],lists["Data"],apiKey,apiToken);
            }
            else if(cardData.desc == "Stop farm")
            {
                std::cout<<"Stopping farmer\n";
                farmer.stopFarmer();
            } 


            else if(cardData.desc == "Start proxy")
            {
                std::cout<<"Starting proxy\n";
                proxy.startProxy(lists["Proxy request"],lists["Proxy response"],apiKey,apiToken);
            }
            else if(cardData.desc == "Stop proxy")
            {
                std::cout<<"Stoping proxy\n";
                proxy.stopProxy();
            }


            else if(cardData.desc == "Ping") Trello::createCard(cli,lists["Command to drone"],apiKey,apiToken,"Pong","Pong");
            else if(cardData.desc == "Reset") 
            {
                reset(cli,boardId,apiKey,apiToken);
                farmer.stopFarmer();
                proxy.stopProxy();
            }


            Trello::deleteCard(cli,cardData.id,apiKey,apiToken);
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }



    
    


    return 0;
}
