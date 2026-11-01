#pragma once

// Off-screen render target (color texture + depth/stencil renderbuffer).
// The editor shows the color texture inside the Scene window.
class Framebuffer
{
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void Resize(int width, int height);
    void Bind() const;
    static void Unbind();

    unsigned int GetColorTexture() const { return colorTexture; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

private:
    void Release();

    unsigned int fbo = 0;
    unsigned int colorTexture = 0;
    unsigned int depthRenderbuffer = 0;
    int width = 0;
    int height = 0;
};
