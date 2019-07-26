




#ifndef mozilla_dom_workers_scriptloader_h__
#define mozilla_dom_workers_scriptloader_h__

#include "Workers.h"

#include "jsapi.h"

class nsIPrincipal;
class nsIURI;
class nsIDocument;
class nsString;
class nsIChannel;

BEGIN_WORKERS_NAMESPACE

namespace scriptloader {

nsresult
ChannelFromScriptURLMainThread(nsIPrincipal* aPrincipal,
                               nsIURI* aBaseURI,
                               nsIDocument* aParentDoc,
                               const nsString& aScriptURL,
                               nsIChannel** aChannel);

nsresult
ChannelFromScriptURLWorkerThread(JSContext* aCx,
                                 WorkerPrivate* aParent,
                                 const nsString& aScriptURL,
                                 nsIChannel** aChannel);

void ReportLoadError(JSContext* aCx, const nsString& aURL,
                     nsresult aLoadResult, bool aIsMainThread);

bool LoadWorkerScript(JSContext* aCx);

bool Load(JSContext* aCx, unsigned aURLCount, jsval* aURLs);

} 

END_WORKERS_NAMESPACE

#endif 
