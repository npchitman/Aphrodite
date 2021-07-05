//
// Created by npchitman on 6/21/21.
//

#ifndef Aphrodite_ENGINE_OPENGLTEXTURE_H
#define Aphrodite_ENGINE_OPENGLTEXTURE_H

#include <glad/glad.h>

#include "Aphrodite/Renderer/Texture.h"

namespace Aph {
    class OpenGLTexture2D : public Texture2D {
    public:
        explicit OpenGLTexture2D(uint32_t width, uint32_t height);
        explicit OpenGLTexture2D(const std::string& path);
        ~OpenGLTexture2D() override;

        uint32_t GetWidth() const override { return m_Width; }
        uint32_t GetHeight() const override { return m_Height; }
        uint32_t GetRendererID() const override { return m_Renderer; }

        void SetData(void* data, uint32_t size) override;
        void Bind(uint32_t slot) const override;

        bool operator==(const Texture& other) const override { return m_RendererID == ((OpenGLTexture2D&) other).m_RendererID; }

    private:
        std::string m_Path;
        uint32_t m_Width, m_Height;
        uint32_t m_Renderer{}, m_RendererID{};
        GLenum m_InternalFormat, m_DataFormat;
    };
}// namespace Aph


#endif//Aphrodite_ENGINE_OPENGLTEXTURE_H
