




































#include "nsIDirectoryService.h"
#include "DirectoryProvider.h"

#include "nsIFile.h"
#include "nsISimpleEnumerator.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "nsArrayEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "nsBrowserDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCategoryManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMArray.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIGenericFactory.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"
#include "nsXULAppAPI.h"

namespace mozilla {
namespace browser {

NS_IMPL_ISUPPORTS2(DirectoryProvider,
                   nsIDirectoryServiceProvider,
                   nsIDirectoryServiceProvider2)

NS_IMETHODIMP
DirectoryProvider::GetFile(const char *aKey, PRBool *aPersist, nsIFile* *aResult)
{
  nsresult rv;

  *aResult = nsnull;

  
  

  nsCOMPtr<nsIFile> file;

  char const* leafName = nsnull;

  if (!strcmp(aKey, NS_APP_BOOKMARKS_50_FILE)) {
    leafName = "bookmarks.html";

    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (prefs) {
      nsCString path;
      rv = prefs->GetCharPref("browser.bookmarks.file", getter_Copies(path));
      if (NS_SUCCEEDED(rv)) {
        NS_NewNativeLocalFile(path, PR_TRUE, (nsILocalFile**)(nsIFile**) getter_AddRefs(file));
      }
    }
  }
  else if (!strcmp(aKey, NS_APP_EXISTING_PREF_OVERRIDE)) {
    rv = NS_GetSpecialDirectory(NS_APP_DEFAULTS_50_DIR,
                                getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    file->AppendNative(NS_LITERAL_CSTRING("existing-profile-defaults.js"));
    file.swap(*aResult);
    return NS_OK;
  }
  else if (!strcmp(aKey, NS_APP_MICROSUMMARY_DIR)) {
    rv = NS_GetSpecialDirectory(NS_XPCOM_CURRENT_PROCESS_DIR,
                                getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    file->AppendNative(NS_LITERAL_CSTRING("microsummary-generators"));
    file.swap(*aResult);
    return NS_OK;
  }
  else if (!strcmp(aKey, NS_APP_USER_MICROSUMMARY_DIR)) {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    file->AppendNative(NS_LITERAL_CSTRING("microsummary-generators"));
    file.swap(*aResult);
    return NS_OK;
  }
  else {
    return NS_ERROR_FAILURE;
  }

  nsDependentCString leafstr(leafName);

  nsCOMPtr<nsIFile> parentDir;
  if (file) {
    rv = file->GetParent(getter_AddRefs(parentDir));
    if (NS_FAILED(rv))
      return rv;
  }
  else {
    rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(parentDir));
    if (NS_FAILED(rv))
      return rv;

    rv = parentDir->Clone(getter_AddRefs(file));
    if (NS_FAILED(rv))
      return rv;

    file->AppendNative(leafstr);
  }

  *aPersist = PR_TRUE;
  NS_ADDREF(*aResult = file);

  return NS_OK;
}

static void
AppendFileKey(const char *key, nsIProperties* aDirSvc,
              nsCOMArray<nsIFile> &array)
{
  nsCOMPtr<nsIFile> file;
  nsresult rv = aDirSvc->Get(key, NS_GET_IID(nsIFile), getter_AddRefs(file));
  if (NS_FAILED(rv))
    return;

  PRBool exists;
  rv = file->Exists(&exists);
  if (NS_FAILED(rv) || !exists)
    return;

  array.AppendObject(file);
}


















static void
AppendDistroSearchDirs(nsIProperties* aDirSvc, nsCOMArray<nsIFile> &array)
{
  nsCOMPtr<nsIFile> searchPlugins;
  nsresult rv = aDirSvc->Get(NS_XPCOM_CURRENT_PROCESS_DIR,
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(searchPlugins));
  if (NS_FAILED(rv))
    return;
  searchPlugins->AppendNative(NS_LITERAL_CSTRING("distribution"));
  searchPlugins->AppendNative(NS_LITERAL_CSTRING("searchplugins"));

  PRBool exists;
  rv = searchPlugins->Exists(&exists);
  if (NS_FAILED(rv) || !exists)
    return;

  nsCOMPtr<nsIFile> commonPlugins;
  rv = searchPlugins->Clone(getter_AddRefs(commonPlugins));
  if (NS_SUCCEEDED(rv)) {
    commonPlugins->AppendNative(NS_LITERAL_CSTRING("common"));
    rv = commonPlugins->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists)
        array.AppendObject(commonPlugins);
  }

  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (prefs) {

    nsCOMPtr<nsIFile> localePlugins;
    rv = searchPlugins->Clone(getter_AddRefs(localePlugins));
    if (NS_FAILED(rv))
      return;

    localePlugins->AppendNative(NS_LITERAL_CSTRING("locale"));

    nsCString locale;
    rv = prefs->GetCharPref("general.useragent.locale", getter_Copies(locale));
    if (NS_SUCCEEDED(rv)) {

      nsCOMPtr<nsIFile> curLocalePlugins;
      rv = localePlugins->Clone(getter_AddRefs(curLocalePlugins));
      if (NS_SUCCEEDED(rv)) {

        curLocalePlugins->AppendNative(locale);
        rv = curLocalePlugins->Exists(&exists);
        if (NS_SUCCEEDED(rv) && exists) {
          array.AppendObject(curLocalePlugins);
          return; 
        }
      }
    }

    
    nsCString defLocale;
    rv = prefs->GetCharPref("distribution.searchplugins.defaultLocale",
                            getter_Copies(defLocale));
    if (NS_SUCCEEDED(rv)) {

      nsCOMPtr<nsIFile> defLocalePlugins;
      rv = localePlugins->Clone(getter_AddRefs(defLocalePlugins));
      if (NS_SUCCEEDED(rv)) {

        defLocalePlugins->AppendNative(defLocale);
        rv = defLocalePlugins->Exists(&exists);
        if (NS_SUCCEEDED(rv) && exists)
          array.AppendObject(defLocalePlugins);
      }
    }
  }
}

NS_IMETHODIMP
DirectoryProvider::GetFiles(const char *aKey, nsISimpleEnumerator* *aResult)
{
  nsresult rv;

  if (!strcmp(aKey, NS_APP_SEARCH_DIR_LIST)) {
    nsCOMPtr<nsIProperties> dirSvc
      (do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
    if (!dirSvc)
      return NS_ERROR_FAILURE;

    nsCOMArray<nsIFile> baseFiles;

    AppendDistroSearchDirs(dirSvc, baseFiles);
    AppendFileKey(NS_APP_SEARCH_DIR, dirSvc, baseFiles);
    AppendFileKey(NS_APP_USER_SEARCH_DIR, dirSvc, baseFiles);

    nsCOMPtr<nsISimpleEnumerator> baseEnum;
    rv = NS_NewArrayEnumerator(getter_AddRefs(baseEnum), baseFiles);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsISimpleEnumerator> list;
    rv = dirSvc->Get(XRE_EXTENSIONS_DIR_LIST,
                     NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(list));
    if (NS_FAILED(rv))
      return rv;

    static char const *const kAppendSPlugins[] = {"searchplugins", nsnull};

    nsCOMPtr<nsISimpleEnumerator> extEnum =
      new AppendingEnumerator(list, kAppendSPlugins);
    if (!extEnum)
      return NS_ERROR_OUT_OF_MEMORY;

    return NS_NewUnionEnumerator(aResult, extEnum, baseEnum);
  }

  return NS_ERROR_FAILURE;
}

NS_METHOD
DirectoryProvider::Register(nsIComponentManager* aCompMgr, nsIFile* aPath,
                            const char *aLoaderStr, const char *aType,
                            const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  if (!catMan)
    return NS_ERROR_FAILURE;

  rv = catMan->AddCategoryEntry(XPCOM_DIRECTORY_PROVIDER_CATEGORY,
                                "browser-directory-provider",
                                NS_BROWSERDIRECTORYPROVIDER_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  return rv;
}


NS_METHOD
DirectoryProvider::Unregister(nsIComponentManager* aCompMgr, 
                              nsIFile* aPath, const char *aLoaderStr,
                              const nsModuleComponentInfo *aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catMan
    (do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  if (!catMan)
    return NS_ERROR_FAILURE;

  rv = catMan->DeleteCategoryEntry(XPCOM_DIRECTORY_PROVIDER_CATEGORY,
                                   "browser-directory-provider", PR_TRUE);
  return rv;
}

NS_IMPL_ISUPPORTS1(DirectoryProvider::AppendingEnumerator, nsISimpleEnumerator)

NS_IMETHODIMP
DirectoryProvider::AppendingEnumerator::HasMoreElements(PRBool *aResult)
{
  *aResult = mNext ? PR_TRUE : PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
DirectoryProvider::AppendingEnumerator::GetNext(nsISupports* *aResult)
{
  if (aResult)
    NS_ADDREF(*aResult = mNext);

  mNext = nsnull;

  nsresult rv;

  

  PRBool more;
  while (NS_SUCCEEDED(mBase->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> nextbasesupp;
    mBase->GetNext(getter_AddRefs(nextbasesupp));

    nsCOMPtr<nsIFile> nextbase(do_QueryInterface(nextbasesupp));
    if (!nextbase)
      continue;

    nextbase->Clone(getter_AddRefs(mNext));
    if (!mNext)
      continue;

    char const *const * i = mAppendList;
    while (*i) {
      mNext->AppendNative(nsDependentCString(*i));
      ++i;
    }

    PRBool exists;
    rv = mNext->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists)
      break;

    mNext = nsnull;
  }

  return NS_OK;
}

DirectoryProvider::AppendingEnumerator::AppendingEnumerator
    (nsISimpleEnumerator* aBase,
     char const *const *aAppendList) :
  mBase(aBase),
  mAppendList(aAppendList)
{
  
  GetNext(nsnull);
}

} 
} 
