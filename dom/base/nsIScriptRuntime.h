



































#ifndef nsIScriptRuntime_h__
#define nsIScriptRuntime_h__

#include "nsIScriptContext.h"

#define NS_ISCRIPTRUNTIME_IID \
{ 0x2c8d774e, 0xb52a, 0x43ec, \
  { 0x8e, 0xbc, 0x82, 0x75, 0xb9, 0x34, 0x20, 0x57 } }






class nsIScriptRuntime : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCRIPTRUNTIME_IID)
  


  virtual PRUint32 GetScriptTypeID() = 0;

  




  virtual nsresult ParseVersion(const nsString &aVersionStr, PRUint32 *verFlags) = 0;
  
  
  virtual already_AddRefed<nsIScriptContext> CreateContext() = 0;
  
  



  virtual nsresult DropScriptObject(void *object) = 0;
  virtual nsresult HoldScriptObject(void *object) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScriptRuntime, NS_ISCRIPTRUNTIME_IID)


nsresult NS_GetScriptRuntime(const nsAString &aLanguageName,
                             nsIScriptRuntime **aRuntime);

nsresult NS_GetScriptRuntimeByID(PRUint32 aLanguageID,
                                 nsIScriptRuntime **aRuntime);

#endif 
