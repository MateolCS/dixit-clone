#include "deck.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <unistd.h>

constexpr uint8_t c_totalCards = 84;

Deck::Deck() {
    for (size_t i = 0; i < c_totalCards; ++i) {
        cards.push_back(i);
    }
}

Deck::~Deck() {}

void Deck::shuffle() {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(cards.begin(), cards.end(), g);
}

uint8_t Deck::drawCard() {
    if (cards.empty()) {
        throw std::runtime_error("No more cards in the deck");
    }
    uint8_t cardId = cards.back();
    cards.pop_back();
    return cardId;
}

void Deck::returnCard(uint8_t cardId) {
    cards.emplace(cards.begin(), cardId);
}
