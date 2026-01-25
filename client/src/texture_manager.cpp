#include "../include/texture_manager.h"
#include <iostream>

TextureManager::TextureManager(int width, int height) : cardW(width), cardH(height) {
    // czerwony prostokat, jezeli nie ma karty o podanym id
    errorTex.create(cardW, cardH);
    sf::Uint8* pixels = new sf::Uint8[cardW * cardH * 4];
    for(int i = 0; i < cardW * cardH * 4; i += 4) {
        pixels[i] = 255;   // R
        pixels[i+1] = 0;   // G
        pixels[i+2] = 0;   // B
        pixels[i+3] = 255; // A
    }
    errorTex.update(pixels);
    delete[] pixels;
}

sf::Texture& TextureManager::getTexture(int cardId) {
    if (textures.find(cardId) == textures.end()) {
        sf::Texture tex;
        std::string path = "./cards/" + std::to_string(cardId) + ".jpg";
        
        if (!tex.loadFromFile(path)) {
            std::cerr << "Warning: Could not load " << path << std::endl;
            return errorTex; 
        }
        tex.setSmooth(true);
        textures[cardId] = tex;
    }
    return textures[cardId];
}