






































#include "nsServiceManagerUtils.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"

#ifndef MOZILLA_1_8_BRANCH
#include "nsIClassInfoImpl.h"
#endif

#include "nsICanvasRenderingContextGLES11.h"
#include "nsICanvasRenderingContextGLWeb20.h"


#define NS_CANVASRENDERINGCONTEXTGLES11_CID \
{ 0x21788585, 0xe91c, 0x448c, { 0x8b, 0xb9, 0x6c, 0x51, 0xab, 0xa9, 0x73, 0x5f } }


#define NS_CANVASRENDERINGCONTEXTGLWEB20_CID \
{ 0x8cb888f5, 0xd754, 0x4e5b, { 0x8c, 0x72, 0xa5, 0xcc, 0x79, 0x8c, 0xbb, 0x8f } }

nsresult NS_NewCanvasRenderingContextGLES11(nsICanvasRenderingContextGLES11** aResult);

nsresult NS_NewCanvasRenderingContextGLWeb20(nsICanvasRenderingContextGLWeb20** aResult);

#define MAKE_CTOR(ctor_, iface_, func_)                   \
static NS_IMETHODIMP                                      \
ctor_(nsISupports* aOuter, REFNSIID aIID, void** aResult) \
{                                                         \
  *aResult = nsnull;                                      \
  if (aOuter)                                             \
    return NS_ERROR_NO_AGGREGATION;                       \
  iface_* inst;                                           \
  nsresult rv = func_(&inst);                             \
  if (NS_SUCCEEDED(rv)) {                                 \
    rv = inst->QueryInterface(aIID, aResult);             \
    NS_RELEASE(inst);                                     \
  }                                                       \
  return rv;                                              \
}

NS_DECL_CLASSINFO(nsCanvasRenderingContextGLES11)
MAKE_CTOR(CreateCanvasRenderingContextGLES11, nsICanvasRenderingContextGLES11, NS_NewCanvasRenderingContextGLES11)

NS_DECL_CLASSINFO(nsCanvasRenderingContextGLWeb20)
MAKE_CTOR(CreateCanvasRenderingContextGLWeb20, nsICanvasRenderingContextGLWeb20, NS_NewCanvasRenderingContextGLWeb20)

static const nsModuleComponentInfo components[] = {
    { "GLES 1.1 Canvas Context",
      NS_CANVASRENDERINGCONTEXTGLES11_CID,
      "@mozilla.org/content/canvas-rendering-context;1?id=moz-gles11",
      CreateCanvasRenderingContextGLES11,
      nsnull, nsnull, nsnull,
      NS_CI_INTERFACE_GETTER_NAME(nsCanvasRenderingContextGLES11),
      nsnull, &NS_CLASSINFO_NAME(nsCanvasRenderingContextGLES11),
      nsIClassInfo::DOM_OBJECT
    },
    { "GLES 2.0 Canvas Context",
      NS_CANVASRENDERINGCONTEXTGLWEB20_CID,
      "@mozilla.org/content/canvas-rendering-context;1?id=moz-glweb20",
      CreateCanvasRenderingContextGLWeb20,
      nsnull, nsnull, nsnull,
      NS_CI_INTERFACE_GETTER_NAME(nsCanvasRenderingContextGLWeb20),
      nsnull, &NS_CLASSINFO_NAME(nsCanvasRenderingContextGLWeb20),
      nsIClassInfo::DOM_OBJECT
    },
};

NS_IMPL_NSGETMODULE(nsCanvas3DModule, components)

