#ifndef SAMA_WAITINGROOMEVENT_H
#define SAMA_WAITINGROOMEVENT_H

#include <functional>
#include <string>
#include "meeting_service_components/meeting_audio_interface.h" 
#include "meeting_service_components/meeting_participants_ctrl_interface.h" 
#include "meeting_service_components/meeting_waiting_room_interface.h"
#include "util/Log.h"

using namespace std;
using namespace ZOOMSDK;

class WaitingRoomEvent : public IMeetingWaitingRoomEvent {
    function<void(unsigned int)> m_onUserJoin;
    function<void(unsigned int)> m_onUserLeft;

public:
    void onWaitingRoomUserJoin(unsigned int userID) override {
        Log::info("waiting room join: " + to_string(userID));
        if (m_onUserJoin) m_onUserJoin(userID);
    }

    void onWaitingRoomUserLeft(unsigned int userID) override {
        Log::info("waiting room left: " + to_string(userID));
        if (m_onUserLeft) m_onUserLeft(userID);
    }

    // Callbacks obligatoires — non utilisés pour l'instant
    void onWaitingRoomPresetAudioStatusChanged(bool bAudioCanTurnOn) override {}
    void onWaitingRoomPresetVideoStatusChanged(bool bVideoCanTurnOn) override {}
    void onWaitingRoomUserNameChanged(unsigned int userID, const zchar_t* userName) override {}
    void onWaitingRoomEntranceEnabled(bool bIsEnabled) override {}
    void onCustomWaitingRoomDataUpdated(CustomWaitingRoomData& bData, IWaitingRoomDataDownloadHandler* bHandler) override {}

    void setOnUserJoin(const function<void(unsigned int)>& cb) { m_onUserJoin = cb; }
    void setOnUserLeft(const function<void(unsigned int)>& cb) { m_onUserLeft = cb; }
};

#endif