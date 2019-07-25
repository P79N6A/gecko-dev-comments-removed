






































#include "WebGLContext.h"

using namespace mozilla;





PRBool
WebGLContext::ValidateBuffers(PRUint32 count)
{
    GLint currentProgram = -1;
    GLint numAttributes = -1;

    NS_ENSURE_TRUE(count > 0, PR_TRUE);

    MakeContextCurrent();

    
    gl->fGetIntegerv(LOCAL_GL_CURRENT_PROGRAM, &currentProgram);
    if (currentProgram == -1) {
        
        LogMessage("glGetIntegerv GL_CURRENT_PROGRAM failed: 0x%08x", (uint) gl->fGetError());
        return PR_FALSE;
    }

    if (WebGLuint(currentProgram) != mCurrentProgram->GLName()) {
        LogMessage("WebGL internal error: current program (%u) doesn't agree with GL current program (%d)", mCurrentProgram->GLName(), currentProgram);
        return PR_FALSE;
    }

    gl->fGetProgramiv(currentProgram, LOCAL_GL_ACTIVE_ATTRIBUTES, &numAttributes);
    if (numAttributes == -1) {
        
        LogMessage("glGetProgramiv GL_ACTIVE_ATTRIBUTES failed: 0x%08x", (uint) gl->fGetError());
        return PR_FALSE;
    }

    
    if (numAttributes > (GLint) mAttribBuffers.Length()) {
        
        LogMessage("GL_ACTIVE_ATTRIBUTES > GL_MAX_VERTEX_ATTRIBS");
        return PR_FALSE;
    }
    PRUint32 maxAttribs = numAttributes;

    for (PRUint32 i = 0; i < maxAttribs; ++i) {
      WebGLVertexAttribData& vd = mAttribBuffers[i];

      
      if (!vd.enabled)
          continue;

      if (vd.buf == nsnull) {
          LogMessage("No VBO bound to index %d (or it's been deleted)!", i);
          return PR_FALSE;
      }

      WebGLuint needed = vd.byteOffset +     
          vd.actualStride() * (count-1) +    
          vd.componentSize() * vd.size;      

      if (vd.buf->ByteLength() < needed) {
          LogMessage("VBO too small for bound attrib index %d: need at least %d bytes, but have only %d", i, needed, vd.buf->ByteLength());
          return PR_FALSE;
      }
    }

    return PR_TRUE;
}

PRBool WebGLContext::ValidateCapabilityEnum(WebGLenum cap)
{
    switch (cap) {
        case LOCAL_GL_BLEND:
        case LOCAL_GL_CULL_FACE:
        case LOCAL_GL_DEPTH_TEST:
        case LOCAL_GL_DITHER:
        case LOCAL_GL_POLYGON_OFFSET_FILL:
        case LOCAL_GL_SAMPLE_ALPHA_TO_COVERAGE:
        case LOCAL_GL_SAMPLE_COVERAGE:
        case LOCAL_GL_SCISSOR_TEST:
        case LOCAL_GL_STENCIL_TEST:
            return PR_TRUE;
        default:
            return PR_FALSE;
    }
}
