#include "dixit_server.h"

DixitServer::DixitServer() {}

DixitServer::~DixitServer() {}

void DixitServer::run() {
    m_networkManager.initialize();
    
    bool running = true;
    while (running) {
        auto events = m_networkManager.poolMessages();
        for (const auto& event : events) {
            size_t playerId = event.senderId;
            Message message = event.message;
            
            switch (message.type()) {
                case Message::JoinGameRequestType:
                    {
                        auto it = std::find_if(
                            m_games.begin(),
                            m_games.end(),
                            [playerId](const Game& game) {
                                return game.hasPlayer(playerId);
                            }
                        );
                        
                        if (it != m_games.end()) {
                            m_networkManager.sendMessage( 
                                playerId,
                                JoinGameResponse{ false, 0 }
                            );
                            break;
                        }

                        if (m_games.empty() || !m_games.back().canJoin()) {
                            m_games.push_back(Game(&m_networkManager));
                        }

                        m_games.back().addPlayer(playerId);
                    }
                    break;

                case Message::GiveClueRequestType:
                    {
                        auto payload = std::get<GiveClueRequest>(message.payload);

                        auto it = std::find_if(
                            m_games.begin(),
                            m_games.end(),
                            [playerId](const Game& game) {
                                return game.hasPlayer(playerId);
                            }
                        );

                        if (it == m_games.end()) {
                            m_networkManager.sendMessage( 
                                playerId,
                                GiveClueResponse{ false }
                            );
                            break;
                        }

                        it->giveClue(
                            playerId,
                            payload.cardId,
                            payload.clue
                        );
                    }
                    break;
                
                case Message::PlayCardRequestType:
                    {
                        auto payload = std::get<PlayCardRequest>(message.payload);

                        auto it = std::find_if(
                            m_games.begin(),
                            m_games.end(),
                            [playerId](const Game& game) {
                                return game.hasPlayer(playerId);
                            }
                        );

                        if (it == m_games.end()) {
                            m_networkManager.sendMessage( 
                                playerId,
                                PlayCardResponse{ false }
                            );
                            break;
                        }

                        it->playCard(
                            playerId,
                            payload.cardId
                        );
                    }
                    break;

                case Message::VoteRequestType:
                    {
                        auto payload = std::get<VoteRequest>(message.payload);

                        auto it = std::find_if(
                            m_games.begin(),
                            m_games.end(),
                            [playerId](const Game& game) {
                                return game.hasPlayer(playerId);
                            }
                        );

                        if (it == m_games.end()) {
                            m_networkManager.sendMessage( 
                                playerId,
                                VoteResponse{ false }
                            );
                            break;
                        }

                        it->vote(
                            playerId,
                            payload.cardId
                        );
                    }
                    break;
                
                default:
                    break;
            }
        }

        // usuwanie zakoncznych gier
        m_games.erase(
            std::remove_if(
                m_games.begin(),
                m_games.end(),
                [](const Game& game) {
                    return game.isEnded();
                }
            ),
            m_games.end()
        );

        // aktualizacja wszystkich gier
        for (auto& game : m_games) {
            game.update();
        }
    }

    m_networkManager.shutdown();
}
