





































#ifndef __NSDOMWORKERSECURITYMANAGER_H__
#define __NSDOMWORKERSECURITYMANAGER_H__

#include "nsIXPCSecurityManager.h"
#include "jsapi.h"

class nsDOMWorkerSecurityManager : public nsIXPCSecurityManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSECURITYMANAGER

  static JSPrincipals* WorkerPrincipal();

  static JSBool JSCheckAccess(JSContext* aCx, JSObject* aObj, jsid aId,
                              JSAccessMode aMode, jsval* aVp);

  static JSPrincipals* JSFindPrincipal(JSContext* aCx, JSObject* aObj);

  static JSBool JSTranscodePrincipals(JSXDRState* aXdr,
                                      JSPrincipals** aJsprinp);
};

#endif 
