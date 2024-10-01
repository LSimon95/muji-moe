/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */
#include "texManager.hpp"
#include <iostream>
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
#include "../plat.hpp"

CTexManager::CTexManager()
{
}

CTexManager::~CTexManager()
{
    ReleaseTextures();
}

CTexManager::TextureInfo* CTexManager::CreateTextureFromPngFile(std::string fileName, bool isPreMultiplyAlpha)
{
    //search loaded texture already.
    for (Csm::csmUint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i]->fileName == fileName)
        {
            return m_textures[i];
        }
    }

    GLuint textureId;
    int width, height, channels;
    unsigned int size;
    unsigned char* png;
    unsigned char* address;

    address = CPlat::LoadFileAsBytes(fileName, &size);

    png = stbi_load_from_memory(
        address,
        static_cast<int>(size),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha);

    if (isPreMultiplyAlpha)
    {
        unsigned int* fourBytes = reinterpret_cast<unsigned int*>(png);
        for (int i = 0; i < width * height; i++)
        {
            unsigned char* p = png + i * 4;
            fourBytes[i] = Premultiply(p[0], p[1], p[2], p[3]);
        }
    }

    // OpenGL用のテクスチャを生成する
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, png);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // 解放処理
    stbi_image_free(png);
    CPlat::ReleaseBytes(address);

    CTexManager::TextureInfo* textureInfo = new CTexManager::TextureInfo();
    if (textureInfo != NULL)
    {
        textureInfo->fileName = fileName;
        textureInfo->width = width;
        textureInfo->height = height;
        textureInfo->id = textureId;

        m_textures.PushBack(textureInfo);
    }

    return textureInfo;

}

void CTexManager::ReleaseTextures()
{
    for (Csm::csmUint32 i = 0; i < m_textures.GetSize(); i++)
    {
        delete m_textures[i];
    }

    m_textures.Clear();
}

void CTexManager::ReleaseTexture(Csm::csmUint32 textureId)
{
    for (Csm::csmUint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i]->id != textureId)
        {
            continue;
        }
        delete m_textures[i];
        m_textures.Remove(i);
        break;
    }
}

void CTexManager::ReleaseTexture(std::string fileName)
{
    for (Csm::csmUint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i]->fileName == fileName)
        {
            delete m_textures[i];
            m_textures.Remove(i);
            break;
        }
    }
}

CTexManager::TextureInfo* CTexManager::GetTextureInfoById(GLuint textureId) const
{
    for (Csm::csmUint32 i = 0; i < m_textures.GetSize(); i++)
    {
        if (m_textures[i]->id == textureId)
        {
            return m_textures[i];
        }
    }

    return NULL;
}
