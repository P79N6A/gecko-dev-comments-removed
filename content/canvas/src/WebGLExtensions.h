




#ifndef WEBGLEXTENSIONS_H_
#define WEBGLEXTENSIONS_H_

namespace mozilla {

class WebGLExtensionLoseContext :
    public nsIWebGLExtensionLoseContext,
    public WebGLExtension
{
public:
    WebGLExtensionLoseContext(WebGLContext*);
    virtual ~WebGLExtensionLoseContext();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSIONLOSECONTEXT
};

class WebGLExtensionStandardDerivatives :
    public nsIWebGLExtensionStandardDerivatives,
    public WebGLExtension
{
public:
    WebGLExtensionStandardDerivatives(WebGLContext* context);
    virtual ~WebGLExtensionStandardDerivatives();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

class WebGLExtensionTextureFilterAnisotropic :
    public nsIWebGLExtensionTextureFilterAnisotropic,
    public WebGLExtension
{
public:
    WebGLExtensionTextureFilterAnisotropic(WebGLContext* context);
    virtual ~WebGLExtensionTextureFilterAnisotropic();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

class WebGLExtensionCompressedTextureS3TC :
    public nsIWebGLExtensionCompressedTextureS3TC,
    public WebGLExtension
{
public:
    WebGLExtensionCompressedTextureS3TC(WebGLContext* context);
    virtual ~WebGLExtensionCompressedTextureS3TC();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

class WebGLExtensionDepthTexture :
    public nsIWebGLExtensionDepthTexture,
    public WebGLExtension
{
public:
    WebGLExtensionDepthTexture(WebGLContext* context);
    virtual ~WebGLExtensionDepthTexture();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIWEBGLEXTENSION
};

}

#endif
