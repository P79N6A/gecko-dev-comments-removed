




#include "GLContext.h"

#include "mozilla/Preferences.h"
#include "mozilla/Assertions.h"
#include <stdint.h>

using namespace mozilla::gfx;

namespace mozilla {
namespace gl {

static const char kTexBlit_VertShaderSource[] = "\
attribute vec2 aPosition;                   \n\
                                            \n\
varying vec2 vTexCoord;                     \n\
                                            \n\
void main(void) {                           \n\
    vTexCoord = aPosition;                  \n\
    vec2 vertPos = aPosition * 2.0 - 1.0;   \n\
    gl_Position = vec4(vertPos, 0.0, 1.0);  \n\
}                                           \n\
";

static const char kTex2DBlit_FragShaderSource[] = "\
#ifdef GL_FRAGMENT_PRECISION_HIGH                   \n\
    precision highp float;                          \n\
#else                                               \n\
    precision mediump float;                        \n\
#endif                                              \n\
                                                    \n\
uniform sampler2D uTexUnit;                         \n\
                                                    \n\
varying vec2 vTexCoord;                             \n\
                                                    \n\
void main(void) {                                   \n\
    gl_FragColor = texture2D(uTexUnit, vTexCoord);  \n\
}                                                   \n\
";

static const char kTex2DRectBlit_FragShaderSource[] = "\
#ifdef GL_FRAGMENT_PRECISION_HIGH                             \n\
    precision highp float;                                    \n\
#else                                                         \n\
    precision mediump float;                                  \n\
#endif                                                        \n\
                                                              \n\
uniform sampler2D uTexUnit;                                   \n\
uniform vec2 uTexCoordMult;                                   \n\
                                                              \n\
varying vec2 vTexCoord;                                       \n\
                                                              \n\
void main(void) {                                             \n\
    gl_FragColor = texture2DRect(uTexUnit,                    \n\
                                 vTexCoord * uTexCoordMult);  \n\
}                                                             \n\
";


bool
GLContext::InitTexQuadProgram(GLenum target)
{
    MOZ_ASSERT(target == LOCAL_GL_TEXTURE_2D ||
               target == LOCAL_GL_TEXTURE_RECTANGLE_ARB);
    bool success = false;

    GLuint *programPtr;
    GLuint *fragShaderPtr;
    const char* fragShaderSource;
    if (target == LOCAL_GL_TEXTURE_2D) {
        programPtr = &mTex2DBlit_Program;
        fragShaderPtr = &mTex2DBlit_FragShader;
        fragShaderSource = kTex2DBlit_FragShaderSource;
    } else {
        programPtr = &mTex2DRectBlit_Program;
        fragShaderPtr = &mTex2DRectBlit_FragShader;
        fragShaderSource = kTex2DRectBlit_FragShaderSource;
    }

    GLuint& program = *programPtr;
    GLuint& fragShader = *fragShaderPtr;

    
    do {
        if (program) {
            
            success = true;
            break;
        }

        if (!mTexBlit_Buffer) {

            




            GLfloat verts[] = {
                0.0f, 0.0f,
                1.0f, 0.0f,
                0.0f, 1.0f,
                1.0f, 1.0f
            };

            MOZ_ASSERT(!mTexBlit_Buffer);
            fGenBuffers(1, &mTexBlit_Buffer);
            fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mTexBlit_Buffer);

            const size_t vertsSize = sizeof(verts);
            
            MOZ_ASSERT(vertsSize >= 3 * sizeof(GLfloat));
            fBufferData(LOCAL_GL_ARRAY_BUFFER, vertsSize, verts, LOCAL_GL_STATIC_DRAW);
        }

        if (!mTexBlit_VertShader) {

            const char* vertShaderSource = kTexBlit_VertShaderSource;

            mTexBlit_VertShader = fCreateShader(LOCAL_GL_VERTEX_SHADER);
            fShaderSource(mTexBlit_VertShader, 1, &vertShaderSource, nullptr);
            fCompileShader(mTexBlit_VertShader);
        }

        MOZ_ASSERT(!fragShader);
        fragShader = fCreateShader(LOCAL_GL_FRAGMENT_SHADER);
        fShaderSource(fragShader, 1, &fragShaderSource, nullptr);
        fCompileShader(fragShader);

        program = fCreateProgram();
        fAttachShader(program, mTexBlit_VertShader);
        fAttachShader(program, fragShader);
        fBindAttribLocation(program, 0, "aPosition");
        fLinkProgram(program);

        if (DebugMode()) {
            GLint status = 0;
            fGetShaderiv(mTexBlit_VertShader, LOCAL_GL_COMPILE_STATUS, &status);
            if (status != LOCAL_GL_TRUE) {
                NS_ERROR("Vert shader compilation failed.");

                GLint length = 0;
                fGetShaderiv(mTexBlit_VertShader, LOCAL_GL_INFO_LOG_LENGTH, &length);
                if (!length) {
                    printf_stderr("No shader info log available.\n");
                    break;
                }

                nsAutoArrayPtr<char> buffer(new char[length]);
                fGetShaderInfoLog(mTexBlit_VertShader, length, nullptr, buffer);

                printf_stderr("Shader info log (%d bytes): %s\n", length, buffer.get());
                break;
            }

            status = 0;
            fGetShaderiv(fragShader, LOCAL_GL_COMPILE_STATUS, &status);
            if (status != LOCAL_GL_TRUE) {
                NS_ERROR("Frag shader compilation failed.");

                GLint length = 0;
                fGetShaderiv(fragShader, LOCAL_GL_INFO_LOG_LENGTH, &length);
                if (!length) {
                    printf_stderr("No shader info log available.\n");
                    break;
                }

                nsAutoArrayPtr<char> buffer(new char[length]);
                fGetShaderInfoLog(fragShader, length, nullptr, buffer);

                printf_stderr("Shader info log (%d bytes): %s\n", length, buffer.get());
                break;
            }
        }

        GLint status = 0;
        fGetProgramiv(program, LOCAL_GL_LINK_STATUS, &status);
        if (status != LOCAL_GL_TRUE) {
            if (DebugMode()) {
                NS_ERROR("Linking blit program failed.");
                GLint length = 0;
                fGetProgramiv(program, LOCAL_GL_INFO_LOG_LENGTH, &length);
                if (!length) {
                    printf_stderr("No program info log available.\n");
                    break;
                }

                nsAutoArrayPtr<char> buffer(new char[length]);
                fGetProgramInfoLog(program, length, nullptr, buffer);

                printf_stderr("Program info log (%d bytes): %s\n", length, buffer.get());
            }
            break;
        }

        MOZ_ASSERT(fGetAttribLocation(program, "aPosition") == 0);
        GLint texUnitLoc = fGetUniformLocation(program, "uTexUnit");
        MOZ_ASSERT(texUnitLoc != -1, "uniform not found");

        
        fUseProgram(program);
        fUniform1i(texUnitLoc, 0);

        success = true;
    } while (false);

