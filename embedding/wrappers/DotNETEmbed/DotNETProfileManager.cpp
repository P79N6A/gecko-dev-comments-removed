





































#using <mscorlib.dll>
#include "cstringt.h"

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsIProfile.h"
#include "DotNetProfileManager.h"

using namespace Mozilla::Embedding;


void
ProfileManager::EnsureProfileService()
{
  if (sProfileService) {
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIProfile> profileService =
    do_GetService(NS_PROFILE_CONTRACTID, &rv);
  ThrowIfFailed(rv);

  sProfileService = profileService.get();
  NS_ADDREF(sProfileService);
}


Int32
ProfileManager::get_ProfileCount() 
{ 
  EnsureProfileService();

  PRInt32 count;
  nsresult rv = sProfileService->GetProfileCount(&count);
  ThrowIfFailed(rv);

  return count;
}


String *
ProfileManager::GetProfileList()[]
{
  EnsureProfileService();

  PRUint32 profileCount;
  PRUnichar **profiles;

  nsresult rv = sProfileService->GetProfileList(&profileCount, &profiles);
  ThrowIfFailed(rv);

  String *list[] = new String *[profileCount];

  for (PRUint32 i = 0; i < profileCount; ++i) {
    list[i] = CopyString(nsDependentString(profiles[i]));
  }

  return list;
}


bool
ProfileManager::ProfileExists(String *aProfileName)
{
  EnsureProfileService();

  PRBool exists;
  const wchar_t __pin * profileName = PtrToStringChars(aProfileName);

  nsresult rv = sProfileService->ProfileExists(profileName, &exists);
  ThrowIfFailed(rv);

  return exists;
}


String*
ProfileManager::get_CurrentProfile()
{
  EnsureProfileService();

  nsXPIDLString currentProfile;
  nsresult rv =
    sProfileService->GetCurrentProfile(getter_Copies(currentProfile));
  ThrowIfFailed(rv);

  return CopyString(currentProfile);
}


void
ProfileManager::set_CurrentProfile(String* aCurrentProfile)
{
  EnsureProfileService();

  const wchar_t __pin * currentProfile = PtrToStringChars(aCurrentProfile);
  nsresult rv = sProfileService->SetCurrentProfile(currentProfile);
  ThrowIfFailed(rv);
}


void
ProfileManager::ShutDownCurrentProfile(UInt32 shutDownType)
{
  EnsureProfileService();

  nsresult rv = sProfileService->ShutDownCurrentProfile(shutDownType);
  ThrowIfFailed(rv);
}


void
ProfileManager::CreateNewProfile(String* aProfileName,
                                 String *aNativeProfileDir, String* aLangcode,
                                 bool useExistingDir)
{
  EnsureProfileService();

  const wchar_t __pin * profileName = PtrToStringChars(aProfileName);
  const wchar_t __pin * nativeProfileDir = PtrToStringChars(aNativeProfileDir);
  const wchar_t __pin * langCode = PtrToStringChars(aLangcode);

  nsresult rv = sProfileService->CreateNewProfile(profileName,
                                                  nativeProfileDir, langCode,
                                                  useExistingDir);
  ThrowIfFailed(rv);
}


void
ProfileManager::RenameProfile(String* aOldName, String* aNewName)
{
  EnsureProfileService();

  const wchar_t __pin * oldName = PtrToStringChars(aOldName);
  const wchar_t __pin * newName = PtrToStringChars(aNewName);

  nsresult rv = sProfileService->RenameProfile(oldName, newName);
  ThrowIfFailed(rv);
}


void
ProfileManager::DeleteProfile(String* aName, bool aCanDeleteFiles)
{
  EnsureProfileService();

  const wchar_t __pin * name = PtrToStringChars(aName);
  nsresult rv = sProfileService->DeleteProfile(name, aCanDeleteFiles);
  ThrowIfFailed(rv);
}


void
ProfileManager::CloneProfile(String* aProfileName)
{
  EnsureProfileService();

  const wchar_t __pin * profileName = PtrToStringChars(aProfileName);

  nsresult rv = sProfileService->CloneProfile(profileName);
  ThrowIfFailed(rv);
}

