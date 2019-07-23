



































#ifndef nsIPluginInstanceInternal_h___
#define nsIPluginInstanceInternal_h___

#include "nsISupports.h"

struct JSObject;
struct JSContext;

#define NPRUNTIME_JSCLASS_NAME "NPObject JS wrapper class"

#define NS_IPLUGININSTANCEINTERNAL_IID \
  {0x1a9c2ae8, 0xab75, 0x4296, \
    { 0xaf, 0xcb, 0x39, 0x54, 0x39, 0x96, 0x06, 0xa9 }}

class NS_NO_VTABLE nsIPluginInstanceInternal : public nsISupports
{
public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGININSTANCEINTERNAL_IID)

  virtual JSObject *GetJSObject(JSContext *cx) = 0;

  virtual nsresult GetFormValue(nsAString& aValue) = 0;

  virtual void PushPopupsEnabledState(PRBool aEnabled) = 0;
  virtual void PopPopupsEnabledState() = 0;

  virtual PRUint16 GetPluginAPIVersion() = 0;

  virtual void DefineJavaProperties() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginInstanceInternal,
                              NS_IPLUGININSTANCEINTERNAL_IID)

#endif 
