




#ifndef nsIScriptEventHandlerOwner_h__
#define nsIScriptEventHandlerOwner_h__

#include "nsISupports.h"
#include "nsIScriptContext.h"
#include "nsAString.h"

template<class> class nsScriptObjectHolder;

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
