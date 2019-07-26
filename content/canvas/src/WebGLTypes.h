




#ifndef WEBGLTYPES_H_
#define WEBGLTYPES_H_

#include "mozilla/TypedEnum.h"


#include "GLTypes.h"



typedef int64_t WebGLsizeiptr;
typedef int64_t WebGLintptr;
typedef bool WebGLboolean;

namespace mozilla {




























MOZ_BEGIN_ENUM_CLASS(WebGLContextFakeBlackStatus, int)
  Unknown,
  NotNeeded,
  Needed
MOZ_END_ENUM_CLASS(WebGLContextFakeBlackStatus)

MOZ_BEGIN_ENUM_CLASS(WebGLTextureFakeBlackStatus, int)
  Unknown,
  NotNeeded,
  IncompleteTexture,
  UninitializedImageData
MOZ_END_ENUM_CLASS(WebGLTextureFakeBlackStatus)







MOZ_BEGIN_ENUM_CLASS(WebGLVertexAttrib0Status, int)
    Default, 
    EmulatedUninitializedArray, 
    EmulatedInitializedArray 
MOZ_END_ENUM_CLASS(WebGLVertexAttrib0Status)











MOZ_BEGIN_ENUM_CLASS(WebGLImageDataStatus, int)
    NoImageData,
    UninitializedImageData,
    InitializedImageData
MOZ_END_ENUM_CLASS(WebGLImageDataStatus)









MOZ_BEGIN_ENUM_CLASS(WebGLTexelFormat, int)
    
    
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
MOZ_END_ENUM_CLASS(WebGLTexelFormat)

} 

#endif
