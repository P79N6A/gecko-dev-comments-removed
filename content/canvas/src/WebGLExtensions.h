




#ifndef WEBGLEXTENSIONS_H_
#define WEBGLEXTENSIONS_H_

namespace mozilla {

class WebGLContext;

class WebGLExtensionBase
    : public nsWrapperCache
    , public WebGLContextBoundObject
{
public:
    WebGLExtensionBase(WebGLContext*);
    virtual ~WebGLExtensionBase();

    WebGLContext *GetParentObject() const {
        return Context();
    }

    NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(WebGLExtensionBase)
    NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(WebGLExtensionBase)
};

#define DECL_WEBGL_EXTENSION_GOOP                                           \
    virtual JSObject* WrapObject(JSContext *cx,                             \
                                 JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

#define IMPL_WEBGL_EXTENSION_GOOP(WebGLExtensionType) \
    JSObject* \
    WebGLExtensionType::WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) { \
        return dom::WebGLExtensionType##Binding::Wrap(cx, scope, this); \
    }

class WebGLExtensionCompressedTextureATC
    : public WebGLExtensionBase
{
public:
    WebGLExtensionCompressedTextureATC(WebGLContext*);
    virtual ~WebGLExtensionCompressedTextureATC();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionCompressedTexturePVRTC
    : public WebGLExtensionBase
{
public:
    WebGLExtensionCompressedTexturePVRTC(WebGLContext*);
    virtual ~WebGLExtensionCompressedTexturePVRTC();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionCompressedTextureS3TC
    : public WebGLExtensionBase
{
public:
    WebGLExtensionCompressedTextureS3TC(WebGLContext*);
    virtual ~WebGLExtensionCompressedTextureS3TC();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionDebugRendererInfo
    : public WebGLExtensionBase
{
public:
    WebGLExtensionDebugRendererInfo(WebGLContext*);
    virtual ~WebGLExtensionDebugRendererInfo();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionDepthTexture
    : public WebGLExtensionBase
{
public:
    WebGLExtensionDepthTexture(WebGLContext*);
    virtual ~WebGLExtensionDepthTexture();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionElementIndexUint
    : public WebGLExtensionBase
{
public:
    WebGLExtensionElementIndexUint(WebGLContext*);
    virtual ~WebGLExtensionElementIndexUint();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionLoseContext
    : public WebGLExtensionBase
{
public:
    WebGLExtensionLoseContext(WebGLContext*);
    virtual ~WebGLExtensionLoseContext();

    void LoseContext();
    void RestoreContext();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionStandardDerivatives
    : public WebGLExtensionBase
{
public:
    WebGLExtensionStandardDerivatives(WebGLContext*);
    virtual ~WebGLExtensionStandardDerivatives();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionTextureFilterAnisotropic
    : public WebGLExtensionBase
{
public:
    WebGLExtensionTextureFilterAnisotropic(WebGLContext*);
    virtual ~WebGLExtensionTextureFilterAnisotropic();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionTextureFloat
    : public WebGLExtensionBase
{
public:
    WebGLExtensionTextureFloat(WebGLContext*);
    virtual ~WebGLExtensionTextureFloat();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionTextureFloatLinear
    : public WebGLExtensionBase
{
public:
    WebGLExtensionTextureFloatLinear(WebGLContext*);
    virtual ~WebGLExtensionTextureFloatLinear();

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionDrawBuffers
    : public WebGLExtensionBase
{
public:
    WebGLExtensionDrawBuffers(WebGLContext*);
    virtual ~WebGLExtensionDrawBuffers();

    void DrawBuffersWEBGL(const dom::Sequence<GLenum>& buffers);

    static bool IsSupported(const WebGLContext*);

    static const size_t sMinColorAttachments = 4;
    static const size_t sMinDrawBuffers = 4;
    





    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionVertexArray
    : public WebGLExtensionBase
{
public:
    WebGLExtensionVertexArray(WebGLContext*);
    virtual ~WebGLExtensionVertexArray();

    already_AddRefed<WebGLVertexArray> CreateVertexArrayOES();
    void DeleteVertexArrayOES(WebGLVertexArray* array);
    bool IsVertexArrayOES(WebGLVertexArray* array);
    void BindVertexArrayOES(WebGLVertexArray* array);

    static bool IsSupported(const WebGLContext* context);

    DECL_WEBGL_EXTENSION_GOOP
};

class WebGLExtensionInstancedArrays
    : public WebGLExtensionBase
{
public:
    WebGLExtensionInstancedArrays(WebGLContext* context);
    virtual ~WebGLExtensionInstancedArrays();

    void DrawArraysInstancedANGLE(GLenum mode, GLint first,
                                  GLsizei count, GLsizei primcount);
    void DrawElementsInstancedANGLE(GLenum mode, GLsizei count,
                                    GLenum type, WebGLintptr offset,
                                    GLsizei primcount);
    void VertexAttribDivisorANGLE(GLuint index, GLuint divisor);

    static bool IsSupported(const WebGLContext* context);

    DECL_WEBGL_EXTENSION_GOOP
};

} 

#endif
