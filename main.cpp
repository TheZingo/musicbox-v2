#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <filesystem>
#include <csignal>
#include "WebHandler.h"
#include "ContentHandler.h"
#include "RFIDHandler.h"
#include "LEDHandler.h"
#include "MusicHandler.h"
#include "TagHandler.h"
#include "ButtonHandler.h"
#include "BatteryHandler.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <random> 
#include "State.h"

static const std::filesystem::path CONFIG_CONTENT_DIRECTORY = "content";

RFIDHandler rfidHandler;
LEDHandler ledHandler;
ContentHandler contentHandler;
TagHandler tagHandler;
MusicHandler musicHandler;
WebHandler webHandler;    
MusicBox::State state;
ButtonHandler buttonHandler(false, 50);
BatteryHandler batteryHandler;

void setContent(unsigned& seed, MusicBox::TagData& tagHandle) {
    std::optional<MusicBox::ContentItem> content = contentHandler.getContentItem(tagHandle.contentRef);
    if(!content.has_value()) {
        std::cout << "no content item found for this tag " << tagHandle.contentRef << std::endl;                
    } else if(content->files.size() ==0) {
        std::cout << "content item defined no files to play for this tag " << tagHandle.contentRef << std::endl;             
    } else {
        std::vector<std::string> elementList;
        for(const std::string& element : content->files) {
            elementList.push_back(CONFIG_CONTENT_DIRECTORY / content->id / element);
        }
        if(tagHandle.randomize) {                    
            if(tagHandle.savePlayState) {
                seed = tagHandle.playState.seed;
            }                    
            std::mt19937 generator(seed);
            std::shuffle(elementList.begin(), elementList.end(), generator);
        }
        musicHandler.setContent(elementList);
        // restore playstate
        if(tagHandle.savePlayState) {
            musicHandler.setIndex(tagHandle.playState.index);
            musicHandler.setPosition(tagHandle.playState.position);
        }

        musicHandler.startPlay();
        state.UpdatePlayState(true);
        webHandler.sendMessage(nlohmann::json{{"event", "play"}}.dump());
        ledHandler.ok();
    }
}

// Signal handler function
void handle_sigterm(int signal_num) {
    std::cout << "Received SIGTERM, cleaning up and exiting..." << std::endl;
    ledHandler.clear();
    musicHandler.stopPlay();    
    exit(signal_num); 
}

bool isAudioBook() {
    MusicBox::StateData currentState = state.GetState();
    if(currentState.activeTag.has_value()) {
        std::optional<MusicBox::ContentItem> content = contentHandler.getContentItem(currentState.activeTag->contentRef);
        if(content.has_value()) {
            if ( content.value().type == "hoerbuch" || 
                 content.value().type == "hoerspiel" || 
                 content.value().type == "book" || 
                 content.value().type == "audiobook" || 
                 content.value().type == "podcast" || 
                 content.value().type == "audiobuch" || 
                 content.value().type == "buch") {
                return true;
            }
        }
            
    }
    return false;
}

int main(int count, char** args) {
    std::cout << "Music Box 2 0.0.1" << std::endl;

    struct sigaction sa;
    sa.sa_handler = handle_sigterm;
    sigfillset(&sa.sa_mask);

    ledHandler.init();

    buttonHandler.registerButton(23, []() { 
        std::cout << "Increase volume by 5%" << std::endl; 
        const char* cmd = "wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+ -l 1.0";
        system(cmd) == 0;
    });
    buttonHandler.registerButton(17, []() { 
        std::cout << "Decrease volume by 5%" << std::endl; 
        const char* cmd = "wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%- -l 1.0";
        system(cmd) == 0;
    });
    buttonHandler.registerButton(27, []() { 
        std::cout << "Forward" << std::endl; 
        if(isAudioBook()) {
            musicHandler.seekInPlaylist(30);
        } else {
            musicHandler.forward();
        }
    });
    buttonHandler.registerButton(22, []() { 
        std::cout << "Backward" << std::endl; 
        if(isAudioBook()) {
            musicHandler.seekInPlaylist(-30);
        } else {
            musicHandler.backward();
        }        
    });
    buttonHandler.start();
    batteryHandler.startMonitoring();

    contentHandler.readContentDirectory(CONFIG_CONTENT_DIRECTORY);
    webHandler.setContentHandler(&contentHandler);
    webHandler.setMusicHandler(&musicHandler);
    webHandler.setBatteryHandler(&batteryHandler);
    webHandler.setStateInstance(&state);
    webHandler.startWebinterface();
    
    // register signals here (startWebInterface will remove the signals handlers that are used in the webserver implementation)
    sigaction(SIGTERM, &sa, NULL);
    
    while(true) {
        std::string tag = rfidHandler.waitForTag();
        unsigned seed = time(nullptr);
        std::cout << "Tag: " << tag << std::endl;
        std::optional<MusicBox::TagData> tagHandle = tagHandler.GetTagData(tag);        
        if(tagHandle.has_value()) {
            state.UpdateTag(tagHandle.value());
            std::cout << "found tag: " << tagHandle->name << std::endl;
        
            // send out web socket message for new tag
            auto event = nlohmann::json{{"event", "tag_found"}, {"id", tag}};
            nlohmann::json j = tagHandle.value();
            event.push_back({"tagData", j});
            webHandler.sendMessage(event.dump());

            // set content
            setContent(seed, tagHandle.value());

        } else {
            auto event = nlohmann::json{{"event", "tag_unknown"}, {"id", tag}};
            webHandler.sendMessage(event.dump());
            ledHandler.error();
        }
        
        rfidHandler.waitForTagRemoved();
        webHandler.sendMessage(nlohmann::json{{"event", "tag_removed"}, {"id", tag}}.dump());
        ledHandler.clear();
        musicHandler.stopPlay();
        state.UpdatePlayState(false);
        tagHandle = state.GetState().activeTag;
        if(tagHandle.has_value()) {
            if(tagHandle->savePlayState == true) {
                tagHandle->playState.index = musicHandler.getIndex();
                tagHandle->playState.position = musicHandler.getPosition();
                tagHandle->playState.seed = seed;
            }
            tagHandle->playCount = tagHandle->playCount + 1;
            auto const now = std::chrono::system_clock::now();
            std::time_t newt = std::chrono::system_clock::to_time_t(now);
            tagHandle->lastSeen = newt;
            tagHandler.SetTagData(tagHandle.value());            
        }
        state.UpdateTag(std::nullopt);
        webHandler.sendMessage(nlohmann::json{{"event", "stop"}}.dump());
    }    
    
    ledHandler.clear();
    buttonHandler.stop();
    musicHandler.stopPlay();
    webHandler.stopWebinterface();
    batteryHandler.endMonitoring();
    std::cout << "Done in Main...." << std::endl << std::flush;
}
