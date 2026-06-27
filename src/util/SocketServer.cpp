#include "SocketServer.h"

SocketServer::SocketServer() {
    pthread_mutex_init(&m_mutex, NULL);
}

SocketServer::~SocketServer() {
    cleanup();
}

void* SocketServer::threadCreate(void* obj) {
    return reinterpret_cast<SocketServer*>(obj)->run();
}

void SocketServer::threadKill(int sig, siginfo_t* info, void* ctx) {
    SocketServer::getInstance().stop();
}

void* SocketServer::run() {
    if (pthread_mutex_trylock(&m_mutex) != 0) {
        Log::error("Unable to lock mutex");
        return nullptr;
    }

    cleanup();

    m_listenSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_listenSocket == -1) {
        Log::error("unable to create listen socket");
        return nullptr;
    }

    memset(&m_addr, 0, sizeof(struct sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    strncpy(m_addr.sun_path, c_socketPath.c_str(), sizeof(m_addr.sun_path) - 1);

    if (bind(m_listenSocket, (const struct sockaddr*)&m_addr, sizeof(struct sockaddr_un)) == -1) {
        Log::error("unable to bind socket");
        return nullptr;
    }

    if (listen(m_listenSocket, 20) == -1) {
        Log::error("unable to listen on socket");
        return nullptr;
    }

    ready = true;
    Log::info("socket server listening on " + c_socketPath);

    char buffer[c_bufferSize];

    // Boucle : accepte une connexion, lit la commande, répond, ferme
    for (;;) {
        m_dataSocket = accept(m_listenSocket, NULL, NULL);
        if (m_dataSocket == -1) {
            Log::error("failed to accept connection");
            continue;
        }

        // Lire la commande complète
        string command;
        ssize_t n;
        while ((n = read(m_dataSocket, buffer, c_bufferSize - 1)) > 0) {
            buffer[n] = 0;
            command += buffer;
            // On arrête si on a reçu un \n (fin de commande)
            if (command.find('\n') != string::npos) break;
        }

        // Traiter et répondre
        string response = "{\"error\": \"no handler\"}";
        if (m_commandHandler && !command.empty()) {
            try {
                response = m_commandHandler(command);
            } catch (const exception& e) {
                response = string("{\"error\": \"") + e.what() + "\"}";
            }
        }

        response += "\n";
        write(m_dataSocket, response.c_str(), response.size());
        close(m_dataSocket);
        m_dataSocket = 0;
    }

    return nullptr;
}

bool SocketServer::isReady() { return ready; }

int SocketServer::writeBuf(const char* buf, int len) {
    auto ret = write(m_dataSocket, buf, len);
    if (ret == -1) { Log::error("failed to write data"); exit(EXIT_FAILURE); }
    return 0;
}

int SocketServer::writeBuf(const unsigned char* buf, int len) {
    return writeBuf((const char*)buf, len);
}

int SocketServer::writeStr(const string& str) {
    return writeBuf(str.c_str(), str.size());
}

void SocketServer::cleanup() {
    if (access(c_socketPath.c_str(), F_OK) != -1)
        unlink(c_socketPath.c_str());
}

int SocketServer::start() {
    if (pthread_create(&m_pid, NULL, &SocketServer::threadCreate, this) != 0) {
        Log::error("unable to start socket server thread");
        return false;
    }
    return true;
}

void SocketServer::stop() {
    pthread_mutex_destroy(&m_mutex);
    if (m_pid) { pthread_cancel(m_pid); m_pid = 0; }
    if (m_listenSocket) { close(m_listenSocket); m_listenSocket = 0; }
    if (m_dataSocket) { close(m_dataSocket); m_dataSocket = 0; }
    Log::info("socket server stopped");
    cleanup();
}