    if (!success) {
        NS_ERROR("Creating program for texture blit failed!");

        
        DeleteTexBlitProgram();
        return false;
    }

    fUseProgram(program);
    fEnableVertexAttribArray(0);
    fBindBuffer(LOCAL_GL_ARRAY_BUFFER, mTexBlit_Buffer);
    fVertexAttribPointer(0,
                         2,
                         LOCAL_GL_FLOAT,
                         false,
                         0,
                         nullptr);
    return true;
}

bool
GLContext::UseTexQuadProgram(GLenum target, const gfxIntSize& srcSize)
{
    if (!InitTexQuadProgram(target)) {
        return false;
    }

    if (target == LOCAL_GL_TEXTURE_RECTANGLE_ARB) {
        GLint texCoordMultLoc = fGetUniformLocation(mTex2DRectBlit_Program, "uTexCoordMult");
        MOZ_ASSERT(texCoordMultLoc != -1, "uniform not found");
        fUniform2f(texCoordMultLoc, srcSize.width, srcSize.height);
    }

    return true;
}

void
GLContext::DeleteTexBlitProgram()
{
    if (mTexBlit_Buffer) {
        fDeleteBuffers(1, &mTexBlit_Buffer);
        mTexBlit_Buffer = 0;
    }
    if (mTexBlit_VertShader) {
        fDeleteShader(mTexBlit_VertShader);
        mTexBlit_VertShader = 0;
    }
    if (mTex2DBlit_FragShader) {
        fDeleteShader(mTex2DBlit_FragShader);
        mTex2DBlit_FragShader = 0;
    }
    if (mTex2DRectBlit_FragShader) {
        fDeleteShader(mTex2DRectBlit_FragShader);
        mTex2DRectBlit_FragShader = 0;
    }
    if (mTex2DBlit_Program) {
        fDeleteProgram(mTex2DBlit_Program);
        mTex2DBlit_Program = 0;
    }
    if (mTex2DRectBlit_Program) {
        fDeleteProgram(mTex2DRectBlit_Program);
        mTex2DRectBlit_Program = 0;
    }
}

void
GLContext::BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                        const gfxIntSize& srcSize,
                                        const gfxIntSize& destSize)
{
    MOZ_ASSERT(!srcFB || fIsFramebuffer(srcFB));
    MOZ_ASSERT(!destFB || fIsFramebuffer(destFB));

    MOZ_ASSERT(IsSupported(GLFeature::framebuffer_blit));

    ScopedBindFramebuffer boundFB(this);
    ScopedGLState scissor(this, LOCAL_GL_SCISSOR_TEST, false);

    BindReadFB(srcFB);
    BindDrawFB(destFB);

    fBlitFramebuffer(0, 0,  srcSize.width,  srcSize.height,
                     0, 0, destSize.width, destSize.height,
                     LOCAL_GL_COLOR_BUFFER_BIT,
                     LOCAL_GL_NEAREST);
}

