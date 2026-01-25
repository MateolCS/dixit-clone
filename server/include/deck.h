#pragma once

#include <vector>

class Deck {

public:
    Deck();
    ~Deck();

    void shuffle();
    uint8_t drawCard();
    void returnCard(uint8_t cardId);

private:
    std::vector<uint8_t> cards;
    
};
