



#ifndef mozilla_dom_indexeddb_actorsparent_h__
#define mozilla_dom_indexeddb_actorsparent_h__

template <class> struct already_AddRefed;
class nsCString;
class nsIPrincipal;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class TabParent;

namespace quota {

class Client;

} 

namespace indexedDB {

class PBackgroundIDBFactoryParent;
class PIndexedDBPermissionRequestParent;

PBackgroundIDBFactoryParent*
AllocPBackgroundIDBFactoryParent();

bool
RecvPBackgroundIDBFactoryConstructor(PBackgroundIDBFactoryParent* aActor);

bool
DeallocPBackgroundIDBFactoryParent(PBackgroundIDBFactoryParent* aActor);

PIndexedDBPermissionRequestParent*
AllocPIndexedDBPermissionRequestParent(nsPIDOMWindow* aWindow,
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
