





#ifndef mozilla_dom_workers_url_h__
#define mozilla_dom_workers_url_h__

#include "mozilla/dom/URLBinding.h"

#include "EventTarget.h"

BEGIN_WORKERS_NAMESPACE

class URL : public EventTarget
{
public: 
  static void
  CreateObjectURL(const GlobalObject& aGlobal,
                  JSObject* aArg, const objectURLOptions& aOptions,
                  nsString& aResult, ErrorResult& aRv);

  static void
  CreateObjectURL(const GlobalObject& aGlobal,
                  JSObject& aArg, const objectURLOptions& aOptions,
                  nsString& aResult, ErrorResult& aRv);

  static void
  RevokeObjectURL(const GlobalObject& aGlobal, const nsAString& aUrl);
};

END_WORKERS_NAMESPACE

#endif 
