#pragma once

#include "network_manager.h"
#include "game.h"

class DixitServer {

public:
    DixitServer();
    ~DixitServer();

    void run();

private:
    NetworkManager m_networkManager;
    std::vector<Game> m_games;
    
};