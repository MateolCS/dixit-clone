#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <cstdlib>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

struct JoinGameRequest {};

struct JoinGameResponse {
    bool success;
    uint8_t playerIndex;
};

struct GiveClueRequest {
    uint8_t cardId;
    std::string clue;
};

struct GiveClueResponse {
    bool success;
};

struct PlayCardRequest {
    uint8_t cardId;
};

struct PlayCardResponse {
    bool success;
};

struct VoteRequest {
    uint8_t cardId;
};

struct VoteResponse {
    bool success;
};

struct GameStartedMessage {
    uint8_t playersCount;
    uint8_t playerIndex;
    std::vector<uint8_t> initialHand;
};

struct GameStateUpdate {
    uint8_t playersCount;
    uint8_t playerIndex;
    std::vector<uint8_t> playerScores;
    std::vector<uint8_t> hand;
    bool isStoryteller;
    int64_t nextPhaseTime;

    enum RoundState : uint8_t {
        WaitingForClue = 0x00,
        WaitingForCards = 0x01,
        WaitingForVotes = 0x02,
        RoundEnded = 0x03,
    } roundState;

    std::optional<std::string> clue;
    std::optional<std::vector<uint8_t>> playedCards;
    std::optional<std::vector<uint8_t>> votes;
};

struct GameEndedMessage {
    uint8_t playersCount;
    uint8_t playerIndex;
    std::vector<uint8_t> finalScores;
};

struct Message {
    enum MessageType : uint8_t {
        JoinGameRequestType = 0x01,
        JoinGameResponseType = 0x02,
        GiveClueRequestType = 0x03,
        GiveClueResponseType = 0x04,
        PlayCardRequestType = 0x05,
        PlayCardResponseType = 0x06,
        VoteRequestType = 0x07,
        VoteResponseType = 0x08,
        GameStartedMessageType = 0x09,
        GameStateUpdateType = 0x0A,
        GameEndedMessageType = 0x0B,
    };

    using Payload = std::variant<
        JoinGameRequest,
        JoinGameResponse,
        GiveClueRequest,
        GiveClueResponse,
        PlayCardRequest,
        PlayCardResponse,
        VoteRequest,
        VoteResponse,
        GameStartedMessage,
        GameStateUpdate,
        GameEndedMessage
    >;

    Payload payload;

    Message() = delete;
    Message(JoinGameRequest v) : payload(std::move(v)) {}
    Message(JoinGameResponse v) : payload(std::move(v)) {}
    Message(GiveClueRequest v) : payload(std::move(v)) {}
    Message(GiveClueResponse v) : payload(std::move(v)) {}
    Message(PlayCardRequest v) : payload(std::move(v)) {}
    Message(PlayCardResponse v) : payload(std::move(v)) {}
    Message(VoteRequest v) : payload(std::move(v)) {}
    Message(VoteResponse v) : payload(std::move(v)) {}
    Message(GameStartedMessage v) : payload(std::move(v)) {}
    Message(GameStateUpdate v) : payload(std::move(v)) {}
    Message(GameEndedMessage v) : payload(std::move(v)) {}

    MessageType type() const {
        return std::visit(
            [](const auto& v) -> MessageType {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, JoinGameRequest>) return JoinGameRequestType;
                else if constexpr (std::is_same_v<T, JoinGameResponse>) return JoinGameResponseType;
                else if constexpr (std::is_same_v<T, GiveClueRequest>) return GiveClueRequestType;
                else if constexpr (std::is_same_v<T, GiveClueResponse>) return GiveClueResponseType;
                else if constexpr (std::is_same_v<T, PlayCardRequest>) return PlayCardRequestType;
                else if constexpr (std::is_same_v<T, PlayCardResponse>) return PlayCardResponseType;
                else if constexpr (std::is_same_v<T, VoteRequest>) return VoteRequestType;
                else if constexpr (std::is_same_v<T, VoteResponse>) return VoteResponseType;
                else if constexpr (std::is_same_v<T, GameStartedMessage>) return GameStartedMessageType;
                else if constexpr (std::is_same_v<T, GameStateUpdate>) return GameStateUpdateType;
                else if constexpr (std::is_same_v<T, GameEndedMessage>) return GameEndedMessageType;
                else { std::terminate(); }
            },
            payload
        );
    }
};
