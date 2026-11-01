#include "Engine/Resources/Framebuffer.h"

#include "Engine/Core/Log.h"

#include <glad/glad.h>

Framebuffer::~Framebuffer()
{
    Release();
}

void Framebuffer::Release()
{
    if (fbo != 0) glDeleteFramebuffers(1, &fbo);
    if (colorTexture != 0) glDeleteTextures(1, &colorTexture);
    if (depthRenderbuffer != 0) glDeleteRenderbuffers(1, &depthRenderbuffer);
    fbo = colorTexture = depthRenderbuffer = 0;
}

void Framebuffer::Resize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0 || (newWidth == width && newHeight == height && fbo != 0))
        return;

    Release();
    width = newWidth;
    height = newHeight;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    glGenRenderbuffers(1, &depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        LOG_ERROR("Scene framebuffer is not complete");

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::Bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void Framebuffer::Unbind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
