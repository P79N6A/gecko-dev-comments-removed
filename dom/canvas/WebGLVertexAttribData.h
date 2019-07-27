




#ifndef WEBGL_VERTEX_ATTRIB_DATA_H_
#define WEBGL_VERTEX_ATTRIB_DATA_H_

#include "GLDefs.h"
#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLBuffer;

struct WebGLVertexAttribData
{
    
    WebGLVertexAttribData()
        : buf(0)
        , stride(0)
        , size(4)
        , divisor(0) 
        , byteOffset(0)
        , type(LOCAL_GL_FLOAT)
        , enabled(false)
        , normalized(false)
        , integer(false)
    {}

    WebGLRefPtr<WebGLBuffer> buf;
    GLuint stride;
    GLuint size;
    GLuint divisor;
    GLuint byteOffset;
    GLenum type;
    bool enabled;
    bool normalized;
    bool integer;

    GLuint componentSize() const {
        switch(type) {
        case LOCAL_GL_BYTE:
            return sizeof(GLbyte);

        case LOCAL_GL_UNSIGNED_BYTE:
            return sizeof(GLubyte);

        case LOCAL_GL_SHORT:
            return sizeof(GLshort);

        case LOCAL_GL_UNSIGNED_SHORT:
            return sizeof(GLushort);

        
        case LOCAL_GL_FLOAT:
            return sizeof(GLfloat);

        default:
            NS_ERROR("Should never get here!");
            return 0;
        }
    }

    GLuint actualStride() const {
        if (stride)
            return stride;

        return size * componentSize();
    }
};

} 

inline void
ImplCycleCollectionUnlink(mozilla::WebGLVertexAttribData& field)
{
    field.buf = nullptr;
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& callback,
                            mozilla::WebGLVertexAttribData& field,
                            const char* name,
                            uint32_t flags = 0)
{
    CycleCollectionNoteChild(callback, field.buf.get(), name, flags);
}

#endif 
