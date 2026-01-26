#include "game.h"

#include <algorithm>
#include <random>

constexpr static std::chrono::duration c_clueTimeLimit = std::chrono::seconds(60);
constexpr static std::chrono::duration c_playCardTimeLimit = std::chrono::seconds(60);
constexpr static std::chrono::duration c_voteTimeLimit = std::chrono::seconds(60);
constexpr static std::chrono::duration c_roundResultsTime = std::chrono::seconds(5);

Game::Game(NetworkManager* networkManager) : m_networkManager(networkManager) {}
Game::~Game() {}

void Game::addPlayer(size_t playerId) {
    // sprawdzenie czy gra sie juz rozpoczela lub zakonczyla
    if (m_gameStarted || m_gameEnded) {
        m_networkManager->sendMessage ( 
            playerId,
            JoinGameResponse{ false,  0 }
        );
        return;
    }

    // sprawdzenie czy jest miejsce aby dolaczyc do gry
    if (m_players.size() >= c_maxPlayers) {
         m_networkManager->sendMessage ( 
            playerId,
            JoinGameResponse{ false,  0 }
        );
        return;
    }

    // dodanie gracza do gry
    m_players.push_back({ playerId, 0, {} });
    m_networkManager->sendMessage ( 
        playerId,
        JoinGameResponse{ true, static_cast<uint8_t>(m_players.size() - 1) }
    );

    // wystartowanie gry gdy pelny stol 
    if (m_players.size() == c_maxPlayers) {
        startGame();
    }
}

void Game::removePlayer(size_t playerId) {
    // sprawdzenie czy gra sie juz rozpoczela lub zakonczyla
    if (m_gameStarted || m_gameEnded) {
        endGame();
        return;
    }

    // usuniecie gracza z gry
    m_players.erase(
        std::remove_if(m_players.begin(), m_players.end(),
        [playerId](const Player& player) { return player.id == playerId; }),
        m_players.end()
    );
}

void Game::startGame() {
    m_gameStarted = true;
    dealCards();

    // wyslanie wiadomosci o rozpoczeciu gry do wszystkich graczy
    for (size_t i = 0; i < c_maxPlayers; i++) {
        m_networkManager->sendMessage( 
            m_players[i].id,
            GameStartedMessage{
                .playersCount = c_maxPlayers,
                .playerIndex = (uint8_t) i,
                .initialHand = m_players[i].hand
            }
        );
    }

    startNewRound();
}

