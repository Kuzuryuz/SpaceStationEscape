#pragma once

#include <string>

class Texture
{
public:
    Texture() = default;
    explicit Texture(const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool isLoaded() const;
    void bind(unsigned int slot = 0) const;

private:
    void reset();

    unsigned int ID = 0;
};
