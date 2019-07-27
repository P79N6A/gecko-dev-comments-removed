



#ifndef mozilla_dom_indexeddb_actorsparent_h__
#define mozilla_dom_indexeddb_actorsparent_h__

template <class> struct already_AddRefed;
class nsIPrincipal;

namespace mozilla {
namespace dom {

class Element;

namespace quota {

class Client;

} 

namespace indexedDB {

class LoggingInfo;
class PBackgroundIDBFactoryParent;
class PIndexedDBPermissionRequestParent;

PBackgroundIDBFactoryParent*
AllocPBackgroundIDBFactoryParent(const LoggingInfo& aLoggingInfo);

bool
RecvPBackgroundIDBFactoryConstructor(PBackgroundIDBFactoryParent* aActor,
                                     const LoggingInfo& aLoggingInfo);

bool
DeallocPBackgroundIDBFactoryParent(PBackgroundIDBFactoryParent* aActor);

PIndexedDBPermissionRequestParent*
AllocPIndexedDBPermissionRequestParent(Element* aOwnerElement,
                                       nsIPrincipal* aPrincipal);

bool
RecvPIndexedDBPermissionRequestConstructor(
                                     PIndexedDBPermissionRequestParent* aActor);

bool
DeallocPIndexedDBPermissionRequestParent(
                                     PIndexedDBPermissionRequestParent* aActor);

already_AddRefed<mozilla::dom::quota::Client>
CreateQuotaClient();

} 
} 
} 

#endif 
