#include "../include/Proxy.h"

#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"

#include "../include/Trello.h"
#include "../include/Parser.h"

#include <thread>

using json = nlohmann::json;

long long getProxyTime()
{
    return std::chrono::duration_cast<std::chrono::seconds>
    (
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

proxyResponse processResponse(const httplib::Result& response)
{
    proxyResponse ret;
    if (!response) 
    {
        httplib::Error err = response.error();
        std::cerr << "Connection failed! Error code: " << static_cast<int>(err) << '\n';
        ret.body = {{"Error",static_cast<int>(err)}};
        return ret;
    }
    if (response->status < 200 || response->status > 299)//all 2xx is actually success!
    {
        std::cerr << "HTTP Error Code: " << response->status<<'\n';
        std::cerr << "Server Response: " << response->body<<'\n';
        ret.body =
        {
            {"Error","HTTP error"},
            {"Status", response->status}
        };
        try
        {
            ret.body["Response"] = json::parse(response->body);
        }
        catch(...)
        {
            ret.body["Response"] = response->body;
        }
        return ret;
    }

    try
    {
        ret.body = json::parse(response->body);
    }
    catch(...)
    {
        ret.body = response->body;
    }

    for(const auto& [key,value]:response->headers)
    {
        if(ret.header.contains(key)) ret.header[key].push_back(value);
        else ret.header[key] = json::array({value});
    }
    return ret;
}

proxyResponse sendRequest(proxyData request,const httplib::Headers& header,const json& body)
{
    try
    {
        httplib::Client cli(request.host);
        if(request.method == "POST")
        {
            auto response = cli.Post(request.path,header,body.dump(),"application/json");
            return processResponse(response);
        }
        else if(request.method == "GET")
        {
            auto response = cli.Get(request.path,header);
            return processResponse(response);
        }
        else if(request.method == "PUT")
        {
            auto response = cli.Put(request.path,header,body.dump(),"application/json");
            return processResponse(response);
        }
        else if(request.method == "PATCH")
        {
            auto response = cli.Patch(request.path,header,body.dump(),"application/json");
            return processResponse(response);
        }
        else if(request.method == "DELETE")
        {
            auto response = cli.Delete(request.path,header,body.dump(),"application/json");
            return processResponse(response);
        }
        throw std::runtime_error("Somehow a malformed card got through!\n");
    }
    catch(const std::exception& e)
    {
        std::cout << "Proxy error: " << e.what() << "\n";
    }
}

void proxyHandler(std::stop_token st,const std::string& proxyRequestID,const std::string& proxyResponseID,const std::string& key,const std::string& token)
{
    httplib::Client cli("https://api.trello.com");
    while(!st.stop_requested())
    {
        auto requests = Trello::getCards(cli,proxyRequestID,key,token);
        for(auto& [cardName,cardData]:requests)
        {
            Trello::deleteCard(cli,cardData.id,key,token);
            
            httplib::Headers headers;
            json body = json::object();
            proxyData request = Parser::parseRequest(cardName);

            json data = json::parse(cardData.desc);
            for(auto& [key,value]:data["Header"].items())
            {
                if(value.is_array()) for(auto& item:value) headers.insert({key,item.is_string() ? item.get<std::string>() : item.dump()});
                else headers.insert({key,value.is_string() ? value.get<std::string>() : value.dump()});
            }
            body.merge_patch(data["Body"]);

            auto response = sendRequest(request,headers,body);
            switch(request.awaitResponse)
            {
                case(0):
                    continue;
                break;

                case(1):
                    Trello::createCard(cli,proxyResponseID,key,token,"Body "+std::to_string(request.id),response.body);
                    Trello::createCard(cli,proxyResponseID,key,token,"done "+std::to_string(request.id),"");
                break;

                case(2):
                    Trello::createCard(cli,proxyResponseID,key,token,"Header "+std::to_string(request.id),response.header.dump());
                    Trello::createCard(cli,proxyResponseID,key,token,"Body "+std::to_string(request.id),response.body.dump());
                    Trello::createCard(cli,proxyResponseID,key,token,"done "+std::to_string(request.id),"");
                break;
            }


            auto proxyResponseCards = Trello::getCards(cli,proxyResponseID,key,token);

            for(auto& [cardName,cardData]:proxyResponseCards)
            {
                auto proxyResponse = Parser::parseResponse(cardName);
                if (getProxyTime() - proxyResponse.id == 120) Trello::deleteCard(cli,cardData.id,key,token);
            }

        }               
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}


void Proxy::startProxy(const std::string& proxyRequestID,const std::string& proxyResponseID,const std::string& key,const std::string& token)
{
    if(running) std::cout<<"Proxy already running, request ignored\n";
    else
    {
        proxyThread = std::jthread([&proxyRequestID,&proxyResponseID,&key,&token](std::stop_token st)
        {
            proxyHandler(st,proxyRequestID,proxyResponseID,key,token);
        });
        running = true;
    }
}

void Proxy::stopProxy()
{
    if(!running)
    {
        std::cout<<"Proxy already stopped, request ignored\n";
    }
    else
    {
        proxyThread.request_stop();
        if(proxyThread.joinable()) proxyThread.join();
        running = false;
    }
}