void Game::giveClue(size_t playerId, uint8_t cardId, const std::string& clue) {
    // sprawdzenie czy gra sie juz rozpoczela lub zakonczyla
    if (!m_gameStarted || m_gameEnded || m_rounds.empty()) {
        m_networkManager->sendMessage( 
            playerId,
            GiveClueResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy gracz jest narratorem 
    Round& currentRound = m_rounds.back();
    auto& storyteller = m_players[currentRound.storytellerIndex];
    if (storyteller.id != playerId) {
        m_networkManager->sendMessage( 
            playerId,
            GiveClueResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy zostala juz podana wskazowka
    if (currentRound.clue.given) {
        m_networkManager->sendMessage( 
            playerId,
            GiveClueResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy karta nalezy do gracza
    auto card = std::find(
        storyteller.hand.begin(),
        storyteller.hand.end(),
        cardId
    );
    if (card == storyteller.hand.end()) {
        m_networkManager->sendMessage( 
            playerId,
            GiveClueResponse{ false }
        );
        return;
    }

    // usuniecie karty z reki gracza
    storyteller.hand.erase(card);
    m_deck.returnCard(cardId);

    currentRound.playedCards.cards[currentRound.storytellerIndex] = std::optional(cardId);
    currentRound.votes.votes[currentRound.storytellerIndex] = std::optional(currentRound.storytellerIndex);

    // zapisanie wskazowki i powiadomienie gracza o sukcesie
    currentRound.clue.text = clue;
    currentRound.clue.given = true;
    m_networkManager->sendMessage( 
        playerId,
        GiveClueResponse{ true }
    );
    
    // powiadomienie wszystkich graczy o podanej wskazowce
    auto endPhaseTime = std::chrono::steady_clock::now() + c_playCardTimeLimit;
    currentRound.playedCards.endTime = endPhaseTime;
    auto endPhaseMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endPhaseTime.time_since_epoch()).count();
    
    std::vector<uint8_t> scores;
    scores.reserve(m_players.size());
    for (const auto& player : m_players) {
        scores.push_back(player.score);
    }
    
    for (size_t i = 0; i < c_maxPlayers; i++) {
        m_networkManager->sendMessage( 
            m_players[i].id,
            GameStateUpdate{
                .playersCount = c_maxPlayers,
                .playerIndex = (uint8_t) i,
                .playerScores = scores,
                .hand = m_players[i].hand,
                .isStoryteller = currentRound.storytellerIndex == i,
                .nextPhaseTime = endPhaseMilliseconds,
                .roundState = GameStateUpdate::WaitingForCards,
                .clue = std::optional(clue),
                .playedCards = std::nullopt,
                .votes = std::nullopt
            }
        );
    }
}

void Game::playCard(size_t playerId, uint8_t cardId) {
    // sprawdzenie czy gra sie juz rozpoczela lub zakonczyla
    if (!m_gameStarted || m_gameEnded || m_rounds.empty()) {
        m_networkManager->sendMessage(
            playerId,
            PlayCardResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy pora na zagranie karty
    Round& currentRound = m_rounds.back();
    if (!currentRound.clue.given || currentRound.playedCards.allPlayed) {
        m_networkManager->sendMessage(
            playerId,
            PlayCardResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy gracz nalezy do gry
    auto it = std::find_if(
        m_players.begin(), m_players.end(),
        [playerId](const Player& player) { return player.id == playerId; }
    );
    if (it == m_players.end()) {
        m_networkManager->sendMessage(
            playerId,
            PlayCardResponse{ false }
        );
        return;
    }
    
    size_t playerIndex = std::distance(m_players.begin(), it);

    // sprawdzenie, czy gracz juz zagral karte
    if (currentRound.playedCards.cards[playerIndex].has_value()) {
        m_networkManager->sendMessage(
            playerId,
            PlayCardResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy karta nalezy do gracza
    auto card = std::find(
        m_players[playerIndex].hand.begin(),
        m_players[playerIndex].hand.end(),
        cardId
    );
    if (card == m_players[playerIndex].hand.end()) {
        m_networkManager->sendMessage(
            playerId,
            PlayCardResponse{ false }
        );
        return;
    }

    // usuniecie karty z reki gracza
    m_players[playerIndex].hand.erase(card);
    m_deck.returnCard(cardId);

    // zagranie karty i powiadomienie gracza o sukcesie
    currentRound.playedCards.cards[playerIndex] = std::optional(cardId);
    m_networkManager->sendMessage(
        playerId,
        PlayCardResponse{ true }
    );

    // sprawdzenie czy wszyscy gracze juz zagrali karty
    bool allPlayed = std::all_of(
        currentRound.playedCards.cards.begin(),
        currentRound.playedCards.cards.end(),
        [](const std::optional<size_t>& card) { return card.has_value(); }
    );

    if (allPlayed) {
        currentRound.playedCards.allPlayed = true;
        auto endPhaseTime = std::chrono::steady_clock::now() + c_voteTimeLimit;
        currentRound.votes.endTime = endPhaseTime;
        auto endPhaseMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endPhaseTime.time_since_epoch()).count();

        // powiadomienie wszystkich graczy o zagranych kartach
        std::vector<uint8_t> scores;
        scores.reserve(m_players.size());
        for (const auto& player : m_players) {
            scores.push_back(player.score);
        }

        std::vector<uint8_t> cards;
        cards.reserve(m_players.size());
        for (const auto& card : currentRound.playedCards.cards) {
            cards.push_back(card.value());
        }
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);

        for (size_t i = 0; i < c_maxPlayers; i++) {
            m_networkManager->sendMessage( 
                m_players[i].id,
                GameStateUpdate{
                    .playersCount = c_maxPlayers,
                    .playerIndex = (uint8_t) i,
                    .playerScores = scores,
                    .hand = m_players[i].hand,
                    .isStoryteller = currentRound.storytellerIndex == i,
                    .nextPhaseTime = endPhaseMilliseconds,
                    .roundState = GameStateUpdate::WaitingForVotes,
                    .clue = currentRound.clue.text,
                    .playedCards = cards,
                    .votes = std::nullopt
                }
            );
        }
    }
}

void Game::vote(size_t playerId, uint8_t cardId) {
    // sprawdzenie czy gra sie juz rozpoczela lub zakonczyla
    if (!m_gameStarted || m_gameEnded || m_rounds.empty()) {
        m_networkManager->sendMessage(
            playerId,
            VoteResponse{ false }
        );
        return;
    }

    // sprawdzenie, czy pora na glosowanie
    Round& currentRound = m_rounds.back();
    if (!currentRound.playedCards.allPlayed || currentRound.votes.allVoted) {
        m_networkManager->sendMessage(
            playerId,
            VoteResponse{ false }
        );
        return;
    }
    
    // sprawdzenie, czy gracz nalezy do gry
    auto it = std::find_if(
        m_players.begin(), m_players.end(),
        [playerId](const Player& player) { return player.id == playerId; }
    );
    if (it == m_players.end()) {
       m_networkManager->sendMessage(
            playerId,
            VoteResponse{ false }
        );
        return;
    }

    // zaakceptowanie glosu i powiadomienie gracza o sukcesie
    size_t playerIndex = std::distance(m_players.begin(), it);
    currentRound.votes.votes[playerIndex] = std::optional(cardId);
    m_networkManager->sendMessage(
        playerId,
        VoteResponse{ true }
    );

    // sprawdzenie czy wszyscy gracze juz zaglosowali
    bool allVoted = std::all_of(
        currentRound.votes.votes.begin(),
        currentRound.votes.votes.end(),
        [](const std::optional<size_t>& vote) { return vote.has_value(); }
    );

    if (allVoted) {
        currentRound.votes.allVoted = true;
        auto nextRoundTime = std::chrono::steady_clock::now() + c_roundResultsTime;
        currentRound.nexRoundStartTime = nextRoundTime;
        auto nextRoundTimeMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(nextRoundTime.time_since_epoch()).count();
        
        // podliczenie punktow i powiadomienie graczy o wynikach rundy
        scoreRound();

        std::vector<uint8_t> scores;
        scores.reserve(m_players.size());
        for (const auto& player : m_players) {
            scores.push_back(player.score);
        }

        std::vector<uint8_t> cards;
        cards.reserve(m_players.size());
        for (const auto& card : currentRound.playedCards.cards) {
            cards.push_back(card.value());
        }

        std::vector<uint8_t> votes;
        votes.reserve(m_players.size());
        for (const auto& vote : currentRound.votes.votes) {
            votes.push_back(vote.value());
        }

        for (size_t i = 0; i < c_maxPlayers; i++) {
            m_networkManager->sendMessage( 
                m_players[i].id,
                GameStateUpdate{
                    .playersCount = c_maxPlayers,
                    .playerIndex = (uint8_t) i,
                    .playerScores = scores,
                    .hand = m_players[i].hand,
                    .isStoryteller = currentRound.storytellerIndex == i,
                    .nextPhaseTime = nextRoundTimeMilliseconds,
                    .roundState = GameStateUpdate::RoundEnded,
                    .clue = currentRound.clue.text,
                    .playedCards = cards,
                    .votes = votes
                }
            );
        }
    }
}

void Game::update() {
    if (!m_gameStarted || m_gameEnded) return;
    if (m_rounds.empty()) return;
    Round& currentRound = m_rounds.back();

    auto now = std::chrono::steady_clock::now();

    // sprawdzenie czy czas na podanie wskazowki minal i czy wskazowka zostala podana
    if (!currentRound.clue.given) {
        if (now  >= currentRound.clue.endTime) {
            startNewRound();
        }
        return;
    }

    // sprawdzenie czy czas na zagranie kart minal i czy wszyscy zagrali
    if (!currentRound.playedCards.allPlayed) {
        if (now >= currentRound.playedCards.endTime) {
            // zagranie kart za graczy, ktorzy nie zagrali w czasie
            for (size_t i = 0; i < currentRound.playedCards.cards.size(); ++i) {
                if (!currentRound.playedCards.cards[i].has_value()) {
                    auto& player = m_players[i];
                    playCard(player.id, player.hand[0]);
                }
            }
        }
        return;
    }
    
    // sprawdzenie czy czas na glosowanie minal i czy wszyscy zaglosowali
    if (!currentRound.votes.allVoted) {
        if (now >= currentRound.votes.endTime) {
            endGame(); // zakonczenie gry jesli nie wszyscy zaglosowali na czas
        }
        return;
    }

    // sprawdzenie czy czas aby rozpoczac nowa runde
    if (now >= currentRound.nexRoundStartTime) {
        startNewRound();
    }
}

bool Game::hasPlayer(size_t playerId) const {
    for (const auto& player : m_players) {
        if (player.id == playerId) {
            return true;
        }
    }
    return false;
}

bool Game::canJoin() const {
    return !m_gameStarted && !m_gameEnded && m_players.size() < c_maxPlayers;
}

bool Game::isEnded() const {
    return m_gameEnded;
}

void Game::dealCards() {
    for (auto& player : m_players) {
        while (player.hand.size() < 6) {
            size_t cardId = m_deck.drawCard();
            player.hand.push_back(cardId);
        }
    }
}

void Game::startNewRound() {
    // rozdanie kart graczom
    m_deck.shuffle();
    dealCards();
    auto now = std::chrono::steady_clock::now();

    // ustawienie stanu poczatkowego rundy
    Clue clue {
        .endTime = now + c_clueTimeLimit,
        .text = "",
        .given = false
    };

    PlayedCards playedCards {
        .endTime = now,
        .cards = std::vector<std::optional<uint8_t>>(m_players.size(), std::nullopt),
        .allPlayed = false
    };

    Votes votes {
        .endTime = now,
        .votes = std::vector<std::optional<uint8_t>>(m_players.size(), std::nullopt),
        .allVoted = false
    };

    Round newRound;
    newRound.storytellerIndex = m_rounds.size() % m_players.size();
    newRound.clue = clue;
    newRound.playedCards = playedCards;
    newRound.votes = votes;
    newRound.nexRoundStartTime = now;
    m_rounds.push_back(newRound);

    // powiadomienie wszystkich graczy o rozpoczeciu nowej rundy
    auto endPhaseTime = now + c_clueTimeLimit;
    m_rounds.back().clue.endTime = endPhaseTime;
    auto endPhaseMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(endPhaseTime.time_since_epoch()).count();

    std::vector<uint8_t> scores;
    scores.reserve(m_players.size());
    for (const auto& player : m_players) {
        scores.push_back(player.score);
    }

    for (size_t i = 0; i < c_maxPlayers; i++) {
        m_networkManager->sendMessage( 
            m_players[i].id,
            GameStateUpdate{
                .playersCount = c_maxPlayers,
                .playerIndex = (uint8_t) i,
                .playerScores = scores,
                .hand = m_players[i].hand,
                .isStoryteller = m_rounds.back().storytellerIndex == i,
                .nextPhaseTime = endPhaseMilliseconds,
                .roundState = GameStateUpdate::WaitingForClue,
                .clue = std::nullopt,
                .playedCards = std::nullopt,
                .votes = std::nullopt
            }
        );
    }

}

void Game::scoreRound() {
    // sprawdzenie czy gra sie jeszcze nie rozpoczela lub zakonczyla
    if (!m_gameStarted || m_gameEnded) return;
    if (m_rounds.empty()) return;
    Round& currentRound = m_rounds.back();
    // sprawdzenie czy wszyscy gracze juz zaglosowali
    if (!currentRound.votes.allVoted) return;
    
    // podliczenie glosow na karte kazdego gracza
    std::vector<size_t> recivedVotes(m_players.size(), 0);
    for (const auto& vote : currentRound.votes.votes) {
        if (vote.has_value()) {
            recivedVotes[vote.value()]++;
        }
    }
    
    // dodanie punktow graczom gdy wszyscy lub nikt nie odgadl karty - 2 dla wszystkich, oprócz narratora
    if (recivedVotes[currentRound.storytellerIndex] == 1 ||
        recivedVotes[currentRound.storytellerIndex] == m_players.size()) {
        for (size_t i = 0; i < m_players.size(); ++i) {
            if (i != currentRound.storytellerIndex) {
                m_players[i].score += 2;
            }
        }
    } else {
        // rozdanie punktow gdy nie wszyscy odgadli karte - 3 dla narratora i 3 dla graczy, ktorzy odgadli karte, po punkcie za glos na karte gracza
        m_players[currentRound.storytellerIndex].score += 3;
        for (size_t i = 0; i < m_players.size(); i++) {
            if (i != currentRound.storytellerIndex) {
                // dodanie punktow za glosy na karte gracza
                m_players[i].score += recivedVotes[i];
                if (currentRound.votes.votes[i].value() == currentRound.storytellerIndex) {
                    m_players[i].score += 3;
                }
            }
        }
    }

    // sprawdzenie, czy ktos wygral i zakonczenie gry
    bool ended = std::find_if(
        m_players.begin(),
        m_players.end(),
        [](const Player& player) { return player.score >= 30; }
    ) != m_players.end();

    if (ended) {
        endGame();
        return;
    }
    
}

void Game::endGame() {
    if (!m_gameStarted || m_gameEnded) return;
    m_gameEnded = true;

    // wyslanie wiadomosci o zakonczeniu gry do wszystkich graczy i podanie wynikow
    std::vector<uint8_t> scores;
        scores.reserve(m_players.size());
        for (const auto& player : m_players) {
            scores.push_back(player.score);
        }

    for (size_t i = 0; i < c_maxPlayers; i++) {
        m_networkManager->sendMessage( 
            m_players[i].id,
            GameEndedMessage{
                .playersCount = c_maxPlayers,
                .playerIndex = (uint8_t) i,
                .finalScores = scores
            }
        );
    }
}
