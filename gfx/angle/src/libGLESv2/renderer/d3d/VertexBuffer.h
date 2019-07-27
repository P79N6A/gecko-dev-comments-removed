








#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER_H_

#include "common/angleutils.h"

#include <GLES2/gl2.h>

#include <cstddef>
#include <vector>

namespace gl
{
struct VertexAttribute;
struct VertexAttribCurrentValueData;
}

namespace rx
{
class Renderer;

class VertexBuffer
{
  public:
    VertexBuffer();
    virtual ~VertexBuffer();

    virtual bool initialize(unsigned int size, bool dynamicUsage) = 0;

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                       GLint start, GLsizei count, GLsizei instances, unsigned int offset) = 0;
    virtual bool getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count, GLsizei instances,
                                  unsigned int *outSpaceRequired) const = 0;

    virtual unsigned int getBufferSize() const = 0;
    virtual bool setBufferSize(unsigned int size) = 0;
    virtual bool discard() = 0;

    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

class VertexBufferInterface
{
  public:
    VertexBufferInterface(rx::Renderer *renderer, bool dynamic);
    virtual ~VertexBufferInterface();

    bool reserveVertexSpace(const gl::VertexAttribute &attribute, GLsizei count, GLsizei instances);

    unsigned int getBufferSize() const;

    unsigned int getSerial() const;

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                                      GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset);

    bool directStoragePossible(const gl::VertexAttribute &attrib,
                               const gl::VertexAttribCurrentValueData &currentValue) const;

    VertexBuffer* getVertexBuffer() const;

  protected:
    virtual bool reserveSpace(unsigned int size) = 0;

    unsigned int getWritePosition() const;
    void setWritePosition(unsigned int writePosition);

    bool discard();

    bool setBufferSize(unsigned int size);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBufferInterface);

    rx::Renderer *const mRenderer;

    VertexBuffer* mVertexBuffer;

    unsigned int mWritePosition;
    unsigned int mReservedSpace;
    bool mDynamic;
};

class StreamingVertexBufferInterface : public VertexBufferInterface
{
  public:
    StreamingVertexBufferInterface(rx::Renderer *renderer, std::size_t initialSize);
    ~StreamingVertexBufferInterface();

  protected:
    bool reserveSpace(unsigned int size);
};

class StaticVertexBufferInterface : public VertexBufferInterface
{
  public:
    explicit StaticVertexBufferInterface(rx::Renderer *renderer);
    ~StaticVertexBufferInterface();

    bool storeVertexAttributes(const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue,
                               GLint start, GLsizei count, GLsizei instances, unsigned int *outStreamOffset);

    bool lookupAttribute(const gl::VertexAttribute &attribute, unsigned int* outStreamFffset);

  protected:
    bool reserveSpace(unsigned int size);

  private:
    struct VertexElement
    {
        GLenum type;
        GLuint size;
        GLuint stride;
        bool normalized;
        bool pureInteger;
        size_t attributeOffset;

        unsigned int streamOffset;
    };

    std::vector<VertexElement> mCache;
};

}

#endif 