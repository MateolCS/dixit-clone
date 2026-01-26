#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <vector>

#include "deck.h"
#include "network_manager.h"

class Game {

public:
    Game(NetworkManager* networkManager);
    ~Game();

    void addPlayer(size_t playerId);
    void removePlayer(size_t playerId);
    void startGame();
    void giveClue(size_t playerId, uint8_t cardId, const std::string& clue);
    void playCard(size_t playerId, uint8_t cardId);
    void vote(size_t playerId, uint8_t cardId);
    void update();
    bool hasPlayer(size_t playerId) const;
    bool canJoin() const;
    bool isEnded() const;
    
private:
    void dealCards();
    void startNewRound();
    void scoreRound();
    void endGame();

    struct Player {
        size_t id;
        size_t score;
        std::vector<uint8_t> hand;
    };

    struct Clue {
        std::chrono::time_point<std::chrono::steady_clock> endTime;
        std::string text;
        bool given;
    };

    struct PlayedCards {
        std::chrono::time_point<std::chrono::steady_clock> endTime;
        std::vector<std::optional<uint8_t>> cards;
        bool allPlayed;
    };

    struct Votes {
        std::chrono::time_point<std::chrono::steady_clock> endTime;
        std::vector<std::optional<uint8_t>> votes;
        bool allVoted;
    };

    struct Round {
        size_t storytellerIndex;
        Clue clue;
        PlayedCards playedCards;
        Votes votes;
        std::chrono::time_point<std::chrono::steady_clock> nexRoundStartTime;
    };

    std::vector<Player> m_players;
    std::vector<Round> m_rounds;
    bool m_gameStarted = false;
    bool m_gameEnded = false;

    Deck m_deck;
    NetworkManager* m_networkManager;

    constexpr static size_t c_maxPlayers = 5;

};