void
GLContext::BlitFramebufferToFramebuffer(GLuint srcFB, GLuint destFB,
                                        const gfxIntSize& srcSize,
                                        const gfxIntSize& destSize,
                                        const GLFormats& srcFormats)
{
    MOZ_ASSERT(!srcFB || fIsFramebuffer(srcFB));
    MOZ_ASSERT(!destFB || fIsFramebuffer(destFB));
    
    if (IsSupported(GLFeature::framebuffer_blit)) {
        BlitFramebufferToFramebuffer(srcFB, destFB,
                                     srcSize, destSize);
        return;
    }

    GLuint tex = CreateTextureForOffscreen(srcFormats, srcSize);
    MOZ_ASSERT(tex);

    BlitFramebufferToTexture(srcFB, tex, srcSize, srcSize);
    BlitTextureToFramebuffer(tex, destFB, srcSize, destSize);

    fDeleteTextures(1, &tex);
}

void
GLContext::BlitTextureToFramebuffer(GLuint srcTex, GLuint destFB,
                                    const gfxIntSize& srcSize,
                                    const gfxIntSize& destSize,
                                    GLenum srcTarget)
{
    MOZ_ASSERT(fIsTexture(srcTex));
    MOZ_ASSERT(!destFB || fIsFramebuffer(destFB));

    if (IsSupported(GLFeature::framebuffer_blit)) {
        ScopedFramebufferForTexture srcWrapper(this, srcTex, srcTarget);
        MOZ_ASSERT(srcWrapper.IsComplete());

        BlitFramebufferToFramebuffer(srcWrapper.FB(), destFB,
                                     srcSize, destSize);
        return;
    }


    ScopedBindFramebuffer boundFB(this, destFB);
    
    
    ScopedBindTextureUnit boundTU(this, LOCAL_GL_TEXTURE0);
    ScopedBindTexture boundTex(this, srcTex, srcTarget);

    GLuint boundProgram = 0;
    GetUIntegerv(LOCAL_GL_CURRENT_PROGRAM, &boundProgram);

    GLuint boundBuffer = 0;
    GetUIntegerv(LOCAL_GL_ARRAY_BUFFER_BINDING, &boundBuffer);

    















    GLint attrib0_enabled = 0;
    GLint attrib0_size = 0;
    GLint attrib0_stride = 0;
    GLint attrib0_type = 0;
    GLint attrib0_normalized = 0;
    GLint attrib0_bufferBinding = 0;
    void* attrib0_pointer = nullptr;

    fGetVertexAttribiv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attrib0_enabled);
    fGetVertexAttribiv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_SIZE, &attrib0_size);
    fGetVertexAttribiv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_STRIDE, &attrib0_stride);
    fGetVertexAttribiv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_TYPE, &attrib0_type);
    fGetVertexAttribiv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &attrib0_normalized);
    fGetVertexAttribiv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &attrib0_bufferBinding);
    fGetVertexAttribPointerv(0, LOCAL_GL_VERTEX_ATTRIB_ARRAY_POINTER, &attrib0_pointer);
    

    ScopedGLState blend       (this, LOCAL_GL_BLEND,      false);
    ScopedGLState cullFace    (this, LOCAL_GL_CULL_FACE,  false);
    ScopedGLState depthTest   (this, LOCAL_GL_DEPTH_TEST, false);
    ScopedGLState dither      (this, LOCAL_GL_DITHER,     false);
    ScopedGLState polyOffsFill(this, LOCAL_GL_POLYGON_OFFSET_FILL,      false);
    ScopedGLState sampleAToC  (this, LOCAL_GL_SAMPLE_ALPHA_TO_COVERAGE, false);
    ScopedGLState sampleCover (this, LOCAL_GL_SAMPLE_COVERAGE, false);
    ScopedGLState scissor     (this, LOCAL_GL_SCISSOR_TEST,    false);
    ScopedGLState stencil     (this, LOCAL_GL_STENCIL_TEST,    false);

    realGLboolean colorMask[4];
    fGetBooleanv(LOCAL_GL_COLOR_WRITEMASK, colorMask);
    fColorMask(LOCAL_GL_TRUE,
               LOCAL_GL_TRUE,
               LOCAL_GL_TRUE,
               LOCAL_GL_TRUE);

    GLint viewport[4];
    fGetIntegerv(LOCAL_GL_VIEWPORT, viewport);
    fViewport(0, 0, destSize.width, destSize.height);

    
    bool good = UseTexQuadProgram(srcTarget, srcSize);
    if (!good) {
        
        
        printf_stderr("[%s:%d] Fatal Error: Failed to prepare to blit texture->framebuffer.\n",
                      __FILE__, __LINE__);
        MOZ_CRASH();
    }
    fDrawArrays(LOCAL_GL_TRIANGLE_STRIP, 0, 4);

    fViewport(viewport[0], viewport[1],
              viewport[2], viewport[3]);

    fColorMask(colorMask[0],
               colorMask[1],
               colorMask[2],
               colorMask[3]);

    if (attrib0_enabled)
        fEnableVertexAttribArray(0);

    fBindBuffer(LOCAL_GL_ARRAY_BUFFER, attrib0_bufferBinding);
    fVertexAttribPointer(0,
                         attrib0_size,
                         attrib0_type,
                         attrib0_normalized,
                         attrib0_stride,
                         attrib0_pointer);

    fBindBuffer(LOCAL_GL_ARRAY_BUFFER, boundBuffer);

    fUseProgram(boundProgram);
}

