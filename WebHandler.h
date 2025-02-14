#ifndef INCLUDE_WEBHANDLER_H_
#define INCLUDE_WEBHANDLER_H_

#include <thread>
#include <unordered_set>
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <string>
#include "ContentHandler.h"
#include "MusicHandler.h"
#include "TagHandler.h"
#include "BatteryHandler.h"
#include "State.h"

class WebHandler {

    private:
        std::future<void> webserver;
        crow::App<crow::CORSHandler> app;
        ContentHandler* contentHandler;
        MusicHandler* musicHandler;
        TagHandler* tagHandler;
        BatteryHandler* batteryHandler;
        std::mutex mutexConnectedClients;
        std::unordered_set<crow::websocket::connection*> connectedClients;
        MusicBox::State* applicationState;

        crow::response upload(const crow::request& request);
        void handleWebsocketData (crow::websocket::connection& , const std::string& data, bool is_binary);
        std::string sanitizeFileName(const std::string& fileName) const;

    public:
        WebHandler() = default;
        ~WebHandler();
        void startWebinterface();
        void stopWebinterface();
        void setContentHandler(ContentHandler* cHandler);
        void setMusicHandler(MusicHandler* mHandler);
        void setTagHandler(TagHandler* tHandler);
        void setBatteryHandler(BatteryHandler* bHandler);
        void sendMessage(const std::string msg);
        void setStateInstance(MusicBox::State* state);
};

#endif