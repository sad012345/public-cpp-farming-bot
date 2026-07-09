#include "../include/Trello.h"

#include "../third_party/httplib.h"
#include "../third_party/nlohmann/json.hpp"

#include <unordered_map>
#include <iostream>
#include <string>

using json = nlohmann::json;



std::unordered_map<std::string,std::string>Trello::getLists(httplib::Client& cli,const std::string& board,const std::string& key,const std::string& token)
{
    std::string path= "/1/boards/"+board+"/lists?fields=name,id&key="+key+"&token="+token;
    auto res = cli.Get(path.c_str());
    if(!res) return json{};
    json data = json::parse(res->body);
    std::unordered_map<std::string,std::string> ret;
    for(auto& d:data) ret[d["name"]]=d["id"];
    return ret;
}


void Trello::createList(httplib::Client& cli,const std::string& board,const std::string& key,const std::string& token,const std::string& name)
{
    std::string path="/1/boards/"+board+"/lists";
    httplib::Params params =
    {
        {"name",name},
        {"key",key},
        {"token",token}
    };
    if(auto res = cli.Post(path.c_str(),params)) std::cout<<"Success"<<"\n";
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
}


void Trello::createCard(httplib::Client& cli,const std::string& list,const std::string& key,const std::string& token,const std::string name,const std::string desc)
{
    std::string path = "/1/cards";
    httplib::Params params =
    {
        {"idList",list},
        {"name",name},
        {"desc",desc},
        {"key",key},
        {"token",token}
    };
    if(auto res = cli.Post(path.c_str(),params)) std::cout<<"Created card: "<<name<<"\n";
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
}

void Trello::deleteCard(httplib::Client& cli,const std::string& cardId,const std::string& key,const std::string& token)
{
    std::string path = "/1/cards/"+cardId+"?key="+key+"&token="+token;
    if(auto res = cli.Delete(path.c_str())) std::cout<<"Deleted a card"<<"\n";
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
}


std::unordered_map<std::string,CardIdmappedDesc> Trello::getCards(httplib::Client& cli,const std::string& list,const std::string& key,const std::string& token)
{
    std::string path = "/1/lists/"+list+"/cards?fields=name,id,desc"+"&key="+key+"&token="+token;
    if(auto res = cli.Get(path.c_str()))
    {
        json data = json::parse(res->body);
        std::unordered_map<std::string,CardIdmappedDesc> ret;
        for(auto& d:data)
        {
            ret[d["name"]].id = d["id"].get<std::string>();
            ret[d["name"]].desc = d["desc"].get<std::string>();
        }
        return ret;
    }
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
    return {};
}


std::string Trello::getCardData(httplib::Client& cli,const std::string& cardId,const std::string& key,const std::string& token)
{
    std::string path = "/1/cards/"+cardId+"?fields=name,desc"+"&key="+key+"&token="+token;
    if(auto res = cli.Get(path.c_str()))
    {
        json data = json::parse(res->body);
        return data["desc"];
    }
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
    return "";
}


std::string Trello::getCardDataNRemove(httplib::Client& cli,const std::string& cardId,const std::string& key,const std::string& token)
{
    std::string path = "/1/cards/"+cardId+"?closed=true"+"&key="+key+"&token="+token;
    if(auto res = cli.Put(path.c_str()))
    {
        json data = json::parse(res->body);
        return data["desc"];
    }
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
    return "";
}

void Trello::clearCards(httplib::Client& cli,const std::string& listId,const std::string& key,const std::string& token)
{
    std::string path = "/1/lists/"+listId+"/archiveAllCards?"+"key="+key+"&token="+token;
    if(auto res = cli.Post(path.c_str()));
    else std::cout<<"Error: "<<httplib::to_string(res.error())<<"\n";
}