






#ifndef mozilla_AppProcessPermissions_h
#define mozilla_AppProcessPermissions_h

namespace mozilla {

namespace dom {
class PBrowserParent;
class PContentParent;
}

namespace hal_sandbox {
class PHalParent;
}






bool
AssertAppProcessPermission(mozilla::dom::PBrowserParent* aActor,
                           const char* aPermission);






bool
AssertAppProcessPermission(mozilla::dom::PContentParent* aActor,
                           const char* aPermission);

bool
AssertAppProcessPermission(mozilla::hal_sandbox::PHalParent* aActor,
                           const char* aPermission);









} 

#endif 
