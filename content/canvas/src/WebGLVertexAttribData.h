




#ifndef WEBGLVERTEXATTRIBDATA_H_
#define WEBGLVERTEXATTRIBDATA_H_

#include "WebGLObjectModel.h"

namespace mozilla {

class WebGLBuffer;

struct WebGLVertexAttribData {
    
    WebGLVertexAttribData()
        : buf(0), stride(0), size(4), byteOffset(0),
          type(LOCAL_GL_FLOAT), enabled(false), normalized(false)
    { }

    WebGLRefPtr<WebGLBuffer> buf;
    WebGLuint stride;
    WebGLuint size;
    GLuint byteOffset;
    GLenum type;
    bool enabled;
    bool normalized;

    GLuint componentSize() const {
        switch(type) {
            case LOCAL_GL_BYTE:
                return sizeof(GLbyte);
                break;
            case LOCAL_GL_UNSIGNED_BYTE:
                return sizeof(GLubyte);
                break;
            case LOCAL_GL_SHORT:
                return sizeof(GLshort);
                break;
            case LOCAL_GL_UNSIGNED_SHORT:
                return sizeof(GLushort);
                break;
            
            case LOCAL_GL_FLOAT:
                return sizeof(GLfloat);
                break;
            default:
                NS_ERROR("Should never get here!");
                return 0;
        }
    }

    GLuint actualStride() const {
        if (stride) return stride;
        return size * componentSize();
    }
};

} 

inline void ImplCycleCollectionUnlink(mozilla::WebGLVertexAttribData& aField)
{
  aField.buf = nullptr;
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            mozilla::WebGLVertexAttribData& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  CycleCollectionNoteEdgeName(aCallback, aName, aFlags);
  aCallback.NoteXPCOMChild(aField.buf);
}

#endif