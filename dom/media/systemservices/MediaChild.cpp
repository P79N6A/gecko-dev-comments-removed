





#include "MediaChild.h"
#include "MediaParent.h"

#include "nsGlobalWindow.h"
#include "mozilla/MediaManager.h"
#include "mozilla/Logging.h"
#include "nsQueryObject.h"

#undef LOG
PRLogModuleInfo *gMediaChildLog;
#define LOG(args) MOZ_LOG(gMediaChildLog, mozilla::LogLevel::Debug, args)

namespace mozilla {
namespace media {

already_AddRefed<Pledge<nsCString>>
GetOriginKey(const nsCString& aOrigin, bool aPrivateBrowsing)
{
  nsRefPtr<MediaManager> mgr = MediaManager::GetInstance();
  MOZ_ASSERT(mgr);

  nsRefPtr<Pledge<nsCString>> p = new Pledge<nsCString>();
  uint32_t id = mgr->mGetOriginKeyPledges.Append(*p);

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    mgr->GetNonE10sParent()->RecvGetOriginKey(id, aOrigin, aPrivateBrowsing);
  } else {
    Child::Get()->SendGetOriginKey(id, aOrigin, aPrivateBrowsing);
  }
  return p.forget();
}

void
SanitizeOriginKeys(const uint64_t& aSinceWhen)
{
  LOG(("SanitizeOriginKeys since %llu", aSinceWhen));

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    
    
    ScopedDeletePtr<Parent<NonE10s>> tmpParent(new Parent<NonE10s>(true));
    tmpParent->RecvSanitizeOriginKeys(aSinceWhen);
  } else {
    Child::Get()->SendSanitizeOriginKeys(aSinceWhen);
  }
}

static Child* sChild;

Child* Child::Get()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Content);
  MOZ_ASSERT(NS_IsMainThread());
  if (!sChild) {
    sChild = static_cast<Child*>(dom::ContentChild::GetSingleton()->SendPMediaConstructor());
  }
  return sChild;
}

Child::Child()
  : mActorDestroyed(false)
{
  if (!gMediaChildLog) {
    gMediaChildLog = PR_NewLogModule("MediaChild");
  }
  LOG(("media::Child: %p", this));
  MOZ_COUNT_CTOR(Child);
}

Child::~Child()
{
  LOG(("~media::Child: %p", this));
  sChild = nullptr;
  MOZ_COUNT_DTOR(Child);
}

void Child::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

bool
Child::RecvGetOriginKeyResponse(const uint32_t& aRequestId, const nsCString& aKey)
{
  nsRefPtr<MediaManager> mgr = MediaManager::GetInstance();
  if (!mgr) {
    return false;
  }
  nsRefPtr<Pledge<nsCString>> pledge = mgr->mGetOriginKeyPledges.Remove(aRequestId);
  if (pledge) {
    pledge->Resolve(aKey);
  }
  return true;
}

PMediaChild*
AllocPMediaChild()
{
  return new Child();
}

bool
DeallocPMediaChild(media::PMediaChild *aActor)
{
  delete static_cast<Child*>(aActor);
  return true;
}

}
}
