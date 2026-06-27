#ifndef MEETINGSDK_HEADLESS_LINUX_SAMPLE_SOCKETSERVER_H
#define MEETINGSDK_HEADLESS_LINUX_SAMPLE_SOCKETSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <iostream>
#include <functional>
#include <string>

#include "Singleton.h"
#include "Log.h"

using namespace std;

class SocketServer : public Singleton<SocketServer> {
    friend class Singleton<SocketServer>;

    const string c_socketPath = "/tmp/meeting.sock";
    const int c_bufferSize = 4096;  // augmenté pour JSON

    struct sockaddr_un m_addr;

    int m_listenSocket = 0;
    int m_dataSocket = 0;

    pthread_t m_pid;
    pthread_mutex_t m_mutex;

    bool ready = false;

    // Handler de commandes — branché depuis Zoom.cpp
    function<string(const string&)> m_commandHandler;

    void* run();
    static void* threadCreate(void* obj);
    static void threadKill(int sig, siginfo_t* info, void* ctx);

public:
    SocketServer();
    ~SocketServer();

    int start();
    void stop();
    void cleanup();
    bool isReady();

    // Écriture vers le client (conservé pour compatibilité)
    int writeBuf(const unsigned char* buf, int len);
    int writeBuf(const char* buf, int len);
    int writeStr(const string& str);

    // Enregistre le handler qui reçoit une commande JSON et retourne une réponse JSON
    void setCommandHandler(const function<string(const string&)>& handler) {
        m_commandHandler = handler;
    }
};

#endif