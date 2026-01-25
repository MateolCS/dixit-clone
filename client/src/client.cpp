#include "../include/client.h"
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

// stale okna i kart
const int Client::WINDOW_W = 1000;
const int Client::WINDOW_H = 700;
const int Client::CARD_W = 100;
const int Client::CARD_H = 150;
const unsigned short SERVER_PORT = 1100;

Client::Client() 
    : window(sf::VideoMode(WINDOW_W, WINDOW_H), "Klient do gry w Dixit"), 
      texManager(CARD_W, CARD_H),
      socketFd(-1), 
      isConnected(false),
      currentState(CONNECT_MENU),
      amIStoryteller(false),
      myPlayerIndex(-1),
      currentRoundState(GameStateUpdate::RoundEnded)
{
    if (!font.loadFromFile("CONSOLA.TTF")) {
        std::cerr << "[ERROR] Nie ma czcionki CONSOLAS.TTF!" << std::endl;
    }

    connectBtn = new Button(WINDOW_W/2 - 100, WINDOW_H/2, 200, 50, "Dolacz do gry!", font);
    submitBtn = new Button(WINDOW_W - 150, WINDOW_H/2, 120, 50, "Potwierdz", font);

    statusText.setFont(font);
    statusText.setPosition(20, 20);
    statusText.setCharacterSize(22);
    
    infoText.setFont(font);
    infoText.setPosition(20, 60);
    infoText.setCharacterSize(18);
    infoText.setFillColor(sf::Color::Cyan);

    hintDisplay.setFont(font);
    hintDisplay.setCharacterSize(30);
    hintDisplay.setFillColor(sf::Color::Yellow);
    hintDisplay.setPosition(20, WINDOW_H/2 - 50);
}

Client::~Client() {
    delete connectBtn;
    delete submitBtn;
    closeSocket(); 
}


void Client::closeSocket() {
    if (socketFd != -1) {
        close(socketFd);
        socketFd = -1;
    }
    isConnected = false;
}

bool Client::setBlocking(bool blocking) {
    if (socketFd == -1) return false;
    
    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags == -1) return false;

    if (blocking) {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    return fcntl(socketFd, F_SETFL, flags) == 0;
}

bool Client::connectSocket(const std::string& ip, int port) {
 
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        perror("[ERROR] socket()");
        return false;
    }

   
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "[ERROR] Zly adres" << std::endl;
        return false;
    }

    if (connect(socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("[ERROR] connect()");
        closeSocket();
        return false;
    }

    return true;
}

bool Client::sendAll(const void* data, size_t length) {
    const char* ptr = static_cast<const char*>(data);
    size_t totalSent = 0;
    
    while (totalSent < length) {
        ssize_t sent = send(socketFd, ptr + totalSent, length - totalSent, 0);
        if (sent == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; 
            }
            perror("[ERROR] send()");
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool Client::recvAll(void* buffer, size_t length) {
    char* ptr = static_cast<char*>(buffer);
    size_t totalReceived = 0;

    while (totalReceived < length) {
        
        ssize_t received = recv(socketFd, ptr + totalReceived, length - totalReceived, 0);
        
        if (received == 0) {
            return false; 
        }
        if (received == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return false; 
        }
        totalReceived += received;
    }
    return true;
}

void Client::sendPacket(const Message& msg) {
    if (socketFd == -1) return;

    std::vector<uint8_t> data = serializeMessage(msg);
    uint32_t msgLen = static_cast<uint32_t>(data.size());
    
    if (!sendAll(&msgLen, sizeof(msgLen))) {
        closeSocket();
        return;
    }

    if (!sendAll(data.data(), data.size())) {
        closeSocket();
        return;
    }
}

void Client::run() {
    while (window.isOpen()) {
        processEvents();
        handleNetwork();
        updateUI();
        render();
    }
}

void Client::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) window.close();

        if (currentState == GAME_PLAYING && 
            currentRoundState == GameStateUpdate::WaitingForClue && 
            amIStoryteller) 
        {
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == 8) { 
                    if (!inputBuffer.empty()) inputBuffer.pop_back();
                } else if (event.text.unicode >= 32 && event.text.unicode < 128) {
                    inputBuffer += static_cast<char>(event.text.unicode);
                }
            }
        }

        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);

            if (currentState == CONNECT_MENU) {
                if (connectBtn->isClicked(mousePos)) {
                    sendJoinRequest();
                }
            }
            else if (currentState == GAME_PLAYING) {
                for (auto& c : handCards) {
                    if (c.isClicked(mousePos)) {
                        for(auto& o : handCards) o.isSelected = false;
                        c.isSelected = true;
                    }
                }
                for (auto& c : tableCards) {
                    if (c.isClicked(mousePos)) {
                        for(auto& o : tableCards) o.isSelected = false;
                        c.isSelected = true;
                    }
                }

                if (submitBtn->isClicked(mousePos)) {
                    if (currentRoundState == GameStateUpdate::WaitingForClue && amIStoryteller) sendClue();
                    else if (currentRoundState == GameStateUpdate::WaitingForCards && !amIStoryteller) sendCard();
                    else if (currentRoundState == GameStateUpdate::WaitingForVotes && !amIStoryteller) sendVote();
                }
            }
        }
    }
}

