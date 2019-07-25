



































#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"

#define NS_ISCRIPTRUNTIME_IID \
{ 0xb146580f, 0x55f7, 0x4d97, \
  { 0x8a, 0xbb, 0x4a, 0x50, 0xb0, 0xa8, 0x04, 0x97 } }






class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)

  




  virtual nsresult ParseVersion(const nsString &aVersionStr, PRUint32 *verFlags) = 0;
  
  
  virtual already_AddRefed<nsIScriptContext> CreateContext() = 0;
  
  



  virtual nsresult DropScriptObject(void *object) = 0;
  virtual nsresult HoldScriptObject(void *object) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptRuntime, NS_ISCRIPTRUNTIME_IID)


nsresult NS_GetJSRuntime(nsIScriptRuntime** aLanguage);

nsresult NS_GetScriptRuntime(const nsAString &aLanguageName,
                             nsIScriptRuntime **aRuntime);

nsresult NS_GetScriptRuntimeByID(PRUint32 aLanguageID,
                                 nsIScriptRuntime **aRuntime);

#endif 
