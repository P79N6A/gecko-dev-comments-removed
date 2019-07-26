






#ifndef mozilla_AppProcessChecker_h
#define mozilla_AppProcessChecker_h

namespace mozilla {

namespace dom {
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
AssertAppProcess(mozilla::dom::PContentParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability);

bool
AssertAppProcess(mozilla::hal_sandbox::PHalParent* aActor,
                 AssertAppProcessType aType,
                 const char* aCapability);












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

} 

#endif 
