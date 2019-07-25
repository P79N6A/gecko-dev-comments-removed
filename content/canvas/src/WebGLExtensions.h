





































#ifndef WEBGLEXTENSIONS_H_
#define WEBGLEXTENSIONS_H_

namespace mozilla {

class WebGLExtensionStandardDerivatives;

#define WEBGLEXTENSIONSTANDARDDERIVATIVES_PRIVATE_IID \
    {0x3de3dfd9, 0x864a, 0x4e4c, {0x98, 0x9b, 0x29, 0x77, 0xea, 0xa8, 0x0b, 0x7b}}
class WebGLExtensionStandardDerivatives :
    public nsIWebGLExtensionStandardDerivatives,
    public WebGLExtension
{
public:
    WebGLExtensionStandardDerivatives(WebGLContext* context);
    virtual ~WebGLExtensionStandardDerivatives();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBGLEXTENSION

    NS_DECLARE_STATIC_IID_ACCESSOR(WEBGLEXTENSIONSTANDARDDERIVATIVES_PRIVATE_IID)
};

NS_DEFINE_STATIC_IID_ACCESSOR(WebGLExtensionStandardDerivatives, WEBGLACTIVEINFO_PRIVATE_IID)

}

#endif
