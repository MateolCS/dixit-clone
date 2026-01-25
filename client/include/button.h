#pragma once
#ifndef BUTTON_H
#define BUTTON_H

#include <SFML/Graphics.hpp>
#include <string>

class Button {
public:
    sf::RectangleShape shape;
    sf::Text label;

    Button(float x, float y, float w, float h, std::string text, sf::Font& font);
    bool isClicked(sf::Vector2i mousePos);
    void draw(sf::RenderWindow& window);

private:
    void centerText();
};

#endif