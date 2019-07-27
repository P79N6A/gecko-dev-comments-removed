





#include "mozilla/dom/cache/StreamUtils.h"

#include "mozilla/unused.h"
#include "mozilla/dom/cache/CacheStreamControlChild.h"
#include "mozilla/dom/cache/PCacheTypes.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/FileDescriptorSetChild.h"

namespace mozilla {
namespace dom {
namespace cache {

namespace {

using mozilla::unused;
using mozilla::void_t;
using mozilla::dom::cache::CacheStreamControlChild;
using mozilla::dom::cache::Feature;
using mozilla::dom::cache::PCacheReadStream;
using mozilla::ipc::FileDescriptor;
using mozilla::ipc::FileDescriptorSetChild;
using mozilla::ipc::OptionalFileDescriptorSet;

void
StartDestroyStreamChild(const PCacheReadStream& aReadStream)
{
  CacheStreamControlChild* cacheControl =
    static_cast<CacheStreamControlChild*>(aReadStream.controlChild());
  if (cacheControl) {
    cacheControl->StartDestroy();
  }

  if (aReadStream.fds().type() ==
      OptionalFileDescriptorSet::TPFileDescriptorSetChild) {
    nsAutoTArray<FileDescriptor, 4> fds;

    FileDescriptorSetChild* fdSetActor =
      static_cast<FileDescriptorSetChild*>(aReadStream.fds().get_PFileDescriptorSetChild());
    MOZ_ASSERT(fdSetActor);

    fdSetActor->ForgetFileDescriptors(fds);
    MOZ_ASSERT(!fds.IsEmpty());

    unused << fdSetActor->Send__delete__(fdSetActor);
  }
}

void
AddFeatureToStreamChild(const PCacheReadStream& aReadStream, Feature* aFeature)
{
  CacheStreamControlChild* cacheControl =
    static_cast<CacheStreamControlChild*>(aReadStream.controlChild());
  if (cacheControl) {
    cacheControl->SetFeature(aFeature);
  }
}

} 

void
StartDestroyStreamChild(const PCacheResponseOrVoid& aResponseOrVoid)
{
  if (aResponseOrVoid.type() == PCacheResponseOrVoid::Tvoid_t) {
    return;
  }

  StartDestroyStreamChild(aResponseOrVoid.get_PCacheResponse());
}

void
StartDestroyStreamChild(const PCacheResponse& aResponse)
{
  if (aResponse.body().type() == PCacheReadStreamOrVoid::Tvoid_t) {
    return;
  }

  StartDestroyStreamChild(aResponse.body().get_PCacheReadStream());
}

void
StartDestroyStreamChild(const nsTArray<PCacheResponse>& aResponses)
{
  for (uint32_t i = 0; i < aResponses.Length(); ++i) {
    StartDestroyStreamChild(aResponses[i]);
  }
}

void
StartDestroyStreamChild(const nsTArray<PCacheRequest>& aRequests)
{
  for (uint32_t i = 0; i < aRequests.Length(); ++i) {
    if (aRequests[i].body().type() == PCacheReadStreamOrVoid::Tvoid_t) {
      continue;
    }
    StartDestroyStreamChild(aRequests[i].body().get_PCacheReadStream());
  }
}

void
AddFeatureToStreamChild(const PCacheResponseOrVoid& aResponseOrVoid,
                        Feature* aFeature)
{
  if (aResponseOrVoid.type() == PCacheResponseOrVoid::Tvoid_t) {
    return;
  }

  AddFeatureToStreamChild(aResponseOrVoid.get_PCacheResponse(), aFeature);
}

void
AddFeatureToStreamChild(const PCacheResponse& aResponse,
                        Feature* aFeature)
{
  if (aResponse.body().type() == PCacheReadStreamOrVoid::Tvoid_t) {
    return;
  }

  AddFeatureToStreamChild(aResponse.body().get_PCacheReadStream(), aFeature);
}

void
AddFeatureToStreamChild(const nsTArray<PCacheResponse>& aResponses,
                         Feature* aFeature)
{
  for (uint32_t i = 0; i < aResponses.Length(); ++i) {
    AddFeatureToStreamChild(aResponses[i], aFeature);
  }
}

void
AddFeatureToStreamChild(const nsTArray<PCacheRequest>& aRequests,
                         Feature* aFeature)
{
  for (uint32_t i = 0; i < aRequests.Length(); ++i) {
    if (aRequests[i].body().type() == PCacheReadStreamOrVoid::Tvoid_t) {
      continue;
    }
    AddFeatureToStreamChild(aRequests[i].body().get_PCacheReadStream(),
                            aFeature);
  }
}

} 
} 
} 
