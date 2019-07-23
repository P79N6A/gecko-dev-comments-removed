



































#ifndef nsIPluginInstanceInternal_h___
#define nsIPluginInstanceInternal_h___

#include "nsISupports.h"

struct JSObject;
struct JSContext;

#define NS_IPLUGININSTANCEINTERNAL_IID \
  { 0x301f13ed, 0x50f2, 0x4ed2, \
    { 0x83, 0x0d, 0x78, 0x36, 0x1d, 0x01, 0x76, 0xaf }}

class NS_NO_VTABLE nsIPluginInstanceInternal : public nsISupports
{
public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPLUGININSTANCEINTERNAL_IID)

  virtual JSObject *GetJSObject(JSContext *cx) = 0;

  virtual nsresult GetFormValue(nsAString& aValue) = 0;

  virtual void PushPopupsEnabledState(PRBool aEnabled) = 0;
  virtual void PopPopupsEnabledState() = 0;

  virtual PRUint16 GetPluginAPIVersion() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPluginInstanceInternal,
                              NS_IPLUGININSTANCEINTERNAL_IID)

#endif 
