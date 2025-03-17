#include "OpenGLTexture.hpp"

#include "OpenGLException.hpp"
#include "Utils/Global.hpp"

namespace OpenGL
{

namespace Detail
{

constexpr GLuint noId{0};

bool isCreated(GLuint id) noexcept;

inline bool isCreated(GLuint id) noexcept { return static_cast<bool>(id); }

} // namespace Detail

OpenGLTexture::OpenGLTexture()
    : id_{Detail::noId}, format_{0}, height_{0}, width_{0}, mipmapCount_{0},
      minificationFilter_{Filter::Nearest},
      magnificationFilter_{Filter::Linear}, wrapOption_{WrapOption::Repeat}
{
}

OpenGLTexture::OpenGLTexture(GLsizei width, GLsizei height, GLenum format,
                             const std::vector<unsigned char> &buffer,
                             Filter minificationFilter,
                             Filter magnificationFilter, WrapOption wrapOption)
    : id_{0}, format_{format}, height_{height}, width_{width},
      minificationFilter_{minificationFilter},
      magnificationFilter_{magnificationFilter}, wrapOption_{wrapOption}
{
    create();

    bind();

    bindBuffer(buffer);
}

OpenGLTexture::OpenGLTexture(OpenGLTexture &&other) noexcept
    : id_{std::move(other.id_)}, format_{std::move(other.format_)},
      height_{std::move(other.height_)}, width_{std::move(other.width_)},
      minificationFilter_{std::move(other.minificationFilter_)},
      magnificationFilter_{std::move(other.magnificationFilter_)},
      wrapOption_{std::move(other.wrapOption_)}
{
    other.id_ = Detail::noId; // Avoid double deletion
}

OpenGLTexture &OpenGLTexture::operator=(OpenGLTexture &&other) noexcept
{
    if (this != &other)
    {
        if (Detail::isCreated(id_))
        {
            tidy();
        }

        id_ = std::move(other.id_);
        format_ = std::move(other.format_);
        height_ = std::move(other.height_);
        width_ = std::move(other.width_);
        mipmapCount_ = std::move(other.mipmapCount_);
        minificationFilter_ = std::move(other.minificationFilter_);
        magnificationFilter_ = std::move(other.magnificationFilter_);
        wrapOption_ = std::move(other.wrapOption_);

        other.id_ = Detail::noId; // Avoid double deletion
    }

    return *this;
}

OpenGLTexture::~OpenGLTexture()
{
    if (Detail::isCreated(id_))
    {
        tidy();
    }
}

void OpenGLTexture::bind()
{
    PROGRAM_ASSERT(Detail::isCreated(id_));

    glBindTexture(GL_TEXTURE_2D, id_);
}

void OpenGLTexture::bindBuffer(const std::vector<unsigned char> &buffer) const
{
    glTexImage2D(GL_TEXTURE_2D, 0, format_, width_, height_, 0, format_,
                 GL_UNSIGNED_BYTE, buffer.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minificationFilter_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magnificationFilter_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapOption_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapOption_);

    glGenerateMipmap(GL_TEXTURE_2D);
}

void OpenGLTexture::create()
{
    PROGRAM_ASSERT(!Detail::isCreated(id_));

    glGenTextures(1, &id_);

    if (!Detail::isCreated(id_))
    {
        throw OpenGLException(
            "OpenGLTexture instantiate failed at 'glGenTextures'.");
    }
}

GLenum OpenGLTexture::format() const { return format_; }

GLsizei OpenGLTexture::height() const { return height_; }

GLuint OpenGLTexture::id() const { return id_; }

OpenGLTexture::Filter OpenGLTexture::magnificationFilter() const
{
    return magnificationFilter_;
}

OpenGLTexture::Filter OpenGLTexture::minificationFilter() const
{
    return minificationFilter_;
}

void OpenGLTexture::release()
{
    PROGRAM_ASSERT(Detail::isCreated(id_));

    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLTexture::setMagnificationFilter(Filter filter)
{
    PROGRAM_ASSERT(Detail::isCreated(id_));
    magnificationFilter_ = filter;

    bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    static_cast<GLenum>(magnificationFilter_));

    release();
}

void OpenGLTexture::setMinificationFilter(Filter filter)
{
    PROGRAM_ASSERT(Detail::isCreated(id_));
    minificationFilter_ = filter;

    bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    static_cast<GLenum>(minificationFilter_));

    release();
}

void OpenGLTexture::setWrapOption(WrapOption option)
{
    PROGRAM_ASSERT(Detail::isCreated(id_));
    wrapOption_ = option;

    bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                    static_cast<GLenum>(option));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                    static_cast<GLenum>(option));

    release();
}

void OpenGLTexture::tidy()
{
    PROGRAM_ASSERT(Detail::isCreated(id_));

    glDeleteTextures(1, &id_);

    id_ = 0;
}

GLsizei OpenGLTexture::width() const { return width_; }

OpenGLTexture::WrapOption OpenGLTexture::wrapOption() const
{
    return wrapOption_;
}

} // namespace OpenGL
