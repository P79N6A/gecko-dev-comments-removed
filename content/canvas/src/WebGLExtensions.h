





































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

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLEXTENSIONLOSECONTEXT
};

class WebGLExtensionStandardDerivatives :
    public nsIWebGLExtensionStandardDerivatives,
    public WebGLExtension
{
public:
    WebGLExtensionStandardDerivatives(WebGLContext* context);
    virtual ~WebGLExtensionStandardDerivatives();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLEXTENSION
};

class WebGLExtensionTextureFilterAnisotropic :
    public nsIWebGLExtensionTextureFilterAnisotropic,
    public WebGLExtension
{
public:
    WebGLExtensionTextureFilterAnisotropic(WebGLContext* context);
    virtual ~WebGLExtensionTextureFilterAnisotropic();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLEXTENSION
};

}

#endif
