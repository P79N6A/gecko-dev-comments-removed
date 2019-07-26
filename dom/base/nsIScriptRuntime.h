



#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"

#define NS_ISCRIPTRUNTIME_IID \
{ 0x86c6b54a, 0xf067, 0x4878, \
  { 0xb2, 0x77, 0x4e, 0xee, 0x95, 0xda, 0x8f, 0x76 } }






class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)

  
  virtual already_AddRefed<nsIScriptContext>
  CreateContext(bool aGCOnDestruction,
                nsIScriptGlobalObject* aGlobalObject) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptRuntime, NS_ISCRIPTRUNTIME_IID)


nsresult NS_GetJSRuntime(nsIScriptRuntime** aLanguage);

nsresult NS_GetScriptRuntime(const nsAString &aLanguageName,
                             nsIScriptRuntime **aRuntime);

nsresult NS_GetScriptRuntimeByID(uint32_t aLanguageID,
                                 nsIScriptRuntime **aRuntime);

#endif 
