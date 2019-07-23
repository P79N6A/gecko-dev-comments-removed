




































#ifndef nsIScriptObjectOwner_h__
#define nsIScriptObjectOwner_h__

#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsAString.h"

class nsScriptObjectHolder;

#define NS_ISCRIPTOBJECTOWNER_IID \
{ /* 8f6bca7e-ce42-11d1-b724-00600891d8c9 */ \
0x8f6bca7e, 0xce42, 0x11d1, \
  {0xb7, 0x24, 0x00, 0x60, 0x08, 0x91, 0xd8, 0xc9} } \











class nsIScriptObjectOwner : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTOBJECTOWNER_IID)

  









  NS_IMETHOD GetScriptObject(nsIScriptContext *aContext, void** aScriptObject) = 0;

  









  NS_IMETHOD SetScriptObject(void* aScriptObject) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptObjectOwner,
                              NS_ISCRIPTOBJECTOWNER_IID)

class nsIAtom;

#define NS_ISCRIPTEVENTHANDLEROWNER_IID \
{ /* 2ad54ae0-a839-11d3-ba97-00104ba02d3d */ \
0x2ad54ae0, 0xa839, 0x11d3, \
  {0xba, 0x97, 0x00, 0x10, 0x4b, 0xa0, 0x2d, 0x3d} }







class nsIScriptEventHandlerOwner : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTEVENTHANDLEROWNER_IID)

  











  virtual nsresult CompileEventHandler(nsIScriptContext* aContext,
                                       nsISupports* aTarget,
                                       nsIAtom *aName,
                                       const nsAString& aBody,
                                       const char* aURL,
                                       PRUint32 aLineNo,
                                       nsScriptObjectHolder &aHandler) = 0;

  






  virtual nsresult GetCompiledEventHandler(nsIAtom *aName,
                                           nsScriptObjectHolder &aHandler) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptEventHandlerOwner,
                              NS_ISCRIPTEVENTHANDLEROWNER_IID)

#endif 
