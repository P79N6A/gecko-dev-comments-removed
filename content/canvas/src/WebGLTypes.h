




#ifndef WEBGLTYPES_H_
#define WEBGLTYPES_H_


typedef uint32_t WebGLenum;
typedef uint32_t WebGLbitfield;
typedef int32_t WebGLint;
typedef int32_t WebGLsizei;
typedef int64_t WebGLsizeiptr;
typedef int64_t WebGLintptr;
typedef uint32_t WebGLuint;
typedef float WebGLfloat;
typedef float WebGLclampf;
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
