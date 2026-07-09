#pragma once

#include "../third_party/httplib.h"

#include <unordered_map>
#include <string>

struct CardIdmappedDesc
{
    std::string id;
    std::string desc;
};

namespace Trello
{
    std::unordered_map<std::string,std::string> getLists(httplib::Client& cli,const std::string& board,const std::string& key,const std::string& token);
    void createList(httplib::Client& cli,const std::string& board,const std::string& key,const std::string& token,const std::string& name);
    void createCard(httplib::Client& cli,const std::string& list,const std::string& key,const std::string& token,const std::string name,const std::string desc);
    void deleteCard(httplib::Client& cli,const std::string& cardId,const std::string& key,const std::string& token);
    std::unordered_map<std::string,CardIdmappedDesc> getCards(httplib::Client& cli,const std::string& list,const std::string& key,const std::string& token);
    std::string getCardData(httplib::Client& cli,const std::string& cardId,const std::string& key,const std::string& token);
    std::string getCardDataNRemove(httplib::Client& cli,const std::string& cardId,const std::string& key,const std::string& token);
    void clearCards(httplib::Client& cli,const std::string& listId,const std::string& key,const std::string& token);
}
