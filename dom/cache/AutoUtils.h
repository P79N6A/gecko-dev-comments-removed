





#ifndef mozilla_dom_cache_AutoUtils_h
#define mozilla_dom_cache_AutoUtils_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/CacheTypes.h"
#include "mozilla/dom/cache/Types.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "nsTArray.h"

struct nsID;

namespace mozilla {

class ErrorResult;

namespace ipc {
class PBackgroundParent;
}

namespace dom {

class InternalRequest;

namespace cache {

class CacheStreamControlParent;
class Manager;
struct SavedRequest;
struct SavedResponse;
class StreamList;









class MOZ_STACK_CLASS AutoChildOpArgs final
{
public:
  typedef TypeUtils::BodyAction BodyAction;
  typedef TypeUtils::SchemeAction SchemeAction;

  AutoChildOpArgs(TypeUtils* aTypeUtils, const CacheOpArgs& aOpArgs);
  ~AutoChildOpArgs();

  void Add(InternalRequest* aRequest, BodyAction aBodyAction,
           SchemeAction aSchemeAction, ErrorResult& aRv);
  void Add(InternalRequest* aRequest, BodyAction aBodyAction,
           SchemeAction aSchemeAction, Response& aResponse, ErrorResult& aRv);

  const CacheOpArgs& SendAsOpArgs();

private:
  TypeUtils* mTypeUtils;
  CacheOpArgs mOpArgs;
  bool mSent;
};

class MOZ_STACK_CLASS AutoParentOpResult final
{
public:
  AutoParentOpResult(mozilla::ipc::PBackgroundParent* aManager,
                     const CacheOpResult& aOpResult);
  ~AutoParentOpResult();

  void Add(CacheId aOpenedCacheId, Manager* aManager);
  void Add(const SavedResponse& aSavedResponse, StreamList* aStreamList);
  void Add(const SavedRequest& aSavedRequest, StreamList* aStreamList);

  const CacheOpResult& SendAsOpResult();

private:
  void SerializeResponseBody(const SavedResponse& aSavedResponse,
                             StreamList* aStreamList,
                             CacheResponse* aResponseOut);

  void SerializeReadStream(const nsID& aId, StreamList* aStreamList,
                           CacheReadStream* aReadStreamOut);

  mozilla::ipc::PBackgroundParent* mManager;
  CacheOpResult mOpResult;
  CacheStreamControlParent* mStreamControl;
  bool mSent;
};

} 
} 
} 

#endif 
