





#ifndef mozilla_dom_cache_ActorUtils_h
#define mozilla_dom_cache_ActorUtils_h

#include "mozilla/dom/cache/Types.h"

namespace mozilla {

namespace ipc {
class PBackgroundParent;
class PrincipalInfo;
}

namespace dom {
namespace cache {

class PCacheChild;
class PCacheParent;
class PCacheStreamControlChild;
class PCacheStreamControlParent;
class PCacheStorageChild;
class PCacheStorageParent;




PCacheChild*
AllocPCacheChild();

void
DeallocPCacheChild(PCacheChild* aActor);

void
DeallocPCacheParent(PCacheParent* aActor);

PCacheStreamControlChild*
AllocPCacheStreamControlChild();

void
DeallocPCacheStreamControlChild(PCacheStreamControlChild* aActor);

void
DeallocPCacheStreamControlParent(PCacheStreamControlParent* aActor);

PCacheStorageParent*
AllocPCacheStorageParent(mozilla::ipc::PBackgroundParent* aManagingActor,
                         Namespace aNamespace,
                         const mozilla::ipc::PrincipalInfo& aPrincipalInfo);

void
DeallocPCacheStorageChild(PCacheStorageChild* aActor);

void
DeallocPCacheStorageParent(PCacheStorageParent* aActor);

} 
} 
} 

#endif 
