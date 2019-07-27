






#ifndef mozilla_AppProcessChecker_h
#define mozilla_AppProcessChecker_h

#include <stdint.h>

class nsIPrincipal;

namespace mozilla {

namespace dom {
class TabContext;
class PBrowserParent;
class PContentParent;
}

namespace hal_sandbox {
class PHalParent;
}

enum AssertAppProcessType {
  ASSERT_APP_PROCESS_PERMISSION,
  ASSERT_APP_PROCESS_MANIFEST_URL,
  ASSERT_APP_HAS_PERMISSION
};






bool
AssertAppProcess(mozilla::dom::PBrowserParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability);





bool
AssertAppStatus(mozilla::dom::PBrowserParent* aActor,
                unsigned short aStatus);






bool
AssertAppProcess(const mozilla::dom::TabContext& aContext,
                 AssertAppProcessType aType,
                 const char* aCapability);





bool
AssertAppStatus(const mozilla::dom::TabContext& aContext,
                unsigned short aStatus);






bool
AssertAppProcess(mozilla::dom::PContentParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability);






bool
AssertAppStatus(mozilla::dom::PContentParent* aActor,
                unsigned short aStatus);

bool
AssertAppProcess(mozilla::hal_sandbox::PHalParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability);









bool
AssertAppPrincipal(mozilla::dom::PContentParent* aParent,
                   nsIPrincipal* aPrincipal);








uint32_t
CheckPermission(mozilla::dom::PContentParent* aParent,
                nsIPrincipal* aPrincipal, const char* aPermission);




template<typename T>
inline bool
AssertAppProcessPermission(T* aActor,
                           const char* aPermission) {
  return AssertAppProcess(aActor,
                          ASSERT_APP_PROCESS_PERMISSION,
                          aPermission);
}




template<typename T>
inline bool
AssertAppProcessManifestURL(T* aActor,
                            const char* aManifestURL) {
  return AssertAppProcess(aActor,
                          ASSERT_APP_PROCESS_MANIFEST_URL,
                          aManifestURL);
}




template<typename T>
inline bool
AssertAppHasPermission(T* aActor,
                       const char* aPermission) {
  return AssertAppProcess(aActor,
                          ASSERT_APP_HAS_PERMISSION,
                          aPermission);
}

template<typename T>
inline bool
AssertAppHasStatus(T* aActor,
                   unsigned short aStatus) {
  return AssertAppStatus(aActor, aStatus);
}

} 

#endif 
