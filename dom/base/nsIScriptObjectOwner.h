




































#ifndef nsIScriptObjectOwner_h__
#define nsIScriptObjectOwner_h__

#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsAString.h"

template<class> class nsScriptObjectHolder;

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
{ 0xc8f35f71, 0x07d1, 0x4ff3, \
  { 0xa3, 0x2f, 0x65, 0xcb, 0x35, 0x64, 0xac, 0xe0 } }







class nsIScriptEventHandlerOwner : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTEVENTHANDLEROWNER_IID)

  










  virtual nsresult CompileEventHandler(nsIScriptContext* aContext,
                                       nsIAtom *aName,
                                       const nsAString& aBody,
                                       const char* aURL,
                                       PRUint32 aLineNo,
                                       nsScriptObjectHolder<JSObject>& aHandler) = 0;

  






  virtual nsresult GetCompiledEventHandler(nsIAtom *aName,
                                           nsScriptObjectHolder<JSObject>& aHandler) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptEventHandlerOwner,
                              NS_ISCRIPTEVENTHANDLEROWNER_IID)

#endif 
