#ifndef INCLUDE_BUTTONHANDLER_H_
#define INCLUDE_BUTTONHANDLER_H_

#include <array>
#include <functional>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <mutex>

class ButtonHandler
{

public:
    using ButtonCallback = std::function<void()>;

    struct ButtonState
    {
        uint8_t pin;
        ButtonCallback callback;
        uint8_t lastState;
        uint64_t lastDebounceTime;
        bool stableState;
    };

    ButtonHandler(bool init = true, uint64_t debounceDelay = 50);
    ~ButtonHandler();
    void registerButton(uint8_t pin, ButtonCallback callback);
    void check();
    void checkLoop();
    void start();
    void stop();


private:
    static constexpr size_t MAX_BUTTONS = 4;
    std::array<ButtonState, MAX_BUTTONS> buttons;
    size_t numButtons = 0;
    uint64_t debounceDelay;
    std::atomic<bool> running;
    std::thread checkThread;
    std::mutex buttonMutex;
};

#endif