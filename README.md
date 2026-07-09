you can compile by doing:  

#Windows:  

g++ -O2 -std=c++20 -I include -I third_party -I third_party\nlohmann src/*.cpp -o server.exe -DCPPHTTPLIB_OPENSSL_SUPPORT -lws2_32 -lssl -lcrypto -lcrypt32  

#Linux:  

g++ -O2 -std=c++20 -I include -I third_party -I third_party/nlohmann src/*.cpp -o server -DCPPHTTPLIB_OPENSSL_SUPPORT -lssl -lcrypto -lpthread  

or using the Dockerfile.If compiled directly on your machine, and you can't inject environment variables directly you will need to hard code the trello api key,token and boardID at lines 86-88.  

Name your environment variables as follows:  
APIKEY  
APITOKEN  
BOARDID