void Client::handleNetwork() {
    if (!isConnected || socketFd == -1) return;

    
    uint32_t msgLen = 0;
    
    
    ssize_t bytesRead = recv(socketFd, &msgLen, sizeof(msgLen), MSG_DONTWAIT | MSG_PEEK);

    if (bytesRead == 0) {
        std::cout << "[INFO] Rozlaczono z serwerem (EOF)." << std::endl;
        closeSocket();
        currentState = CONNECT_MENU;
        statusText.setString("Rozlaczono z serwerem.");
        return;
    }
    
    if (bytesRead < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        } else {
            perror("[ERROR] Blad sieci");
            closeSocket();
            return;
        }
    }

    
    if (bytesRead >= 4) {
       
        setBlocking(true); 
        
        if (!recvAll(&msgLen, sizeof(msgLen))) {
            closeSocket();
            return;
        }

      
        std::vector<uint8_t> buffer(msgLen);
        if (!recvAll(buffer.data(), msgLen)) {
            closeSocket();
            return;
        }

        setBlocking(false); 

       
        auto msgOpt = deserializeMesssage(buffer);
        if (msgOpt.has_value()) {
            handleMessage(msgOpt.value());
        }
    }
}

void Client::sendJoinRequest() {
    window.clear(sf::Color(30, 30, 30));
    statusText.setString("Laczenie z serwerem...");
    
    sf::FloatRect textRect = statusText.getLocalBounds();
    statusText.setOrigin(textRect.left + textRect.width/2.0f, textRect.top + textRect.height/2.0f);
    statusText.setPosition(WINDOW_W/2.0f, WINDOW_H/2.0f);
    
    window.draw(statusText);
    window.display(); 

    
    if (!connectSocket("127.0.0.1", SERVER_PORT)) {
        statusText.setString("Nie udalo sie polaczyc z serwerem.");
        statusText.setOrigin(0, 0);
        statusText.setPosition(20, 20);
        return;
    }

    isConnected = true;
    std::cout << "[INFO] Polaczono TCP. Dolaczanie do gry" << std::endl;

   
    sendPacket(Message(JoinGameRequest{}));

    window.clear(sf::Color(30, 30, 30));
    statusText.setString("Czekam na serwer...");
    window.draw(statusText);
    window.display();

    
    struct pollfd pfd;
    pfd.fd = socketFd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, 3000); 

    if (ret > 0) {
        if (pfd.revents & POLLIN) {
            uint32_t msgLen = 0;
            if (recvAll(&msgLen, sizeof(msgLen))) {
                std::vector<uint8_t> buffer(msgLen);
                if (recvAll(buffer.data(), msgLen)) {
                    auto msgOpt = deserializeMesssage(buffer);
                    if (msgOpt.has_value()) {
                        std::cout << "[INFO] Otrzymano odpowiedz! Typ: " << (int)msgOpt->type() << std::endl;
                        handleMessage(msgOpt.value()); 
                    }
                }
            }
        }
    } else if (ret == 0) {
        std::cerr << "[TIMEOUT] Brak odpowiedzi od serwera." << std::endl;
        statusText.setString("Server Timeout.");
        closeSocket();
    } else {
        perror("[ERROR] poll failed");
        closeSocket();
    }

    if (isConnected) {
        setBlocking(false);
    }

    statusText.setOrigin(0, 0);
    statusText.setPosition(20, 20);
}

