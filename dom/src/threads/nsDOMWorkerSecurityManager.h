





































#ifndef __NSDOMWORKERSECURITYMANAGER_H__
#define __NSDOMWORKERSECURITYMANAGER_H__

#include "nsIXPCSecurityManager.h"
#include "jsapi.h"

class nsDOMWorkerSecurityManager : public nsIXPCSecurityManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSECURITYMANAGER

  static JSBool JSCheckAccess(JSContext *cx, JSObject *obj, jsval id,
                              JSAccessMode mode, jsval *vp);

};

#endif 
