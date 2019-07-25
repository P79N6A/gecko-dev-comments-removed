




































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
{ 0x1e2be5d2, 0x381a, 0x46dc, \
 { 0xae, 0x97, 0xa5, 0x5f, 0x45, 0xfd, 0x36, 0x63 } }







class nsIScriptEventHandlerOwner : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTEVENTHANDLEROWNER_IID)

  










  virtual nsresult CompileEventHandler(nsIScriptContext* aContext,
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
