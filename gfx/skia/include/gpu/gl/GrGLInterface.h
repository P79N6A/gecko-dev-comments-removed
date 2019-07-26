








#ifndef GrGLInterface_DEFINED
#define GrGLInterface_DEFINED

#include "GrGLFunctions.h"
#include "GrRefCnt.h"







enum GrGLBinding {
    kNone_GrGLBinding = 0x0,

    kDesktop_GrGLBinding = 0x01,
    kES2_GrGLBinding = 0x02,

    
    kFirstGrGLBinding = kDesktop_GrGLBinding,
    kLastGrGLBinding = kES2_GrGLBinding
};
























struct GrGLInterface;

const GrGLInterface* GrGLDefaultInterface();







const GrGLInterface* GrGLCreateNativeInterface();

#if SK_MESA



const GrGLInterface* GrGLCreateMesaInterface();
#endif

#if SK_ANGLE



const GrGLInterface* GrGLCreateANGLEInterface();
#endif





const GrGLInterface* GrGLCreateNullInterface();





const GrGLInterface* GrGLCreateDebugInterface();

#if GR_GL_PER_GL_FUNC_CALLBACK
typedef void (*GrGLInterfaceCallbackProc)(const GrGLInterface*);
typedef intptr_t GrGLInterfaceCallbackData;
#endif












struct GR_API GrGLInterface : public GrRefCnt {
private:
    
    template <typename FNPTR_TYPE> class GLPtr {
    public:
        GLPtr() : fPtr(NULL) {}
        GLPtr operator =(FNPTR_TYPE ptr) { fPtr = ptr; return *this; }
        operator FNPTR_TYPE() const { return fPtr; }
    private:
        FNPTR_TYPE fPtr;
    };

    typedef GrRefCnt INHERITED;

public:
    SK_DECLARE_INST_COUNT(GrGLInterface)

    GrGLInterface();

    
    
    
    
    bool validate(GrGLBinding binding) const;

    
    
    GrGLBinding fBindingsExported;

