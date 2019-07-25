






































#include "WebGLContext.h"

using namespace mozilla;




PRBool
WebGLProgram::UpdateInfo(gl::GLContext *gl)
{
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &mAttribMaxNameLength);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_UNIFORM_MAX_LENGTH, &mUniformMaxNameLength);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_UNIFORMS, &mUniformCount);
    gl->fGetProgramiv(mName, LOCAL_GL_ACTIVE_ATTRIBUTES, &mAttribCount);

    GLint numVertexAttribs;
    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &numVertexAttribs);
    mAttribsInUse.clear();
    mAttribsInUse.resize(numVertexAttribs);

    nsAutoArrayPtr<char> nameBuf(new char[mAttribMaxNameLength]);

    for (int i = 0; i < mAttribCount; ++i) {
        GLint attrnamelen;
        GLint attrsize;
        GLenum attrtype;
        gl->fGetActiveAttrib(mName, i, mAttribMaxNameLength, &attrnamelen, &attrsize, &attrtype, nameBuf);
        if (attrnamelen > 0) {
            GLint loc = gl->fGetAttribLocation(mName, nameBuf);
            mAttribsInUse[loc] = true;
        }
    }

    return PR_TRUE;
}





PRBool
WebGLContext::ValidateBuffers(PRUint32 count)
{
    NS_ENSURE_TRUE(count > 0, PR_TRUE);

#ifdef DEBUG
    GLuint currentProgram = 0;
    MakeContextCurrent();
    gl->fGetIntegerv(LOCAL_GL_CURRENT_PROGRAM, (GLint*) &currentProgram);
    NS_ASSERTION(currentProgram == mCurrentProgram->GLName(),
                 "WebGL: current program doesn't agree with GL state");
    if (currentProgram != mCurrentProgram->GLName())
        return PR_FALSE;
#endif

    PRUint32 attribs = mAttribBuffers.Length();
    for (PRUint32 i = 0; i < attribs; ++i) {
        const WebGLVertexAttribData& vd = mAttribBuffers[i];

        
        
        if (!vd.enabled)
            continue;

        if (vd.buf == nsnull) {
            LogMessage("No VBO bound to enabled attrib index %d!", i);
            return PR_FALSE;
        }

        
        
        if (!mCurrentProgram->IsAttribInUse(i))
            continue;

        
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

PRBool
WebGLContext::InitAndValidateGL()
{
    if (!gl) return PR_FALSE;

    mActiveTexture = 0;
    mSynthesizedGLError = LOCAL_GL_NO_ERROR;

    mAttribBuffers.Clear();

    mUniformTextures.Clear();
    mBound2DTextures.Clear();
    mBoundCubeMapTextures.Clear();

    mBoundArrayBuffer = nsnull;
    mBoundElementArrayBuffer = nsnull;
    mCurrentProgram = nsnull;

    mFramebufferColorAttachments.Clear();
    mFramebufferDepthAttachment = nsnull;
    mFramebufferStencilAttachment = nsnull;

    mBoundFramebuffer = nsnull;
    mBoundRenderbuffer = nsnull;

    mMapTextures.Clear();
    mMapBuffers.Clear();
    mMapPrograms.Clear();
    mMapShaders.Clear();
    mMapFramebuffers.Clear();
    mMapRenderbuffers.Clear();

    
    GLint val = 0;

    
    

    gl->fGetIntegerv(LOCAL_GL_MAX_VERTEX_ATTRIBS, &val);
    if (val == 0) {
        LogMessage("GL_MAX_VERTEX_ATTRIBS is 0!");
        return PR_FALSE;
    }

    mAttribBuffers.SetLength(val);

    

    
    
    
    
    gl->fGetIntegerv(LOCAL_GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &val);
    if (val == 0) {
        LogMessage("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS is 0!");
        return PR_FALSE;
    }

    mBound2DTextures.SetLength(val);
    mBoundCubeMapTextures.SetLength(val);

    

    gl->fGetIntegerv(LOCAL_GL_MAX_COLOR_ATTACHMENTS, &val);
    mFramebufferColorAttachments.SetLength(val);

#if defined(DEBUG_vladimir) && defined(USE_GLES2)
    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_FORMAT, &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_FORMAT: 0x%04x\n", val);

    gl->fGetIntegerv(LOCAL_GL_IMPLEMENTATION_COLOR_READ_TYPE, &val);
    fprintf(stderr, "GL_IMPLEMENTATION_COLOR_READ_TYPE: 0x%04x\n", val);
#endif

#ifndef USE_GLES2
    
    
    gl->fEnable(LOCAL_GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

    return PR_TRUE;
}