void
GLContext::BlitFramebufferToTexture(GLuint srcFB, GLuint destTex,
                                    const gfxIntSize& srcSize,
                                    const gfxIntSize& destSize,
                                    GLenum destTarget)
{
    MOZ_ASSERT(!srcFB || fIsFramebuffer(srcFB));
    MOZ_ASSERT(fIsTexture(destTex));

    if (IsSupported(GLFeature::framebuffer_blit)) {
        ScopedFramebufferForTexture destWrapper(this, destTex, destTarget);

        BlitFramebufferToFramebuffer(srcFB, destWrapper.FB(),
                                     srcSize, destSize);
        return;
    }

    ScopedBindTexture autoTex(this, destTex, destTarget);
    ScopedBindFramebuffer boundFB(this, srcFB);
    ScopedGLState scissor(this, LOCAL_GL_SCISSOR_TEST, false);

    fCopyTexSubImage2D(destTarget, 0,
                       0, 0,
                       0, 0,
                       srcSize.width, srcSize.height);
}

void
GLContext::BlitTextureToTexture(GLuint srcTex, GLuint destTex,
                                const gfxIntSize& srcSize,
                                const gfxIntSize& destSize,
                                GLenum srcTarget, GLenum destTarget)
{
    MOZ_ASSERT(fIsTexture(srcTex));
    MOZ_ASSERT(fIsTexture(destTex));

    if (mTexBlit_UseDrawNotCopy) {
        
        ScopedFramebufferForTexture destWrapper(this, destTex, destTarget);

        BlitTextureToFramebuffer(srcTex, destWrapper.FB(),
                                 srcSize, destSize, srcTarget);
        return;
    }

    
    ScopedFramebufferForTexture srcWrapper(this, srcTex, srcTarget);

    BlitFramebufferToTexture(srcWrapper.FB(), destTex,
                             srcSize, destSize, destTarget);
}


} 
} 
