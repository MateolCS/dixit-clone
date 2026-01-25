#ifndef CLIENT_H
#define CLIENT_H

#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>

#include "texture_manager.h"
#include "button.h"
#include "card_ui.h"
#include "message.h"
#include "serializer.h"

class Client {
private:
    static const int WINDOW_W;
    static const int WINDOW_H;
    static const int CARD_W;
    static const int CARD_H;

    sf::RenderWindow window;
    sf::Font font;
    TextureManager texManager;
    
    
    int socketFd; 
    bool isConnected;
    std::vector<uint8_t> netBuffer; 

    
    enum State { CONNECT_MENU, LOBBY, GAME_PLAYING };
    State currentState;
    
    bool amIStoryteller;
    int myPlayerIndex;
    GameStateUpdate::RoundState currentRoundState;
    std::string currentClue;
    std::string inputBuffer;
    
    // UI 
    Button* connectBtn;
    Button* submitBtn;
    sf::Text statusText;
    sf::Text infoText;
    sf::Text hintDisplay;

    std::vector<CardUI> handCards;
    std::vector<CardUI> tableCards;

public:
    Client();
    ~Client();
    void run();

private:
    void processEvents();
    void handleNetwork(); // Odbieranie danych
    void updateUI();
    void render();

    // Helpers
    void sendJoinRequest();
    void sendClue();
    void sendCard();
    void sendVote();
    
    void handleMessage(const Message& msg);


    void sendPacket(const Message& msg);
    bool setBlocking(bool blocking);
    bool connectSocket(const std::string& ip, int port);
    void closeSocket();
   
    bool sendAll(const void* data, size_t length);
  
    bool recvAll(void* buffer, size_t length);
};

#endif