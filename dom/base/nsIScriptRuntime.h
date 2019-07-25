



#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"

#define NS_ISCRIPTRUNTIME_IID \
{ 0xfa30d7a8, 0x7f0a, 0x437a, \
  { 0xa1, 0x0c, 0xc2, 0xbe, 0xa3, 0xdb, 0x4f, 0x4b } }






class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)

  
  virtual already_AddRefed<nsIScriptContext> CreateContext() = 0;
  
  



  virtual nsresult DropScriptObject(void *object) = 0;
  virtual nsresult HoldScriptObject(void *object) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptRuntime, NS_ISCRIPTRUNTIME_IID)


nsresult NS_GetJSRuntime(nsIScriptRuntime** aLanguage);

nsresult NS_GetScriptRuntime(const nsAString &aLanguageName,
                             nsIScriptRuntime **aRuntime);

nsresult NS_GetScriptRuntimeByID(uint32_t aLanguageID,
                                 nsIScriptRuntime **aRuntime);

#endif 
