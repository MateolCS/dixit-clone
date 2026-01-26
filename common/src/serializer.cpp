#include "serializer.h"

// serializacja wiadomosci
std::vector<uint8_t> serializeMessage(const Message& msg) {
    std::vector<uint8_t> data;
    data.push_back(static_cast<uint8_t>(msg.type()));

    // okreslenie typu wiadomosci i serializacja w zaleznosci od typu
    switch (msg.type()) {
        case Message::JoinGameRequestType:
            break;

        case Message::JoinGameResponseType:
            {
                const auto& payload = std::get<JoinGameResponse>(msg.payload);
                data.push_back(payload.success ? 0x01 : 0x00);
                data.push_back(payload.playerIndex);
            }
            break;

        case Message::GiveClueRequestType:
            {
                const auto& payload = std::get<GiveClueRequest>(msg.payload);
                data.push_back(payload.cardId);

                const auto& clue = payload.clue;
                uint8_t clueSize = clue.size() > 0xFF ? 0xFF : static_cast<uint8_t>(clue.size());
                data.push_back(clueSize);
                data.insert(data.end(), clue.begin(), clue.begin() + clueSize);
                //data.back() = '\0'; // zapewnienie zakoncznenia nullem
            }
            break;

        case Message::GiveClueResponseType:
            {
                const auto& payload = std::get<GiveClueResponse>(msg.payload);
                data.push_back(payload.success ? 0x01 : 0x00);
            }
            break;

        case Message::PlayCardRequestType:
            {
                const auto& payload = std::get<PlayCardRequest>(msg.payload);
                data.push_back(payload.cardId);
            }
            break;

        case Message::PlayCardResponseType:
            {
                const auto& payload = std::get<PlayCardResponse>(msg.payload);
                data.push_back(payload.success ? 0x01 : 0x00);
            }
            break;

        case Message::VoteRequestType:
            {
                const auto& payload = std::get<VoteRequest>(msg.payload);
                data.push_back(payload.cardId);
            }
            break;

        case Message::VoteResponseType:
            {
                const auto& payload = std::get<VoteResponse>(msg.payload);
                data.push_back(payload.success ? 0x01 : 0x00);
            }
            break;

        case Message::GameStartedMessageType:
            {
                const auto& payload = std::get<GameStartedMessage>(msg.payload);
                data.push_back(payload.playersCount);
                data.push_back(payload.playerIndex);

                const auto& hand = payload.initialHand;
                uint8_t handSize = hand.size() > 0xFF ? 0xFF : static_cast<uint8_t>(hand.size());
                data.push_back(handSize);
                data.insert(data.end(), hand.begin(), hand.begin() + handSize);
            }
            break;

        case Message::GameStateUpdateType:
            {
                const auto& payload = std::get<GameStateUpdate>(msg.payload);
                data.push_back(payload.playersCount);
                data.push_back(payload.playerIndex);

                uint8_t playersCount = payload.playersCount;

                // serializacja punktacji graczy
                auto scores = payload.playerScores;
                // zapewnienie, że wektor punktacji ma odpowiedni rozmiar
                scores.resize(playersCount, 0);
                data.insert(data.end(), scores.begin(), scores.end());
                
                const auto& hand = payload.hand;
                uint8_t handSize = hand.size() > 0xFF ? 0xFF : static_cast<uint8_t>(hand.size());
                data.push_back(handSize);
                data.insert(data.end(), hand.begin(), hand.begin() + handSize);

                data.push_back(payload.isStoryteller ? 0x01 : 0x00);

                for (uint8_t offset = 0; offset < 8; ++offset) {
                    data.push_back((payload.nextPhaseTime >> (offset * 8)) & 0xFF);
                }

                data.push_back(static_cast<uint8_t>(payload.roundState));
                
                auto clue = payload.clue;
                if (clue.has_value()) {
                    data.push_back(0x01);
                    uint8_t clueSize = clue->size() > 0xFF ? 0xFF : static_cast<uint8_t>(clue->size());
                    data.push_back(clueSize);
                    data.insert(data.end(), clue->begin(), clue->begin() + clueSize);
                    //data.back() = '\0'; // zapewnienie zakoncznenia nullem
                } else {
                    data.push_back(0x00);
                }

                auto playedCards = payload.playedCards;
                if (playedCards.has_value()) {
                    data.push_back(0x01);
                    uint8_t cardsSize = playedCards->size() > 0xFF ? 0xFF : static_cast<uint8_t>(playedCards->size());
                    data.push_back(cardsSize);
                    data.insert(data.end(), playedCards->begin(), playedCards->begin() + cardsSize);
                } else {
                    data.push_back(0x00);
                }

                auto votes = payload.votes;
                if (votes.has_value()) {
                    data.push_back(0x01);
                    uint8_t votesSize = votes->size() > 0xFF ? 0xFF : static_cast<uint8_t>(votes->size());
                    data.push_back(votesSize);
                    data.insert(data.end(), votes->begin(), votes->begin() + votesSize);
                } else {
                    data.push_back(0x00);
                }
            }
            break;

        case Message::GameEndedMessageType:
            {
                const auto& payload = std::get<GameEndedMessage>(msg.payload);
                data.push_back(payload.playersCount);
                data.push_back(payload.playerIndex);

                uint8_t playersCount = payload.playersCount;
                auto scores = payload.finalScores;
                // zapewnienie, że wektor punktacji ma odpowiedni rozmiar
                scores.resize(playersCount, 0);
                data.insert(data.end(), scores.begin(), scores.end());
            }
            break;
    }

    return data;
}