void Client::handleMessage(const Message& msg) {
    switch (msg.type()) {
        case Message::GiveClueResponseType: {
            const auto& payload = std::get<GiveClueResponse>(msg.payload);
            if (payload.success) {
                std::cout << "[INFO] Podpowiedz zaakceptowana." << std::endl;
                statusText.setString("Wskazowka wyslana.");
            } else {
                statusText.setString("Blad! Wskazowka odrzucona.");
            }
            break;
        }

        case Message::PlayCardResponseType: {
            const auto& payload = std::get<PlayCardResponse>(msg.payload);
            if (payload.success) {
                std::cout << "[INFO] Karta zaakceptowana." << std::endl;
                statusText.setString("Karta wybrana");
            } else {
                statusText.setString("Blad! Karta odrzucona.");
            }
            break;
        }

        case Message::VoteResponseType: {
            const auto& payload = std::get<VoteResponse>(msg.payload);
            if (payload.success) {
                std::cout << "[INFO] Glos przyjeto." << std::endl;
                statusText.setString("Zaglosowano pomyslnie.");
            } else {
                statusText.setString("Blad! Glos odrzucony.");
            }
            break;
        }

        case Message::JoinGameResponseType: {
            const auto& payload = std::get<JoinGameResponse>(msg.payload);
            if (payload.success) {
                currentState = LOBBY;
                myPlayerIndex = payload.playerIndex;
                statusText.setString("W lobby. Czekanie na graczy...");
                std::cout << "[INFO] Dolaczono do Lobby. Moj ID: " << (int)myPlayerIndex << std::endl;
            } else {
                statusText.setString("Niedolaczono. Sprobuj ponownie.");
                closeSocket();
            }
            break;
        }
        
        case Message::GameStartedMessageType: {
            const auto& payload = std::get<GameStartedMessage>(msg.payload);
            currentState = GAME_PLAYING;
            myPlayerIndex = payload.playerIndex;
            std::cout << "[INFO] Start gry!" << std::endl;
            
            amIStoryteller = (myPlayerIndex == 0); 
            currentRoundState = GameStateUpdate::WaitingForClue; 

            handCards.clear();
            int i = 0;
            for (uint8_t cardId : payload.initialHand) {
                handCards.emplace_back(cardId, 50 + (i * 120), WINDOW_H - 180, 
                                       texManager.getTexture(cardId), CARD_W, CARD_H);
                i++;
            }
            updateUI();
            break;
        }

        case Message::GameStateUpdateType: {
            const auto& update = std::get<GameStateUpdate>(msg.payload);
            
            std::cout << "[INFO] Nowy stan gry: " << (int)update.roundState << std::endl;

            currentRoundState = update.roundState;
            amIStoryteller = update.isStoryteller;
            
            handCards.clear();
            int hIdx = 0;
            for (uint8_t cardId : update.hand) {
                CardUI card(cardId, 50 + (hIdx * 120), WINDOW_H - 180, 
                            texManager.getTexture(cardId), CARD_W, CARD_H);
                
                if (currentRoundState == GameStateUpdate::WaitingForVotes) {
                    card.sprite.setColor(sf::Color(100, 100, 100)); 
                }
                
                handCards.push_back(card);
                hIdx++;
            }

            tableCards.clear();
            if (update.playedCards.has_value()) {
                int tIdx = 0;
                for (uint8_t cardId : update.playedCards.value()) {
                    tableCards.emplace_back(cardId, 150 + (tIdx * 120), 100, 
                                            texManager.getTexture(cardId), CARD_W, CARD_H);
                    tIdx++;
                }
            }

            if (update.clue.has_value()) currentClue = update.clue.value();
            else currentClue = "";

            std::string scoresStr = "Wyniki: ";
            for (size_t i = 0; i < update.playerScores.size(); ++i) {
                scoresStr += "P" + std::to_string(i) + ":" + std::to_string(update.playerScores[i]) + " ";
            }
            infoText.setString(scoresStr);
            
            updateUI();
            break;
        }

        case Message::GameEndedMessageType: {
            const auto& payload = std::get<GameEndedMessage>(msg.payload);
            currentState = CONNECT_MENU;
            
            std::string winnerText = "Koniec gry! Wyniki:\n";
            for(size_t i=0; i<payload.finalScores.size(); i++) {
                winnerText += "P" + std::to_string(i) + ": " + std::to_string(payload.finalScores[i]) + "\n";
            }
            
            statusText.setString(winnerText);
            closeSocket();
            break;
        }
        
        default: break;
    }
}

