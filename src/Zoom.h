#ifndef MEETING_SDK_LINUX_SAMPLE_ZOOM_H
#define MEETING_SDK_LINUX_SAMPLE_ZOOM_H

#include <iostream>
#include <chrono>
#include <string>
#include <sstream>

#include <jwt-cpp/jwt.h>

#include "Config.h"
#include "util/Singleton.h"
#include "util/Log.h"
#include "util/SocketServer.h"
#include "WaitingRoomEvent.h"

#include "zoom_sdk.h"
#include "rawdata/zoom_rawdata_api.h"
#include "rawdata/rawdata_renderer_interface.h"

#include "meeting_service_components/meeting_audio_interface.h"
#include "meeting_service_components/meeting_participants_ctrl_interface.h"
#include "meeting_service_components/meeting_video_interface.h"
#include "meeting_service_components/meeting_waiting_room_interface.h"
#include "setting_service_interface.h"

#include "events/AuthServiceEvent.h"
#include "events/MeetingServiceEvent.h"
#include "events/MeetingReminderEvent.h"
#include "events/MeetingRecordingCtrlEvent.h"

#include "raw_record/ZoomSDKRendererDelegate.h"
#include "raw_record/ZoomSDKAudioRawDataDelegate.h"

#include "raw_send/ZoomSDKVideoSource.h"

using namespace std;
using namespace jwt;
using namespace ZOOMSDK;

typedef chrono::time_point<chrono::system_clock> time_point;

class Zoom : public Singleton<Zoom> {

    friend class Singleton<Zoom>;

    Config m_config;

    string m_jwt;

    time_point m_iat;
    time_point m_exp;

    IMeetingService*              m_meetingService        = nullptr;
    ISettingService*              m_settingService        = nullptr;
    IAuthService*                 m_authService           = nullptr;
    IMeetingWaitingRoomController* m_waitingRoomController = nullptr;
    WaitingRoomEvent*             m_waitingRoomEvent      = nullptr;

    IZoomSDKRenderer*             m_videoHelper           = nullptr;
    ZoomSDKRendererDelegate*      m_renderDelegate        = nullptr;

    IZoomSDKAudioRawDataHelper*   m_audioHelper           = nullptr;
    ZoomSDKAudioRawDataDelegate*  m_audioSource           = nullptr;

    ZoomSDKVideoSource*           m_videoSource           = nullptr;

    SDKError createServices();
    void generateJWT(const string& key, const string& secret);

    /**
     * Callback fired when the SDK authenticates the credentials
     */
    function<void()> onAuth = [&]() {
        auto e = isMeetingStart() ? start() : join();
        string action = isMeetingStart() ? "start" : "join";
        if (hasError(e, action + " a meeting")) exit(e);
    };

    /**
     * Callback fired when the app joins the meeting
     */
    function<void()> onJoin = [&]() {
        // ── Reminder controller (existant) ──
        auto* reminderController = m_meetingService->GetMeetingReminderController();
        reminderController->SetEvent(new MeetingReminderEvent());

        // ── Waiting room controller (nouveau) ──
        m_waitingRoomController = m_meetingService->GetMeetingWaitingRoomController();
        if (m_waitingRoomController) {
            m_waitingRoomEvent = new WaitingRoomEvent();
            m_waitingRoomController->SetEvent(m_waitingRoomEvent);
            Log::success("waiting room controller ready");
        } else {
            Log::error("failed to get waiting room controller");
        }

        // ── Socket server (nouveau) ──
        auto& socketServer = SocketServer::getInstance();
        socketServer.setCommandHandler([this](const string& cmd) {
            return handleSocketCommand(cmd);
        });
        socketServer.start();

        // ── Raw recording (existant) ──
        if (!m_config.useRawRecording())
            return;

        function<void(bool)> onRecordingPrivilegeChanged = [&](bool canRec) {
            if (!canRec) {
                Log::error("Failed to get recording privilege");
                return;
            }
            startRawRecording();
        };

        auto recCtl = m_meetingService->GetMeetingRecordingController();
        auto recordingEvent = new MeetingRecordingCtrlEvent(onRecordingPrivilegeChanged);
        recCtl->SetEvent(recordingEvent);

        SDKError err = recCtl->CanStartRawRecording();
        if (hasError(err)) {
            Log::info("requesting local recording privilege");
            recCtl->RequestLocalRecordingPrivilege();
        }
    };

public:
    Zoom() {};

    SDKError init();
    SDKError auth();
    SDKError config(int ac, char** av);

    SDKError join();
    SDKError start();
    SDKError leave();
    SDKError clean();

    SDKError startRawRecording();
    SDKError stopRawRecording();

    bool isMeetingStart();

    // ── Waiting room & socket commands (nouveau) ──
    string handleSocketCommand(const string& json);
    string listParticipants();
    string listWaitingRoom();
    string admitUser(unsigned int userID);
    string putInWaitingRoom(unsigned int userID);

    static bool hasError(SDKError e, const string& action = "");
};

#endif //MEETING_SDK_LINUX_SAMPLE_ZOOM_H