#ifndef INCLUDE_STATE_H
#define INCLUDE_STATE_H

#include <string>
#include <optional>
#include <mutex>
#include "TagData.h"
#include <nlohmann/json.hpp>

namespace MusicBox {

struct StateData {
    std::optional<TagData> activeTag;
    bool playing;
    std::string activeTrack;
    std::string contentRef;
    float batteryPercentage;
    bool batteryCharging;
};

class State {
    private:
        StateData state;
        std::mutex mutex;

    public:
        void UpdateTag(std::optional<TagData> tagdata);
        void UpdateContentRef(std::string contentId);
        void UpdateActiveTrack(std::string track);
        void UpdatePlayState(bool playState);
        void UpdateBatteryState(float percentage, bool charging);
        StateData GetState();
};

inline void to_json(nlohmann::json& j, const MusicBox::StateData& p) {
    TagData defaultTag;
   j = nlohmann::json{ 
    {"activeTag", p.activeTag.value_or(defaultTag)}, 
    {"playing", p.playing}, 
    {"activeTrack", p.activeTrack}, 
    {"contentRef", p.contentRef}, 
    {"batteryPercentage", p.batteryPercentage},
    {"batteryCharging", p.batteryCharging}

    };
}

}
#endif  //  INCLUDE_STATE_H