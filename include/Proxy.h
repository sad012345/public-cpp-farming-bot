#pragma once

#include "../third_party/nlohmann/json.hpp"

#include <thread>

using json=nlohmann::json;


struct proxyResponse
{
    json header = json::object();
    json body = json::object();
};

class Proxy
{
    private:
        std::jthread proxyThread;
        bool running = false;




    public:
        void startProxy(const std::string& proxyRequestID,const std::string& proxyResponseID,const std::string& key,const std::string& token);
        void stopProxy();
};
