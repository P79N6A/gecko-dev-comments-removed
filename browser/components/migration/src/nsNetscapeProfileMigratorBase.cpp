




































#include "nsAppDirectoryServiceDefs.h"
#include "nsBrowserProfileMigratorUtils.h"
#include "nsICookieManager2.h"
#include "nsIFile.h"
#include "nsILineInputStream.h"
#include "nsInt64.h"
#include "nsIOutputStream.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"
#include "nsIPrefService.h"
#include "NSReg.h"
#include "nsIServiceManager.h"
#include "nsISupportsPrimitives.h"
#include "nsIURL.h"
#include "nsNetscapeProfileMigratorBase.h"
#include "nsNetUtil.h"
#include "prtime.h"
#include "prprf.h"

#ifdef XP_MACOSX
#define NEED_TO_FIX_4X_COOKIES 1
#define SECONDS_BETWEEN_1900_AND_1970 2208988800UL
#endif 

#define FILE_NAME_PREFS_5X NS_LITERAL_STRING("prefs.js")



nsNetscapeProfileMigratorBase::nsNetscapeProfileMigratorBase()
{
}

static nsresult
regerr2nsresult(REGERR errCode)
{
  switch (errCode) {
    case REGERR_PARAM:
    case REGERR_BADTYPE:
    case REGERR_BADNAME:
      return NS_ERROR_INVALID_ARG;

    case REGERR_MEMORY:
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsNetscapeProfileMigratorBase::GetProfileDataFromRegistry(nsILocalFile* aRegistryFile,
                                                          nsISupportsArray* aProfileNames,
                                                          nsISupportsArray* aProfileLocations)
{
  nsresult rv;
  REGERR errCode;

  
  PRBool regFileExists = PR_FALSE;
  rv = aRegistryFile->Exists(&regFileExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!regFileExists)
    return NS_ERROR_FILE_NOT_FOUND;

  
  nsCAutoString regPath;
  rv = aRegistryFile->GetNativePath(regPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if ((errCode = NR_StartupRegistry()))
    return regerr2nsresult(errCode);

  HREG reg;
  if ((errCode = NR_RegOpen(regPath.get(), &reg))) {
    NR_ShutdownRegistry();

    return regerr2nsresult(errCode);
  }

  RKEY profilesTree;
  if ((errCode = NR_RegGetKey(reg, ROOTKEY_COMMON, "Profiles", &profilesTree))) {
    NR_RegClose(reg);
    NR_ShutdownRegistry();

    return regerr2nsresult(errCode);
  }

  char profileStr[MAXREGPATHLEN];
  REGENUM enumState = nsnull;

  while (!NR_RegEnumSubkeys(reg, profilesTree, &enumState, profileStr,
                            sizeof(profileStr), REGENUM_CHILDREN))
  {
    RKEY profileKey;
    if (NR_RegGetKey(reg, profilesTree, profileStr, &profileKey))
      continue;

    
    
    char migratedStr[3];
    errCode = NR_RegGetEntryString(reg, profileKey, "migrated",
                                   migratedStr, sizeof(migratedStr));
    if ((errCode != REGERR_OK && errCode != REGERR_BUFTOOSMALL) ||
        strcmp(migratedStr, "no") == 0)
      continue;

    
    REGINFO regInfo;
    regInfo.size = sizeof(REGINFO);

    if (NR_RegGetEntryInfo(reg, profileKey, "directory", &regInfo))
      continue;

    nsCAutoString dirStr;
    dirStr.SetLength(regInfo.entryLength);

    errCode = NR_RegGetEntryString(reg, profileKey, "directory",
                                   dirStr.BeginWriting(), regInfo.entryLength);
    
    dirStr.SetLength(regInfo.entryLength-1);

    nsCOMPtr<nsILocalFile> dir;
#ifdef XP_MACOSX
    rv = NS_NewNativeLocalFile(EmptyCString(), PR_TRUE, getter_AddRefs(dir));
    if (NS_FAILED(rv)) break;
    dir->SetPersistentDescriptor(dirStr);
#else
    rv = NS_NewLocalFile(NS_ConvertUTF8toUTF16(dirStr), PR_TRUE,
                         getter_AddRefs(dir));
    if (NS_FAILED(rv)) break;
#endif

    PRBool exists;
    dir->Exists(&exists);

    if (exists) {
      aProfileLocations->AppendElement(dir);

      
      nsString profileName;
      CopyUTF8toUTF16(nsDependentCString(profileStr), profileName);

      nsCOMPtr<nsISupportsString> profileNameString(
        do_CreateInstance("@mozilla.org/supports-string;1"));

      profileNameString->SetData(profileName);
      aProfileNames->AppendElement(profileNameString);
    }
  }
  NR_RegClose(reg);
  NR_ShutdownRegistry();

  return rv;
}

#define GETPREF(xform, method, value) \
  nsresult rv = aBranch->method(xform->sourcePrefName, value); \
  if (NS_SUCCEEDED(rv)) \
    xform->prefHasValue = PR_TRUE; \
  return rv;

#define SETPREF(xform, method, value) \
  if (xform->prefHasValue) { \
    return aBranch->method(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, value); \
  } \
  return NS_OK;

nsresult 
nsNetscapeProfileMigratorBase::GetString(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  GETPREF(xform, GetCharPref, &xform->stringValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetString(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  SETPREF(xform, SetCharPref, xform->stringValue);
}

nsresult
nsNetscapeProfileMigratorBase::GetWString(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  nsCOMPtr<nsIPrefLocalizedString> prefValue;
  nsresult rv = aBranch->GetComplexValue(xform->sourcePrefName, 
                                         NS_GET_IID(nsIPrefLocalizedString),
                                         getter_AddRefs(prefValue));

  if (NS_SUCCEEDED(rv) && prefValue) {
    nsString data;
    prefValue->ToString(getter_Copies(data));

    xform->stringValue = ToNewCString(NS_ConvertUTF16toUTF8(data));
    xform->prefHasValue = PR_TRUE;
  }
  return rv;
}

nsresult 
nsNetscapeProfileMigratorBase::SetWStringFromASCII(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  if (xform->prefHasValue) {
    nsCOMPtr<nsIPrefLocalizedString> pls(do_CreateInstance("@mozilla.org/pref-localizedstring;1"));
    NS_ConvertUTF8toUTF16 data(xform->stringValue);
    pls->SetData(data.get());
    return aBranch->SetComplexValue(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, NS_GET_IID(nsIPrefLocalizedString), pls);
  }
  return NS_OK;
}

nsresult
nsNetscapeProfileMigratorBase::SetWString(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  if (xform->prefHasValue) {
    nsCOMPtr<nsIPrefLocalizedString> pls(do_CreateInstance("@mozilla.org/pref-localizedstring;1"));
    nsAutoString data = NS_ConvertUTF8toUTF16(xform->stringValue);
    pls->SetData(data.get());
    return aBranch->SetComplexValue(xform->targetPrefName ? xform->targetPrefName : xform->sourcePrefName, NS_GET_IID(nsIPrefLocalizedString), pls);
  }
  return NS_OK;
}


nsresult 
nsNetscapeProfileMigratorBase::GetBool(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  GETPREF(xform, GetBoolPref, &xform->boolValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetBool(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  SETPREF(xform, SetBoolPref, xform->boolValue);
}

nsresult 
nsNetscapeProfileMigratorBase::GetInt(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  GETPREF(xform, GetIntPref, &xform->intValue);
}

nsresult 
nsNetscapeProfileMigratorBase::SetInt(void* aTransform, nsIPrefBranch* aBranch)
{
  PrefTransform* xform = (PrefTransform*)aTransform;
  SETPREF(xform, SetIntPref, xform->intValue);
}

nsresult
nsNetscapeProfileMigratorBase::CopyFile(const nsAString& aSourceFileName, const nsAString& aTargetFileName)
{
  nsCOMPtr<nsIFile> sourceFile;
  mSourceProfile->Clone(getter_AddRefs(sourceFile));

  sourceFile->Append(aSourceFileName);
  PRBool exists = PR_FALSE;
  sourceFile->Exists(&exists);
  if (!exists)
    return NS_OK;

  nsCOMPtr<nsIFile> targetFile;
  mTargetProfile->Clone(getter_AddRefs(targetFile));
  
  targetFile->Append(aTargetFileName);
  targetFile->Exists(&exists);
  if (exists)
    targetFile->Remove(PR_FALSE);

  return sourceFile->CopyTo(mTargetProfile, aTargetFileName);
}

nsresult
nsNetscapeProfileMigratorBase::ImportNetscapeBookmarks(const nsAString& aBookmarksFileName,
                                                       const PRUnichar* aImportSourceNameKey)
{
  nsCOMPtr<nsIFile> bookmarksFile;
  mSourceProfile->Clone(getter_AddRefs(bookmarksFile));
  bookmarksFile->Append(aBookmarksFileName);
  
  return ImportBookmarksHTML(bookmarksFile, aImportSourceNameKey);
}

nsresult
nsNetscapeProfileMigratorBase::ImportNetscapeCookies(nsIFile* aCookiesFile)
{
  nsresult rv;
  nsCOMPtr<nsIInputStream> cookiesStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(cookiesStream), aCookiesFile);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILineInputStream> lineInputStream(do_QueryInterface(cookiesStream));

  
  static NS_NAMED_LITERAL_CSTRING(kTrue, "TRUE");

  nsCAutoString buffer;
  PRBool isMore = PR_TRUE;
  PRInt32 hostIndex = 0, isDomainIndex, pathIndex, secureIndex, expiresIndex, nameIndex, cookieIndex;
  PRInt32 numInts;
  PRInt64 expires;
  PRBool isDomain;
  nsInt64 currentTime = nsInt64(PR_Now()) / nsInt64(1000000);

  nsCOMPtr<nsICookieManager2> cookieManager(do_GetService(NS_COOKIEMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  












  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    if (buffer.IsEmpty() || buffer.First() == '#')
      continue;

    
    
    
    
    if ((isDomainIndex = buffer.FindChar('\t', hostIndex)     + 1) == 0 ||
        (pathIndex     = buffer.FindChar('\t', isDomainIndex) + 1) == 0 ||
        (secureIndex   = buffer.FindChar('\t', pathIndex)     + 1) == 0 ||
        (expiresIndex  = buffer.FindChar('\t', secureIndex)   + 1) == 0 ||
        (nameIndex     = buffer.FindChar('\t', expiresIndex)  + 1) == 0 ||
        (cookieIndex   = buffer.FindChar('\t', nameIndex)     + 1) == 0)
      continue;

    
    
    char *iter = buffer.BeginWriting();
    *(iter += nameIndex - 1) = char(0);
    numInts = PR_sscanf(buffer.get() + expiresIndex, "%lld", &expires);
    if (numInts != 1 || nsInt64(expires) < currentTime)
      continue;

    isDomain = Substring(buffer, isDomainIndex, pathIndex - isDomainIndex - 1).Equals(kTrue);
    const nsDependentCSubstring host =
      Substring(buffer, hostIndex, isDomainIndex - hostIndex - 1);
    
    
    if (isDomain && !host.IsEmpty() && host.First() != '.' ||
        host.FindChar(':') != -1)
      continue;

    
    rv = cookieManager->Add(host,
                            Substring(buffer, pathIndex, secureIndex - pathIndex - 1),
                            Substring(buffer, nameIndex, cookieIndex - nameIndex - 1),
                            Substring(buffer, cookieIndex, buffer.Length() - cookieIndex),
                            Substring(buffer, secureIndex, expiresIndex - secureIndex - 1).Equals(kTrue),
                            PR_FALSE,
                            expires);
  }

  return rv;
}

nsresult
nsNetscapeProfileMigratorBase::GetSignonFileName(PRBool aReplace, char** aFileName)
{
  nsresult rv;
  if (aReplace) {
    
    
    nsCOMPtr<nsIPrefService> psvc(do_GetService(NS_PREFSERVICE_CONTRACTID));
    psvc->ResetPrefs();

    nsCOMPtr<nsIFile> sourcePrefsName;
    mSourceProfile->Clone(getter_AddRefs(sourcePrefsName));
    sourcePrefsName->Append(FILE_NAME_PREFS_5X);
    psvc->ReadUserPrefs(sourcePrefsName);

    nsCOMPtr<nsIPrefBranch> branch(do_QueryInterface(psvc));
    rv = branch->GetCharPref("signon.SignonFileName", aFileName);
  }
  else 
    rv = LocateSignonsFile(aFileName);
  return rv;
}

nsresult
nsNetscapeProfileMigratorBase::LocateSignonsFile(char** aResult)
{
  nsCOMPtr<nsISimpleEnumerator> entries;
  nsresult rv = mSourceProfile->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_FAILED(rv)) return rv;

  nsCAutoString fileName;
  do {
    PRBool hasMore = PR_FALSE;
    rv = entries->HasMoreElements(&hasMore);
    if (NS_FAILED(rv) || !hasMore) break;

    nsCOMPtr<nsISupports> supp;
    rv = entries->GetNext(getter_AddRefs(supp));
    if (NS_FAILED(rv)) break;

    nsCOMPtr<nsIFile> currFile(do_QueryInterface(supp));

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewFileURI(getter_AddRefs(uri), currFile);
    if (NS_FAILED(rv)) break;
    nsCOMPtr<nsIURL> url(do_QueryInterface(uri));

    nsCAutoString extn;
    url->GetFileExtension(extn);

    if (extn.Equals("s", CaseInsensitiveCompare)) {
      url->GetFileName(fileName);
      break;
    }
  }
  while (1);

  *aResult = ToNewCString(fileName);

  return NS_OK;
}

