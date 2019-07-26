




#ifndef WEBGLTYPES_H_
#define WEBGLTYPES_H_


#include "GLTypes.h"



typedef int64_t WebGLsizeiptr;
typedef int64_t WebGLintptr;
typedef bool WebGLboolean;

namespace mozilla {

enum FakeBlackStatus { DoNotNeedFakeBlack, DoNeedFakeBlack, DontKnowIfNeedFakeBlack };

struct VertexAttrib0Status {
    enum { Default, EmulatedUninitializedArray, EmulatedInitializedArray };
};

namespace WebGLTexelConversions {









enum WebGLTexelFormat
{
    
    
    BadFormat,
    
    
    
    Auto,
    
    R8,
    A8,
    D16, 
    D32, 
    R32F, 
    A32F, 
    
    RA8,
    RA32F,
    D24S8, 
    
    RGB8,
    BGRX8, 
    RGB565,
    RGB32F, 
    
    RGBA8,
    BGRA8, 
    RGBA5551,
    RGBA4444,
    RGBA32F 
};

} 

} 

#endif
