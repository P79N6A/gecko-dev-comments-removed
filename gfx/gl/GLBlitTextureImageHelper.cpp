





#include "GLBlitTextureImageHelper.h"
#include "GLUploadHelpers.h"
#include "DecomposeIntoNoRepeatTriangles.h"
#include "GLContext.h"
#include "ScopedGLHelpers.h"
#include "nsRect.h"
#include "gfx2DGlue.h"
#include "gfxUtils.h"

namespace mozilla {
namespace gl {

GLBlitTextureImageHelper::GLBlitTextureImageHelper(GLContext* gl)
    : mGL(gl)
    , mBlitProgram(0)
    , mBlitFramebuffer(0)

{
}

GLBlitTextureImageHelper::~GLBlitTextureImageHelper()
{
    
    mGL->fDeleteProgram(mBlitProgram);
    mGL->fDeleteFramebuffers(1, &mBlitFramebuffer);
}

void
GLBlitTextureImageHelper::BlitTextureImage(TextureImage *aSrc, const nsIntRect& aSrcRect,
                                           TextureImage *aDst, const nsIntRect& aDstRect)
{
    NS_ASSERTION(!aSrc->InUpdate(), "Source texture is in update!");
    NS_ASSERTION(!aDst->InUpdate(), "Destination texture is in update!");

    if (aSrcRect.IsEmpty() || aDstRect.IsEmpty())
        return;

    int savedFb = 0;
    mGL->fGetIntegerv(LOCAL_GL_FRAMEBUFFER_BINDING, &savedFb);

    ScopedGLState scopedScissorTestState(mGL, LOCAL_GL_SCISSOR_TEST, false);
    ScopedGLState scopedBlendState(mGL, LOCAL_GL_BLEND, false);

    
    float blitScaleX = float(aDstRect.width) / float(aSrcRect.width);
    float blitScaleY = float(aDstRect.height) / float(aSrcRect.height);

    
    aDst->BeginBigImageIteration();
    do {
        
        nsIntRect dstSubRect;
        nsIntRect dstTextureRect = ThebesIntRect(aDst->GetTileRect());
        dstSubRect.IntersectRect(aDstRect, dstTextureRect);

        
        if (dstSubRect.IsEmpty())
            continue;

        
        nsIntRect dstInSrcRect(dstSubRect);
        dstInSrcRect.MoveBy(-aDstRect.TopLeft());
        
        dstInSrcRect.ScaleRoundOut(1.0f / blitScaleX, 1.0f / blitScaleY);
        dstInSrcRect.MoveBy(aSrcRect.TopLeft());

        SetBlitFramebufferForDestTexture(aDst->GetTextureID());
        UseBlitProgram();

        aSrc->BeginBigImageIteration();
        
        do {
            
            nsIntRect srcSubRect;
            nsIntRect srcTextureRect = ThebesIntRect(aSrc->GetTileRect());
            srcSubRect.IntersectRect(aSrcRect, srcTextureRect);

            
            if (srcSubRect.IsEmpty()) {
                continue;
            }
            
            srcSubRect.IntersectRect(srcSubRect, dstInSrcRect);
            
            if (srcSubRect.IsEmpty()) {
                continue;
            }
            
            
            
            
            
            
            
            nsIntRect srcSubInDstRect(srcSubRect);
            srcSubInDstRect.MoveBy(-aSrcRect.TopLeft());
            srcSubInDstRect.ScaleRoundOut(blitScaleX, blitScaleY);
            srcSubInDstRect.MoveBy(aDstRect.TopLeft());

            
            nsIntSize srcSize = srcTextureRect.Size();
            nsIntSize dstSize = dstTextureRect.Size();
            srcSubRect.MoveBy(-srcTextureRect.x, -srcTextureRect.y);
            srcSubInDstRect.MoveBy(-dstTextureRect.x, -dstTextureRect.y);

            float dx0 = 2.0f * float(srcSubInDstRect.x) / float(dstSize.width) - 1.0f;
            float dy0 = 2.0f * float(srcSubInDstRect.y) / float(dstSize.height) - 1.0f;
            float dx1 = 2.0f * float(srcSubInDstRect.x + srcSubInDstRect.width) / float(dstSize.width) - 1.0f;
            float dy1 = 2.0f * float(srcSubInDstRect.y + srcSubInDstRect.height) / float(dstSize.height) - 1.0f;
            ScopedViewportRect autoViewportRect(mGL, 0, 0, dstSize.width, dstSize.height);

            RectTriangles rects;

            nsIntSize realTexSize = srcSize;
            if (!CanUploadNonPowerOfTwo(mGL)) {
                realTexSize = nsIntSize(gfx::NextPowerOfTwo(srcSize.width),
                                        gfx::NextPowerOfTwo(srcSize.height));
            }

            if (aSrc->GetWrapMode() == LOCAL_GL_REPEAT) {
                rects.addRect(
                        dx0, dy0, dx1, dy1,
                        
                        srcSubRect.x / float(realTexSize.width),
                        srcSubRect.y / float(realTexSize.height),
                        srcSubRect.XMost() / float(realTexSize.width),
                        srcSubRect.YMost() / float(realTexSize.height));
            } else {
                DecomposeIntoNoRepeatTriangles(srcSubRect, realTexSize, rects);

                
                
                InfallibleTArray<RectTriangles::coord>& coords = rects.vertCoords();

                for (unsigned int i = 0; i < coords.Length(); ++i) {
                    coords[i].x = (coords[i].x * (dx1 - dx0)) + dx0;
                    coords[i].y = (coords[i].y * (dy1 - dy0)) + dy0;
                }
            }

            ScopedBindTextureUnit autoTexUnit(mGL, LOCAL_GL_TEXTURE0);
            ScopedBindTexture autoTex(mGL, aSrc->GetTextureID());
            ScopedVertexAttribPointer autoAttrib0(mGL, 0, 2, LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0, 0, rects.vertCoords().Elements());
            ScopedVertexAttribPointer autoAttrib1(mGL, 1, 2, LOCAL_GL_FLOAT, LOCAL_GL_FALSE, 0, 0, rects.texCoords().Elements());

            mGL->fDrawArrays(LOCAL_GL_TRIANGLES, 0, rects.elements());

        } while (aSrc->NextTile());
    } while (aDst->NextTile());

    
    SetBlitFramebufferForDestTexture(0);

    mGL->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, savedFb);
}

void
GLBlitTextureImageHelper::SetBlitFramebufferForDestTexture(GLuint aTexture)
{
    if (!mBlitFramebuffer) {
        mGL->fGenFramebuffers(1, &mBlitFramebuffer);
    }

    mGL->fBindFramebuffer(LOCAL_GL_FRAMEBUFFER, mBlitFramebuffer);
    mGL->fFramebufferTexture2D(LOCAL_GL_FRAMEBUFFER,
                               LOCAL_GL_COLOR_ATTACHMENT0,
                               LOCAL_GL_TEXTURE_2D,
                               aTexture,
                               0);

    GLenum result = mGL->fCheckFramebufferStatus(LOCAL_GL_FRAMEBUFFER);
    if (aTexture && (result != LOCAL_GL_FRAMEBUFFER_COMPLETE)) {
        nsAutoCString msg;
        msg.AppendLiteral("Framebuffer not complete -- error 0x");
        msg.AppendInt(result, 16);
        
        
        
        
        NS_RUNTIMEABORT(msg.get());
    }
}

void
GLBlitTextureImageHelper::UseBlitProgram()
{
    if (mBlitProgram) {
        mGL->fUseProgram(mBlitProgram);
        return;
    }

    mBlitProgram = mGL->fCreateProgram();

    GLuint shaders[2];
    shaders[0] = mGL->fCreateShader(LOCAL_GL_VERTEX_SHADER);
    shaders[1] = mGL->fCreateShader(LOCAL_GL_FRAGMENT_SHADER);

    const char *blitVSSrc =
        "attribute vec2 aVertex;"
        "attribute vec2 aTexCoord;"
        "varying vec2 vTexCoord;"
        "void main() {"
        "  vTexCoord = aTexCoord;"
        "  gl_Position = vec4(aVertex, 0.0, 1.0);"
        "}";
    const char *blitFSSrc = "#ifdef GL_ES\nprecision mediump float;\n#endif\n"
        "uniform sampler2D uSrcTexture;"
        "varying vec2 vTexCoord;"
        "void main() {"
        "  gl_FragColor = texture2D(uSrcTexture, vTexCoord);"
        "}";

    mGL->fShaderSource(shaders[0], 1, (const GLchar**) &blitVSSrc, nullptr);
    mGL->fShaderSource(shaders[1], 1, (const GLchar**) &blitFSSrc, nullptr);

    for (int i = 0; i < 2; ++i) {
        GLint success, len = 0;

        mGL->fCompileShader(shaders[i]);
        mGL->fGetShaderiv(shaders[i], LOCAL_GL_COMPILE_STATUS, &success);
        NS_ASSERTION(success, "Shader compilation failed!");

        if (!success) {
            nsAutoCString log;
            mGL->fGetShaderiv(shaders[i], LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &len);
            log.SetCapacity(len);
            mGL->fGetShaderInfoLog(shaders[i], len, (GLint*) &len, (char*) log.BeginWriting());
            log.SetLength(len);

            printf_stderr("Shader %d compilation failed:\n%s\n", i, log.get());
            return;
        }

        mGL->fAttachShader(mBlitProgram, shaders[i]);
        mGL->fDeleteShader(shaders[i]);
    }

    mGL->fBindAttribLocation(mBlitProgram, 0, "aVertex");
    mGL->fBindAttribLocation(mBlitProgram, 1, "aTexCoord");

    mGL->fLinkProgram(mBlitProgram);

    GLint success, len = 0;
    mGL->fGetProgramiv(mBlitProgram, LOCAL_GL_LINK_STATUS, &success);
    NS_ASSERTION(success, "Shader linking failed!");

    if (!success) {
        nsAutoCString log;
        mGL->fGetProgramiv(mBlitProgram, LOCAL_GL_INFO_LOG_LENGTH, (GLint*) &len);
        log.SetCapacity(len);
        mGL->fGetProgramInfoLog(mBlitProgram, len, (GLint*) &len, (char*) log.BeginWriting());
        log.SetLength(len);

        printf_stderr("Program linking failed:\n%s\n", log.get());
        return;
    }

    mGL->fUseProgram(mBlitProgram);
    mGL->fUniform1i(mGL->fGetUniformLocation(mBlitProgram, "uSrcTexture"), 0);
}

}
}
