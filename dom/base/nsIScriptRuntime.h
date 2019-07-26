



#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"

#define NS_ISCRIPTRUNTIME_IID \
{ 0x80f87f51, 0x626d, 0x4311, \
  { 0x8c, 0x4e, 0x31, 0x37, 0xcf, 0x52, 0x1e, 0x9b } }






class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)

  
  virtual already_AddRefed<nsIScriptContext>
  CreateContext(nsIScriptGlobalObject* aGlobalObject) = 0;
  
  



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
