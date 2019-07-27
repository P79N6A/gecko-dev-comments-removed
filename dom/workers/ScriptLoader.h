





#ifndef mozilla_dom_workers_scriptloader_h__
#define mozilla_dom_workers_scriptloader_h__

#include "Workers.h"

class nsIPrincipal;
class nsIURI;
class nsIDocument;
class nsILoadGroup;
class nsString;
class nsIChannel;

namespace mozilla {

class ErrorResult;

} 

BEGIN_WORKERS_NAMESPACE

enum WorkerScriptType {
  WorkerScript,
  DebuggerScript
};

namespace scriptloader {

nsresult
ChannelFromScriptURLMainThread(nsIPrincipal* aPrincipal,
                               nsIURI* aBaseURI,
                               nsIDocument* aParentDoc,
                               nsILoadGroup* aLoadGroup,
                               const nsAString& aScriptURL,
                               nsIChannel** aChannel);

nsresult
ChannelFromScriptURLWorkerThread(JSContext* aCx,
                                 WorkerPrivate* aParent,
                                 const nsAString& aScriptURL,
                                 nsIChannel** aChannel);

void ReportLoadError(JSContext* aCx, const nsAString& aURL,
                     nsresult aLoadResult, bool aIsMainThread);

bool LoadMainScript(JSContext* aCx, const nsAString& aScriptURL,
                    WorkerScriptType aWorkerScriptType);

void Load(JSContext* aCx,
          WorkerPrivate* aWorkerPrivate,
          const nsTArray<nsString>& aScriptURLs,
          WorkerScriptType aWorkerScriptType,
          mozilla::ErrorResult& aRv);

} 

END_WORKERS_NAMESPACE

#endif
