



#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"

#define NS_ISCRIPTRUNTIME_IID \
{ 0x41ded433, 0x83a5, 0x43fb, \
  { 0x85, 0xf4, 0x33, 0xba, 0x8f, 0xd1, 0x3f, 0xdc } }






class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)

  
  virtual already_AddRefed<nsIScriptContext>
  CreateContext(bool aGCOnDestruction,
                nsIScriptGlobalObject* aGlobalObject) = 0;
  
  



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
