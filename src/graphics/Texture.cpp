#include "Texture.h"

#include <glad/glad.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    std::wstring utf8ToWide(const std::string& input)
    {
        if (input.empty())
            return {};

        const int size = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
        if (size <= 0)
            throw std::runtime_error("Failed to convert texture path to UTF-16");

        std::wstring result(static_cast<size_t>(size), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, result.data(), size);
        result.pop_back();
        return result;
    }

    struct ComInit
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

        ~ComInit()
        {
            if (SUCCEEDED(hr))
                CoUninitialize();
        }
    };
}

Texture::Texture(const std::string& path)
{
    ComInit com;
    if (FAILED(com.hr) && com.hr != RPC_E_CHANGED_MODE)
        throw std::runtime_error("Failed to initialize COM for texture loading");

    IWICImagingFactory* factory = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* frame = nullptr;
    IWICFormatConverter* converter = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));

    if (FAILED(hr))
        throw std::runtime_error("Failed to create WIC imaging factory");

    const std::wstring widePath = utf8ToWide(path);

    hr = factory->CreateDecoderFromFilename(
        widePath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnLoad,
        &decoder);

    if (SUCCEEDED(hr))
        hr = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(hr))
        hr = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(hr))
    {
        hr = converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);
    }

    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(hr))
        hr = converter->GetSize(&width, &height);

    std::vector<unsigned char> pixels;
    if (SUCCEEDED(hr))
    {
        pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);
        hr = converter->CopyPixels(
            nullptr,
            width * 4u,
            static_cast<UINT>(pixels.size()),
            pixels.data());
    }

    if (converter) converter->Release();
    if (frame) frame->Release();
    if (decoder) decoder->Release();
    if (factory) factory->Release();

    if (FAILED(hr))
        throw std::runtime_error("Failed to decode texture: " + path);

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::~Texture()
{
    reset();
}

Texture::Texture(Texture&& other) noexcept
    : ID(other.ID)
{
    other.ID = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        reset();
        ID = other.ID;
        other.ID = 0;
    }
    return *this;
}

bool Texture::isLoaded() const
{
    return ID != 0;
}

void Texture::bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}

void Texture::reset()
{
    if (ID != 0)
    {
        glDeleteTextures(1, &ID);
        ID = 0;
    }
}
