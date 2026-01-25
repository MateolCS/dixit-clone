#pragma once
#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <SFML/Graphics.hpp>
#include <map>
#include <string>

class TextureManager {
private:
    std::map<int, sf::Texture> textures;
    sf::Texture errorTex;
    int cardW, cardH;

public:
    TextureManager(int width, int height);
    sf::Texture& getTexture(int cardId);
};

#endif