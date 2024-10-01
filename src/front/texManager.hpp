/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Type/csmVector.hpp>

class CTexManager
{
public:

    struct TextureInfo
    {
        GLuint id;
        int width;
        int height;
        std::string fileName;
    };

    CTexManager();
    ~CTexManager();

    inline unsigned int Premultiply(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
    {
        return static_cast<unsigned>(\
            (red * (alpha + 1) >> 8) | \
            ((green * (alpha + 1) >> 8) << 8) | \
            ((blue * (alpha + 1) >> 8) << 16) | \
            (((alpha)) << 24)   \
            );
    }

    TextureInfo*CreateTextureFromPngFile(std::string fileName, bool isPreMultiplyAlpha=false);

    void ReleaseTextures();
    void ReleaseTexture(Csm::csmUint32 textureId);
    void ReleaseTexture(std::string fileName);
    TextureInfo* GetTextureInfoById(GLuint textureId) const;

private:
    Csm::csmVector<TextureInfo*> m_textures;
};