void Client::sendClue() {
    uint8_t selectedCardId = 0;
    bool found = false;
    for (auto& c : handCards) {
        if (c.isSelected) {
            selectedCardId = static_cast<uint8_t>(c.id);
            found = true;
            break;
        }
    }

    if (found && !inputBuffer.empty()) {
        setBlocking(true);
        sendPacket(Message(GiveClueRequest{ selectedCardId, inputBuffer }));
        setBlocking(false);
        
        inputBuffer.clear();
        std::cout << "[INFO] Wyslano podpowiedz." << std::endl;
    }
}

void Client::sendCard() {
    uint8_t selectedCardId = 0;
    bool found = false;
    for (auto& c : handCards) {
        if (c.isSelected) {
            selectedCardId = static_cast<uint8_t>(c.id);
            found = true;
            break;
        }
    }

    if (found) {
        setBlocking(true);
        sendPacket(Message(PlayCardRequest{ selectedCardId }));
        setBlocking(false);
        std::cout << "[INFO] Wyslano karte." << std::endl;
    }
}

void Client::sendVote() {
    uint8_t selectedCardId = 0;
    bool found = false;
    for (auto& c : tableCards) {
        if (c.isSelected) {
            selectedCardId = static_cast<uint8_t>(c.id);
            found = true;
            break;
        }
    }

    if (found) {
        setBlocking(true);
        sendPacket(Message(VoteRequest{ selectedCardId }));
        setBlocking(false);
        std::cout << "[INFO] Wyslano glos." << std::endl;
    }
}

void Client::updateUI() {
    if (currentState == GAME_PLAYING) {
        std::string status;
        
        switch (currentRoundState) {
            case GameStateUpdate::WaitingForClue:
                if (amIStoryteller) status = "Twoja tura, wybierz karte i wskazowke:  " + inputBuffer;
                else status = "Oczekiwanie na podpowiedz";
                break;
                
            case GameStateUpdate::WaitingForCards:
                status = "Wskazowka: '" + currentClue + "'. Wybierz karte!";
                if (amIStoryteller) status = "Oczekiwanie na wybor kart...";
                break;
                
            case GameStateUpdate::WaitingForVotes:
                if (amIStoryteller) status = "Inni glosuja...";
                else status = "zaglosuj na karte! Wskazowka: " + currentClue;
                break;
                
            case GameStateUpdate::RoundEnded:
                status = "Koniec rundy. Zaktualizowano wynik.";
                break;
        }
        statusText.setString(status);
        hintDisplay.setString(amIStoryteller && currentRoundState == GameStateUpdate::WaitingForClue ? inputBuffer : currentClue);
    }
}

void Client::render() {
    window.clear(sf::Color(30, 30, 30));

    if (currentState == CONNECT_MENU) {
        connectBtn->draw(window);
        window.draw(statusText);
    }
    else if (currentState == LOBBY) {
        statusText.setString("Kolejka: Czekamy na graczy...");
        window.draw(statusText);
    }
    else if (currentState == GAME_PLAYING) {
        for(auto& c : tableCards) c.draw(window);
        for(auto& c : handCards) c.draw(window);

        window.draw(statusText);
        window.draw(infoText);
        
        bool showSubmit = false;
        if (currentRoundState == GameStateUpdate::WaitingForClue && amIStoryteller) showSubmit = true;
        if (currentRoundState == GameStateUpdate::WaitingForCards && !amIStoryteller) showSubmit = true;
        if (currentRoundState == GameStateUpdate::WaitingForVotes && !amIStoryteller) showSubmit = true;

        if (showSubmit) submitBtn->draw(window);
    }

    window.display();
}