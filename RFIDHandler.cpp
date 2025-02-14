#include "RFIDHandler.h"
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include <thread>
#include <chrono>

RFIDHandler::RFIDHandler() {
    mfrc.PCD_Init();
}


std::string getUID(MFRC522& mfrc) {
    std::stringstream uid;
    uid << std::uppercase << std::setfill('0') << std::setw(2) << std::hex;
    for (MFRC522Types::byte i = 0; i < mfrc.uid.size; ++i) {
        uid << static_cast<unsigned int>(mfrc.uid.uidByte[i]);
    }
    return uid.str();
}


std::string RFIDHandler::waitForTag() {

    while (true) {
        //   Look for a card
        if (!mfrc.PICC_IsNewCardPresent()) { 
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (!mfrc.PICC_ReadCardSerial()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        std::string uid = getUID(mfrc);
                
        return uid;
    }

    return "";
}

void RFIDHandler::waitForTagRemoved() {
    int control = 0;
    while (true) {
        control = 0;
        for (int i = 0; i < 3; i++) {
            if (!mfrc.PICC_IsNewCardPresent()) {
                if (mfrc.PICC_ReadCardSerial()) {
                    control |= 0x16;
                }
                if (mfrc.PICC_ReadCardSerial()) {
                    control |= 0x16;
                }
                control += 0x1;
            }
            control += 0x4;
        }
        if (control == 13 || control == 14) {
            // tag is still there
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } else {
            break;
        }
    }
}