





#ifndef mozilla_dom_workers_url_h__
#define mozilla_dom_workers_url_h__

#include "mozilla/dom/URLBinding.h"

#include "EventTarget.h"

BEGIN_WORKERS_NAMESPACE

class URL : public EventTarget
{
public: 
  static void
  CreateObjectURL(const WorkerGlobalObject& aGlobal,
                  JSObject* aArg, const objectURLOptionsWorkers& aOptions,
                  nsString& aResult, ErrorResult& aRv);

  static void
  CreateObjectURL(const WorkerGlobalObject& aGlobal,
                  JSObject& aArg, const objectURLOptionsWorkers& aOptions,
                  nsString& aResult, ErrorResult& aRv);

  static void
  RevokeObjectURL(const WorkerGlobalObject& aGlobal, const nsAString& aUrl);
};

END_WORKERS_NAMESPACE

#endif 
