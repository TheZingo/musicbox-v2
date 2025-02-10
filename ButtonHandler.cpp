#include "ButtonHandler.h"
#include "bcm2835.h"
#include <chrono>

ButtonHandler::ButtonHandler(bool init, uint64_t delay) : debounceDelay(delay)
{
    if (init && !bcm2835_init())
    {
        throw std::runtime_error("Failed to initialize bcm2835");
    }
}

ButtonHandler::~ButtonHandler()
{
    stop();
    bcm2835_close();
}

void ButtonHandler::registerButton(uint8_t pin, ButtonCallback callback)
{
    std::lock_guard<std::mutex> lock(buttonMutex);
    if (numButtons >= MAX_BUTTONS)
    {
        throw std::runtime_error("Maximum number of buttons reached");
    }

    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(pin, BCM2835_GPIO_PUD_UP);

    ButtonState state{
        .pin = pin,
        .callback = callback,
        .lastState = HIGH,
        .lastDebounceTime = 0,
        .stableState = HIGH};

    buttons[numButtons++] = state;
}

void ButtonHandler::check()
{
    std::lock_guard<std::mutex> lock(buttonMutex);
    uint64_t currentTime = bcm2835_st_read() / 1000; // Convert to milliseconds

    for (size_t i = 0; i < numButtons; i++)
    {
        uint8_t reading = bcm2835_gpio_lev(buttons[i].pin);

        if (reading != buttons[i].lastState)
        {
            buttons[i].lastDebounceTime = currentTime;
        }

        if ((currentTime - buttons[i].lastDebounceTime) > debounceDelay)
        {
            if (reading != buttons[i].stableState)
            {
                buttons[i].stableState = reading;
                if (buttons[i].stableState == LOW && buttons[i].callback)
                {
                    buttons[i].callback();
                }
            }
        }

        buttons[i].lastState = reading;
    }
}

void ButtonHandler::checkLoop() {
        while (running) {
            check();
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
    }


void ButtonHandler::start() {
        if (!running) {
            running = true;
            checkThread = std::thread(&ButtonHandler::checkLoop, this);
        }
    }

    void ButtonHandler::stop() {
        if (running) {
            running = false;
            if (checkThread.joinable()) {
                checkThread.join();
            }
        }
    }