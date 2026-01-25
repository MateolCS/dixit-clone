#pragma once
#ifndef CARD_UI_H
#define CARD_UI_H

#include <SFML/Graphics.hpp>

class CardUI {
public:
    int id;
    sf::Sprite sprite;
    sf::RectangleShape highlight;
    bool isSelected;

    CardUI(int cardId, float x, float y, sf::Texture& texture, int width, int height);
    bool isClicked(sf::Vector2i mousePos);
    void draw(sf::RenderWindow& window);
};

#endif