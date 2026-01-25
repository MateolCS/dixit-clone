#include "../include/card_ui.h"

CardUI::CardUI(int cardId, float x, float y, sf::Texture& texture, int width, int height) 
    : id(cardId), isSelected(false) 
{
    sprite.setTexture(texture);
    sprite.setPosition(x, y);

    sf::Vector2u texSize = texture.getSize();
    if (texSize.x > 0 && texSize.y > 0) {
        sprite.setScale(
            (float)width / texSize.x,
            (float)height / texSize.y
        );
    }

    highlight.setPosition(x, y);
    highlight.setSize(sf::Vector2f(width, height));
    highlight.setFillColor(sf::Color::Transparent);
    highlight.setOutlineThickness(4);
    highlight.setOutlineColor(sf::Color::Cyan);
}

bool CardUI::isClicked(sf::Vector2i mousePos) {
    return sprite.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
}

void CardUI::draw(sf::RenderWindow& window) {
    window.draw(sprite);
    if (isSelected) window.draw(highlight);
}