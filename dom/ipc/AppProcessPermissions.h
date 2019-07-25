






#ifndef mozilla_Capabilities_h
#define mozilla_Capabilities_h

namespace mozilla {

namespace dom {
class PBrowserParent;
class PContentParent;
}

namespace hal_sandbox {
class PHalParent;
}




bool
AppProcessHasPermissions(mozilla::dom::PBrowserParent* aActor,
                         const char* aPermission);





bool
AppProcessHasPermission(mozilla::dom::PContentParent* aActor,
                        const char* aPermission);

bool
AppProcessHasPermission(mozilla::hal_sandbox::PHalParent* aActor,
                        const char* aPermission);









} 

#endif 
