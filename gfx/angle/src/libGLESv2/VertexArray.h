











#ifndef LIBGLESV2_VERTEXARRAY_H_
#define LIBGLESV2_VERTEXARRAY_H_

#include "common/RefCountObject.h"
#include "libGLESv2/constants.h"
#include "libGLESv2/VertexAttribute.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class Buffer;

class VertexArray : public RefCountObject
{
  public:
    VertexArray(rx::Renderer *renderer, GLuint id);
    ~VertexArray();

    const VertexAttribute& getVertexAttribute(unsigned int attributeIndex) const;
    void detachBuffer(GLuint bufferName);
    void setVertexAttribDivisor(GLuint index, GLuint divisor);
    void enableAttribute(unsigned int attributeIndex, bool enabledState);
    void setAttributeState(unsigned int attributeIndex, gl::Buffer *boundBuffer, GLint size, GLenum type,
                           bool normalized, bool pureInteger, GLsizei stride, const void *pointer);

    const VertexAttribute* getVertexAttributes() const { return mVertexAttributes; }
    Buffer *getElementArrayBuffer() const { return mElementArrayBuffer.get(); }
    void setElementArrayBuffer(Buffer *elementArrayBuffer) { mElementArrayBuffer.set(elementArrayBuffer); }
    GLuint getElementArrayBufferId() const { return mElementArrayBuffer.id(); }

  private:
    VertexAttribute mVertexAttributes[MAX_VERTEX_ATTRIBS];
    BindingPointer<Buffer> mElementArrayBuffer;
};

}

#endif 
