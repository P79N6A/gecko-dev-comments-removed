





#ifndef mozilla_DebuggerOnGCRunnable_h
#define mozilla_DebuggerOnGCRunnable_h

#include "nsThreadUtils.h"
#include "js/GCAPI.h"
#include "mozilla/Move.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {


class DebuggerOnGCRunnable : public nsCancelableRunnable
{
  JS::dbg::GarbageCollectionEvent::Ptr mGCData;

  explicit DebuggerOnGCRunnable(JS::dbg::GarbageCollectionEvent::Ptr&& aGCData)
    : mGCData(Move(aGCData))
  { }

public:
  static NS_METHOD Enqueue(JSRuntime* aRt, const JS::GCDescription& aDesc);

  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICANCELABLERUNNABLE
};

} 

#endif