    GLPtr<GrGLActiveTextureProc> fActiveTexture;
    GLPtr<GrGLAttachShaderProc> fAttachShader;
    GLPtr<GrGLBeginQueryProc> fBeginQuery;
    GLPtr<GrGLBindAttribLocationProc> fBindAttribLocation;
    GLPtr<GrGLBindBufferProc> fBindBuffer;
    GLPtr<GrGLBindFragDataLocationProc> fBindFragDataLocation;
    GLPtr<GrGLBindFragDataLocationIndexedProc> fBindFragDataLocationIndexed;
    GLPtr<GrGLBindFramebufferProc> fBindFramebuffer;
    GLPtr<GrGLBindRenderbufferProc> fBindRenderbuffer;
    GLPtr<GrGLBindTextureProc> fBindTexture;
    GLPtr<GrGLBlendColorProc> fBlendColor;
    GLPtr<GrGLBlendFuncProc> fBlendFunc;
    GLPtr<GrGLBlitFramebufferProc> fBlitFramebuffer;
    GLPtr<GrGLBufferDataProc> fBufferData;
    GLPtr<GrGLBufferSubDataProc> fBufferSubData;
    GLPtr<GrGLCheckFramebufferStatusProc> fCheckFramebufferStatus;
    GLPtr<GrGLClearProc> fClear;
    GLPtr<GrGLClearColorProc> fClearColor;
    GLPtr<GrGLClearStencilProc> fClearStencil;
    GLPtr<GrGLColorMaskProc> fColorMask;
    GLPtr<GrGLCompileShaderProc> fCompileShader;
    GLPtr<GrGLCompressedTexImage2DProc> fCompressedTexImage2D;
    GLPtr<GrGLCreateProgramProc> fCreateProgram;
    GLPtr<GrGLCreateShaderProc> fCreateShader;
    GLPtr<GrGLCullFaceProc> fCullFace;
    GLPtr<GrGLDeleteBuffersProc> fDeleteBuffers;
    GLPtr<GrGLDeleteFramebuffersProc> fDeleteFramebuffers;
    GLPtr<GrGLDeleteProgramProc> fDeleteProgram;
    GLPtr<GrGLDeleteQueriesProc> fDeleteQueries;
    GLPtr<GrGLDeleteRenderbuffersProc> fDeleteRenderbuffers;
    GLPtr<GrGLDeleteShaderProc> fDeleteShader;
    GLPtr<GrGLDeleteTexturesProc> fDeleteTextures;
    GLPtr<GrGLDepthMaskProc> fDepthMask;
    GLPtr<GrGLDisableProc> fDisable;
    GLPtr<GrGLDisableVertexAttribArrayProc> fDisableVertexAttribArray;
    GLPtr<GrGLDrawArraysProc> fDrawArrays;
    GLPtr<GrGLDrawBufferProc> fDrawBuffer;
    GLPtr<GrGLDrawBuffersProc> fDrawBuffers;
    GLPtr<GrGLDrawElementsProc> fDrawElements;
    GLPtr<GrGLEnableProc> fEnable;
    GLPtr<GrGLEnableVertexAttribArrayProc> fEnableVertexAttribArray;
    GLPtr<GrGLEndQueryProc> fEndQuery;
    GLPtr<GrGLFinishProc> fFinish;
    GLPtr<GrGLFlushProc> fFlush;
    GLPtr<GrGLFramebufferRenderbufferProc> fFramebufferRenderbuffer;
    GLPtr<GrGLFramebufferTexture2DProc> fFramebufferTexture2D;
    GLPtr<GrGLFrontFaceProc> fFrontFace;
    GLPtr<GrGLGenBuffersProc> fGenBuffers;
    GLPtr<GrGLGenFramebuffersProc> fGenFramebuffers;
    GLPtr<GrGLGenQueriesProc> fGenQueries;
    GLPtr<GrGLGenRenderbuffersProc> fGenRenderbuffers;
    GLPtr<GrGLGenTexturesProc> fGenTextures;
    GLPtr<GrGLGetBufferParameterivProc> fGetBufferParameteriv;
    GLPtr<GrGLGetErrorProc> fGetError;
    GLPtr<GrGLGetFramebufferAttachmentParameterivProc> fGetFramebufferAttachmentParameteriv;
    GLPtr<GrGLGetIntegervProc> fGetIntegerv;
    GLPtr<GrGLGetQueryObjecti64vProc> fGetQueryObjecti64v;
    GLPtr<GrGLGetQueryObjectivProc> fGetQueryObjectiv;
    GLPtr<GrGLGetQueryObjectui64vProc> fGetQueryObjectui64v;
    GLPtr<GrGLGetQueryObjectuivProc> fGetQueryObjectuiv;
    GLPtr<GrGLGetQueryivProc> fGetQueryiv;
    GLPtr<GrGLGetProgramInfoLogProc> fGetProgramInfoLog;
    GLPtr<GrGLGetProgramivProc> fGetProgramiv;
    GLPtr<GrGLGetRenderbufferParameterivProc> fGetRenderbufferParameteriv;
    GLPtr<GrGLGetShaderInfoLogProc> fGetShaderInfoLog;
    GLPtr<GrGLGetShaderivProc> fGetShaderiv;
    GLPtr<GrGLGetStringProc> fGetString;
    GLPtr<GrGLGetTexLevelParameterivProc> fGetTexLevelParameteriv;
    GLPtr<GrGLGetUniformLocationProc> fGetUniformLocation;
    GLPtr<GrGLLineWidthProc> fLineWidth;
    GLPtr<GrGLLinkProgramProc> fLinkProgram;
    GLPtr<GrGLMapBufferProc> fMapBuffer;
    GLPtr<GrGLPixelStoreiProc> fPixelStorei;
    GLPtr<GrGLQueryCounterProc> fQueryCounter;
    GLPtr<GrGLReadBufferProc> fReadBuffer;
    GLPtr<GrGLReadPixelsProc> fReadPixels;
    GLPtr<GrGLRenderbufferStorageProc> fRenderbufferStorage;
    GLPtr<GrGLRenderbufferStorageMultisampleProc> fRenderbufferStorageMultisample;
    GLPtr<GrGLRenderbufferStorageMultisampleCoverageProc> fRenderbufferStorageMultisampleCoverage;
    GLPtr<GrGLResolveMultisampleFramebufferProc> fResolveMultisampleFramebuffer;
    GLPtr<GrGLScissorProc> fScissor;
    GLPtr<GrGLShaderSourceProc> fShaderSource;
    GLPtr<GrGLStencilFuncProc> fStencilFunc;
    GLPtr<GrGLStencilFuncSeparateProc> fStencilFuncSeparate;
    GLPtr<GrGLStencilMaskProc> fStencilMask;
    GLPtr<GrGLStencilMaskSeparateProc> fStencilMaskSeparate;
    GLPtr<GrGLStencilOpProc> fStencilOp;
    GLPtr<GrGLStencilOpSeparateProc> fStencilOpSeparate;
    GLPtr<GrGLTexImage2DProc> fTexImage2D;
    GLPtr<GrGLTexParameteriProc> fTexParameteri;
    GLPtr<GrGLTexParameterivProc> fTexParameteriv;
    GLPtr<GrGLTexSubImage2DProc> fTexSubImage2D;
    GLPtr<GrGLTexStorage2DProc> fTexStorage2D;
    GLPtr<GrGLUniform1fProc> fUniform1f;
    GLPtr<GrGLUniform1iProc> fUniform1i;
    GLPtr<GrGLUniform1fvProc> fUniform1fv;
    GLPtr<GrGLUniform1ivProc> fUniform1iv;
    GLPtr<GrGLUniform2fProc> fUniform2f;
    GLPtr<GrGLUniform2iProc> fUniform2i;
    GLPtr<GrGLUniform2fvProc> fUniform2fv;
    GLPtr<GrGLUniform2ivProc> fUniform2iv;
    GLPtr<GrGLUniform3fProc> fUniform3f;
    GLPtr<GrGLUniform3iProc> fUniform3i;
    GLPtr<GrGLUniform3fvProc> fUniform3fv;
    GLPtr<GrGLUniform3ivProc> fUniform3iv;
    GLPtr<GrGLUniform4fProc> fUniform4f;
    GLPtr<GrGLUniform4iProc> fUniform4i;
    GLPtr<GrGLUniform4fvProc> fUniform4fv;
    GLPtr<GrGLUniform4ivProc> fUniform4iv;
    GLPtr<GrGLUniformMatrix2fvProc> fUniformMatrix2fv;
    GLPtr<GrGLUniformMatrix3fvProc> fUniformMatrix3fv;
    GLPtr<GrGLUniformMatrix4fvProc> fUniformMatrix4fv;
    GLPtr<GrGLUnmapBufferProc> fUnmapBuffer;
    GLPtr<GrGLUseProgramProc> fUseProgram;
    GLPtr<GrGLVertexAttrib4fvProc> fVertexAttrib4fv;
    GLPtr<GrGLVertexAttribPointerProc> fVertexAttribPointer;
    GLPtr<GrGLViewportProc> fViewport;

    
    
    
    GLPtr<GrGLMatrixModeProc> fMatrixMode;
    GLPtr<GrGLLoadIdentityProc> fLoadIdentity;
    GLPtr<GrGLLoadMatrixfProc> fLoadMatrixf;
    GLPtr<GrGLPathCommandsProc> fPathCommands;
    GLPtr<GrGLPathCoordsProc> fPathCoords;
    GLPtr<GrGLPathSubCommandsProc> fPathSubCommands;
    GLPtr<GrGLPathSubCoordsProc> fPathSubCoords;
    GLPtr<GrGLPathStringProc> fPathString;
    GLPtr<GrGLPathGlyphsProc> fPathGlyphs;
    GLPtr<GrGLPathGlyphRangeProc> fPathGlyphRange;
    GLPtr<GrGLWeightPathsProc> fWeightPaths;
    GLPtr<GrGLCopyPathProc> fCopyPath;
    GLPtr<GrGLInterpolatePathsProc> fInterpolatePaths;
    GLPtr<GrGLTransformPathProc> fTransformPath;
    GLPtr<GrGLPathParameterivProc> fPathParameteriv;
    GLPtr<GrGLPathParameteriProc> fPathParameteri;
    GLPtr<GrGLPathParameterfvProc> fPathParameterfv;
    GLPtr<GrGLPathParameterfProc> fPathParameterf;
    GLPtr<GrGLPathDashArrayProc> fPathDashArray;
    GLPtr<GrGLGenPathsProc> fGenPaths;
    GLPtr<GrGLDeletePathsProc> fDeletePaths;
    GLPtr<GrGLIsPathProc> fIsPath;
    GLPtr<GrGLPathStencilFuncProc> fPathStencilFunc;
    GLPtr<GrGLPathStencilDepthOffsetProc> fPathStencilDepthOffset;
    GLPtr<GrGLStencilFillPathProc> fStencilFillPath;
    GLPtr<GrGLStencilStrokePathProc> fStencilStrokePath;
    GLPtr<GrGLStencilFillPathInstancedProc> fStencilFillPathInstanced;
    GLPtr<GrGLStencilStrokePathInstancedProc> fStencilStrokePathInstanced;
    GLPtr<GrGLPathCoverDepthFuncProc> fPathCoverDepthFunc;
    GLPtr<GrGLPathColorGenProc> fPathColorGen;
    GLPtr<GrGLPathTexGenProc> fPathTexGen;
    GLPtr<GrGLPathFogGenProc> fPathFogGen;
    GLPtr<GrGLCoverFillPathProc> fCoverFillPath;
    GLPtr<GrGLCoverStrokePathProc> fCoverStrokePath;
    GLPtr<GrGLCoverFillPathInstancedProc> fCoverFillPathInstanced;
    GLPtr<GrGLCoverStrokePathInstancedProc> fCoverStrokePathInstanced;
    GLPtr<GrGLGetPathParameterivProc> fGetPathParameteriv;
    GLPtr<GrGLGetPathParameterfvProc> fGetPathParameterfv;
    GLPtr<GrGLGetPathCommandsProc> fGetPathCommands;
    GLPtr<GrGLGetPathCoordsProc> fGetPathCoords;
    GLPtr<GrGLGetPathDashArrayProc> fGetPathDashArray;
    GLPtr<GrGLGetPathMetricsProc> fGetPathMetrics;
    GLPtr<GrGLGetPathMetricRangeProc> fGetPathMetricRange;
    GLPtr<GrGLGetPathSpacingProc> fGetPathSpacing;
    GLPtr<GrGLGetPathColorGenivProc> fGetPathColorGeniv;
    GLPtr<GrGLGetPathColorGenfvProc> fGetPathColorGenfv;
    GLPtr<GrGLGetPathTexGenivProc> fGetPathTexGeniv;
    GLPtr<GrGLGetPathTexGenfvProc> fGetPathTexGenfv;
    GLPtr<GrGLIsPointInFillPathProc> fIsPointInFillPath;
    GLPtr<GrGLIsPointInStrokePathProc> fIsPointInStrokePath;
    GLPtr<GrGLGetPathLengthProc> fGetPathLength;
    GLPtr<GrGLPointAlongPathProc> fPointAlongPath;

    
#if GR_GL_PER_GL_FUNC_CALLBACK
    GrGLInterfaceCallbackProc fCallback;
    GrGLInterfaceCallbackData fCallbackData;
#endif

};

#endif
