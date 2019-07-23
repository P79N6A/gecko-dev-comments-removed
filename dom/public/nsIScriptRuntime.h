



































#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"


#define NS_ISCRIPTRUNTIME_IID \
{ 0x47032a4d, 0xc22, 0x4125, { 0x94, 0xb7, 0x86, 0x4a, 0x4b, 0x74, 0x43, 0x35 } }







class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)
  


  virtual PRUint32 GetScriptTypeID() = 0;

  


  virtual void ShutDown() = 0;

  




  virtual nsresult ParseVersion(const nsString &aVersionStr, PRUint32 *verFlags) = 0;
  
  
  virtual nsresult CreateContext(nsIScriptContext **ret) = 0;
  
  



  virtual nsresult DropScriptObject(void *object) = 0;
  virtual nsresult HoldScriptObject(void *object) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptRuntime, NS_ISCRIPTRUNTIME_IID)


nsresult NS_GetScriptRuntime(const nsAString &aLanguageName,
                             nsIScriptRuntime **aRuntime);

nsresult NS_GetScriptRuntimeByID(PRUint32 aLanguageID,
                                 nsIScriptRuntime **aRuntime);

#endif 