std::optional<Message> deserializeMesssage(const std::vector<uint8_t>& data) {
    if (data.empty()) return std::nullopt;
    size_t index = 0;
    
    // odczytanie typu wiadomosci
    uint8_t type = data[index++];

    // deserializacja w zaleznosci od typu wiadomosci
    switch (type) {
        case Message::JoinGameRequestType:
            {
                return Message{ JoinGameRequest{} };
            }
            break;

        case Message::JoinGameResponseType:
            {
                if (data.size() < index + 1) return std::nullopt;
                bool success = data[index++] != 0x00;

                if (data.size() < index + 1) return std::nullopt;
                uint8_t playerIndex = data[index++];

                return Message{ JoinGameResponse{ success, playerIndex } };
            }
            break;

        case Message::GiveClueRequestType:
            {
                if (data.size() < index + 1) return std::nullopt;
                uint8_t cardId = data[index++];

                if (data.size() < index + 1) return std::nullopt;
                uint8_t clueSize = data[index++];
                if (data.size() < index + clueSize) return std::nullopt;
                std::string clue(reinterpret_cast<const char*>(&data[index]), clueSize);
                index += clueSize;

                return Message{ GiveClueRequest{ cardId, clue } };
            }
            break;

        case Message::GiveClueResponseType:
            {
                if (data.size() < index + 1) return std::nullopt;
                bool success = data[index++] != 0x00;

                return Message{ GiveClueResponse{ success } };
            }
            break;

        case Message::PlayCardRequestType:
            {
                if (data.size() < index + 1) return std::nullopt;
                uint8_t cardId = data[index++];

                return Message{ PlayCardRequest{ cardId } };
            }
            break;

        case Message::PlayCardResponseType:
            {
                if (data.size() < index + 1) return std::nullopt;
                bool success = data[index++] != 0x00;

                return Message{ PlayCardResponse{ success } };
            }
            break;

        case Message::VoteRequestType:
            {
                if (data.size() < index + 1) return std::nullopt;
                uint8_t cardId = data[index++];

                return Message{ VoteRequest{ cardId } };
            }
            break;

        case Message::VoteResponseType:
            {
                if (data.size() < index + 1) return std::nullopt;
                bool success = data[index++] != 0x00;

                return Message{ VoteResponse{ success } };
            }
            break;

        case Message::GameStartedMessageType:
            {
                if (data.size() < index + 1) return std::nullopt;
                uint8_t playersCount = data[index++];

                if (data.size() < index + 1) return std::nullopt;
                uint8_t playerIndex = data[index++];

                if (data.size() < index + 1) return std::nullopt;
                uint8_t handSize = data[index++];
                if (data.size() < index + handSize) return std::nullopt;
                std::vector<uint8_t> initialHand(data.begin() + index, data.begin() + index + handSize);
                index += handSize;

                return Message{ GameStartedMessage{ playersCount, playerIndex, initialHand } };
            }
            break;

        case Message::GameStateUpdateType:
            {
                if (data.size() < index + 1) return std::nullopt;
                uint8_t playersCount = data[index++];

                if (data.size() < index + 1) return std::nullopt;
                uint8_t playerIndex = data[index++];

                if (data.size() < index + playersCount) return std::nullopt;
                std::vector<uint8_t> playerScores(data.begin() + index, data.begin() + index + playersCount);
                index += playersCount;

                if (data.size() < index + 1) return std::nullopt;
                uint8_t handSize = data[index++];
                if (data.size() < index + handSize) return std::nullopt;
                std::vector<uint8_t> hand(data.begin() + index, data.begin() + index + handSize);
                index += handSize;

                if (data.size() < index + 1) return std::nullopt;
                bool isStoryteller = data[index++] != 0x00;

                if (data.size() < index + 8) return std::nullopt;
                int64_t nextPhaseTime = 0;
                for (uint8_t offset = 0; offset < 8; ++offset) {
                    nextPhaseTime |= static_cast<int64_t>(data[index++]) << (offset * 8);
                }

                if (data.size() < index + 1) return std::nullopt;
                uint8_t roundStateVal = data[index++];
                switch (roundStateVal) {
                    case GameStateUpdate::WaitingForClue:
                    case GameStateUpdate::WaitingForCards:
                    case GameStateUpdate::WaitingForVotes:
                    case GameStateUpdate::RoundEnded:
                        break;
                    default:
                        return std::nullopt;
                }
                GameStateUpdate::RoundState roundState = static_cast<GameStateUpdate::RoundState>(roundStateVal);

                std::optional<std::string> clue;
                if (data.size() < index + 1) return std::nullopt;
                uint8_t hasClue = data[index++];
                if (hasClue != 0x00) {
                    if (data.size() < index + 1) return std::nullopt;
                    uint8_t clueSize = data[index++];
                    if (data.size() < index + clueSize) return std::nullopt;
                    clue = std::string(reinterpret_cast<const char*>(&data[index]), clueSize);
                    index += clueSize;
                }

                std::optional<std::vector<uint8_t>> playedCards;
                if (data.size() < index + 1) return std::nullopt;
                uint8_t hasPlayedCards = data[index++];
                if (hasPlayedCards != 0x00) {
                    if (data.size() < index + 1) return std::nullopt;
                    uint8_t cardsSize = data[index++];
                    if (data.size() < index + cardsSize) return std::nullopt;
                    playedCards = std::vector<uint8_t>(data.begin() + index, data.begin() + index + cardsSize);
                    index += cardsSize;
                }

                std::optional<std::vector<uint8_t>> votes;
                if (data.size() < index + 1) return std::nullopt;
                uint8_t hasVotes = data[index++];
                if (hasVotes != 0x00) {
                    if (data.size() < index + 1) return std::nullopt;
                    uint8_t votesSize = data[index++];
                    if (data.size() < index + votesSize) return std::nullopt;
                    votes = std::vector<uint8_t>(data.begin() + index, data.begin() + index + votesSize);
                    index += votesSize;
                }

                return Message{ GameStateUpdate{
                    playersCount,
                    playerIndex,
                    playerScores,
                    hand,
                    isStoryteller,
                    nextPhaseTime,
                    roundState,
                    clue,
                    playedCards,
                    votes,
                } };
            }
            break;

        case Message::GameEndedMessageType:
            {
                if (data.size() < index + 1) return std::nullopt;
                uint8_t playersCount = data[index++];

                if (data.size() < index + 1) return std::nullopt;
                uint8_t playerIndex = data[index++];

                if (data.size() < index + playersCount) return std::nullopt;
                std::vector<uint8_t> finalScores(data.begin() + index, data.begin() + index + playersCount);
                index += playersCount;

                return Message{ GameEndedMessage{ playersCount, playerIndex, finalScores } };
            }
            break;
    }

    return std::nullopt;
}