#include "../include/button.h"

Button::Button(float x, float y, float w, float h, std::string text, sf::Font& font) {
    shape.setPosition(x, y);
    shape.setSize(sf::Vector2f(w, h));
    shape.setFillColor(sf::Color(60, 60, 60));
    shape.setOutlineThickness(2);
    shape.setOutlineColor(sf::Color::White);

    label.setFont(font);
    label.setString(text);
    label.setCharacterSize(20);
    label.setFillColor(sf::Color::White);
    
    centerText();
}

void Button::centerText() {
    sf::FloatRect r = label.getLocalBounds();
    label.setOrigin(r.left + r.width/2.0f, r.top + r.height/2.0f);
    label.setPosition(
        shape.getPosition().x + shape.getSize().x/2.0f,
        shape.getPosition().y + shape.getSize().y/2.0f
    );
}

bool Button::isClicked(sf::Vector2i mousePos) {
    return shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
}

void Button::draw(sf::RenderWindow& window) {
    window.draw(shape);
    window.draw(label);
}