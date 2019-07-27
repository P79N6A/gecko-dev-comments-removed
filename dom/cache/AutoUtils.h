





#ifndef mozilla_dom_cache_AutoUtils_h
#define mozilla_dom_cache_AutoUtils_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/cache/PCacheTypes.h"
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
struct SavedRequest;
struct SavedResponse;
class StreamList;









class MOZ_STACK_CLASS AutoChildBase
{
protected:
  typedef TypeUtils::BodyAction BodyAction;
  typedef TypeUtils::ReferrerAction ReferrerAction;
  typedef TypeUtils::SchemeAction SchemeAction;

  AutoChildBase(TypeUtils* aTypeUtils);
  virtual ~AutoChildBase() = 0;

  TypeUtils* mTypeUtils;
  bool mSent;
};

class MOZ_STACK_CLASS AutoChildRequest final : public AutoChildBase
{
public:
  explicit AutoChildRequest(TypeUtils* aTypeUtils);
  ~AutoChildRequest();

  void Add(InternalRequest* aRequest, BodyAction aBodyAction,
           ReferrerAction aReferrerAction, SchemeAction aSchemeAction,
           ErrorResult& aRv);

  const PCacheRequest& SendAsRequest();
  const PCacheRequestOrVoid& SendAsRequestOrVoid();

private:
  PCacheRequestOrVoid mRequestOrVoid;
};

class MOZ_STACK_CLASS AutoChildRequestList final : public AutoChildBase
{
public:
  AutoChildRequestList(TypeUtils* aTypeUtils, uint32_t aCapacity);
  ~AutoChildRequestList();

  void Add(InternalRequest* aRequest, BodyAction aBodyAction,
           ReferrerAction aReferrerAction, SchemeAction aSchemeAction,
           ErrorResult& aRv);

  const nsTArray<PCacheRequest>& SendAsRequestList();

private:
  
  nsAutoTArray<PCacheRequest, 32> mRequestList;
};

class MOZ_STACK_CLASS AutoChildRequestResponse final : public AutoChildBase
{
public:
  explicit AutoChildRequestResponse(TypeUtils* aTypeUtils);
  ~AutoChildRequestResponse();

  void Add(InternalRequest* aRequest, BodyAction aBodyAction,
           ReferrerAction aReferrerAction, SchemeAction aSchemeAction,
           ErrorResult& aRv);
  void Add(Response& aResponse, ErrorResult& aRv);

  const CacheRequestResponse& SendAsRequestResponse();

private:
  CacheRequestResponse mRequestResponse;
};

class MOZ_STACK_CLASS AutoParentBase
{
protected:
  explicit AutoParentBase(mozilla::ipc::PBackgroundParent* aManager);
  virtual ~AutoParentBase() = 0;

  void SerializeReadStream(const nsID& aId, StreamList* aStreamList,
                           PCacheReadStream* aReadStreamOut);

  mozilla::ipc::PBackgroundParent* mManager;
  CacheStreamControlParent* mStreamControl;
  bool mSent;
};

class MOZ_STACK_CLASS AutoParentRequestList final : public AutoParentBase
{
public:
  AutoParentRequestList(mozilla::ipc::PBackgroundParent* aManager,
                        uint32_t aCapacity);
  ~AutoParentRequestList();

  void Add(const SavedRequest& aSavedRequest, StreamList* aStreamList);

  const nsTArray<PCacheRequest>& SendAsRequestList();

private:
  
  nsAutoTArray<PCacheRequest, 32> mRequestList;
};

class MOZ_STACK_CLASS AutoParentResponseList final : public AutoParentBase
{
public:
  AutoParentResponseList(mozilla::ipc::PBackgroundParent* aManager,
                         uint32_t aCapacity);
  ~AutoParentResponseList();

  void Add(const SavedResponse& aSavedResponse, StreamList* aStreamList);

  const nsTArray<PCacheResponse>& SendAsResponseList();

private:
  
  nsAutoTArray<PCacheResponse, 32> mResponseList;
};

class MOZ_STACK_CLASS AutoParentResponseOrVoid final : public AutoParentBase
{
public:
  explicit AutoParentResponseOrVoid(mozilla::ipc::PBackgroundParent* aManager);
  ~AutoParentResponseOrVoid();

  void Add(const SavedResponse& aSavedResponse, StreamList* aStreamList);

  const PCacheResponseOrVoid& SendAsResponseOrVoid();

private:
  PCacheResponseOrVoid mResponseOrVoid;
};

} 
} 
} 

#endif 
