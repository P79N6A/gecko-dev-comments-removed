




































#include "nsHyphenationManager.h"
#include "nsHyphenator.h"
#include "nsIAtom.h"
#include "nsIFile.h"
#include "nsIURI.h"
#include "nsIProperties.h"
#include "nsISimpleEnumerator.h"
#include "nsIDirectoryEnumerator.h"
#include "nsDirectoryServiceDefs.h"
#include "nsNetUtil.h"
#include "nsUnicharUtils.h"
#include "mozilla/Preferences.h"
#include "nsZipArchive.h"

using namespace mozilla;

#define INTL_HYPHENATIONALIAS_PREFIX "intl.hyphenation-alias."

nsHyphenationManager *nsHyphenationManager::sInstance = nsnull;

nsHyphenationManager*
nsHyphenationManager::Instance()
{
  if (sInstance == nsnull) {
    sInstance = new nsHyphenationManager();
  }
  return sInstance;
}

void
nsHyphenationManager::Shutdown()
{
  delete sInstance;
}

nsHyphenationManager::nsHyphenationManager()
{
  mHyphAliases.Init();
  mPatternFiles.Init();
  mHyphenators.Init();
  LoadPatternList();
  LoadAliases();
}

nsHyphenationManager::~nsHyphenationManager()
{
  sInstance = nsnull;
}

already_AddRefed<nsHyphenator>
nsHyphenationManager::GetHyphenator(nsIAtom *aLocale)
{
  nsRefPtr<nsHyphenator> hyph;
  mHyphenators.Get(aLocale, getter_AddRefs(hyph));
  if (hyph) {
    return hyph.forget();
  }
  nsCOMPtr<nsIURI> uri = mPatternFiles.Get(aLocale);
  if (!uri) {
    nsCOMPtr<nsIAtom> alias = mHyphAliases.Get(aLocale);
    if (alias) {
      mHyphenators.Get(alias, getter_AddRefs(hyph));
      if (hyph) {
        return hyph.forget();
      }
      uri = mPatternFiles.Get(alias);
      if (uri) {
        aLocale = alias;
      }
    }
    if (!uri) {
      
      
      
      nsAtomCString localeStr(aLocale);
      if (StringEndsWith(localeStr, NS_LITERAL_CSTRING("-*"))) {
        localeStr.Truncate(localeStr.Length() - 2);
      }
      PRInt32 i = localeStr.RFindChar('-');
      if (i > 1) {
        localeStr.Replace(i, localeStr.Length() - i, "-*");
        nsCOMPtr<nsIAtom> fuzzyLocale = do_GetAtom(localeStr);
        return GetHyphenator(fuzzyLocale);
      } else {
        return nsnull;
      }
    }
  }
  hyph = new nsHyphenator(uri);
  if (hyph->IsValid()) {
    mHyphenators.Put(aLocale, hyph);
    return hyph.forget();
  }
#ifdef DEBUG
  nsCString msg;
  uri->GetSpec(msg);
  msg.Insert("failed to load patterns from ", 0);
  NS_WARNING(msg.get());
#endif
  mPatternFiles.Remove(aLocale);
  return nsnull;
}

void
nsHyphenationManager::LoadPatternList()
{
  mPatternFiles.Clear();
  mHyphenators.Clear();

  LoadPatternListFromOmnijar(Omnijar::GRE);
  LoadPatternListFromOmnijar(Omnijar::APP);

  nsCOMPtr<nsIProperties> dirSvc =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID);
  if (!dirSvc) {
    return;
  }

  nsresult rv;
  nsCOMPtr<nsIFile> greDir;
  rv = dirSvc->Get(NS_GRE_DIR,
                   NS_GET_IID(nsIFile), getter_AddRefs(greDir));
  if (NS_SUCCEEDED(rv)) {
    greDir->AppendNative(NS_LITERAL_CSTRING("hyphenation"));
    LoadPatternListFromDir(greDir);
  }

  nsCOMPtr<nsIFile> appDir;
  rv = dirSvc->Get(NS_XPCOM_CURRENT_PROCESS_DIR,
                   NS_GET_IID(nsIFile), getter_AddRefs(appDir));
  if (NS_SUCCEEDED(rv)) {
    appDir->AppendNative(NS_LITERAL_CSTRING("hyphenation"));
    bool equals;
    if (NS_SUCCEEDED(appDir->Equals(greDir, &equals)) && !equals) {
      LoadPatternListFromDir(appDir);
    }
  }
}

void
nsHyphenationManager::LoadPatternListFromOmnijar(Omnijar::Type aType)
{
  nsCString base;
  nsresult rv = Omnijar::GetURIString(aType, base);
  if (NS_FAILED(rv)) {
    return;
  }

  nsRefPtr<nsZipArchive> zip = Omnijar::GetReader(aType);
  if (!zip) {
    return;
  }

  nsZipFind *find;
  zip->FindInit("hyphenation/hyph_*.dic", &find);
  if (!find) {
    return;
  }

  const char *result;
  PRUint16 len;
  while (NS_SUCCEEDED(find->FindNext(&result, &len))) {
    nsCString uriString(base);
    uriString.Append(result, len);
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), uriString);
    if (NS_FAILED(rv)) {
      continue;
    }
    nsCString locale;
    rv = uri->GetPath(locale);
    if (NS_FAILED(rv)) {
      continue;
    }
    ToLowerCase(locale);
    locale.SetLength(locale.Length() - 4); 
    locale.Cut(0, locale.RFindChar('/') + 1); 
    if (StringBeginsWith(locale, NS_LITERAL_CSTRING("hyph_"))) {
      locale.Cut(0, 5);
    }
    for (PRUint32 i = 0; i < locale.Length(); ++i) {
      if (locale[i] == '_') {
        locale.Replace(i, 1, '-');
      }
    }
    nsCOMPtr<nsIAtom> localeAtom = do_GetAtom(locale);
    if (NS_SUCCEEDED(rv)) {
      mPatternFiles.Put(localeAtom, uri);
    }
  }

  delete find;
}

void
nsHyphenationManager::LoadPatternListFromDir(nsIFile *aDir)
{
  nsresult rv;

  bool check = false;
  rv = aDir->Exists(&check);
  if (NS_FAILED(rv) || !check) {
    return;
  }

  rv = aDir->IsDirectory(&check);
  if (NS_FAILED(rv) || !check) {
    return;
  }

  nsCOMPtr<nsISimpleEnumerator> e;
  rv = aDir->GetDirectoryEntries(getter_AddRefs(e));
  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDirectoryEnumerator> files(do_QueryInterface(e));
  if (!files) {
    return;
  }

  nsCOMPtr<nsIFile> file;
  while (NS_SUCCEEDED(files->GetNextFile(getter_AddRefs(file))) && file){
    nsAutoString dictName;
    file->GetLeafName(dictName);
    NS_ConvertUTF16toUTF8 locale(dictName);
    ToLowerCase(locale);
    if (!StringEndsWith(locale, NS_LITERAL_CSTRING(".dic"))) {
      continue;
    }
    if (StringBeginsWith(locale, NS_LITERAL_CSTRING("hyph_"))) {
      locale.Cut(0, 5);
    }
    locale.SetLength(locale.Length() - 4); 
    for (PRUint32 i = 0; i < locale.Length(); ++i) {
      if (locale[i] == '_') {
        locale.Replace(i, 1, '-');
      }
    }
#ifdef DEBUG_hyph
    printf("adding hyphenation patterns for %s: %s\n", locale.get(),
           NS_ConvertUTF16toUTF8(dictName).get());
#endif
    nsCOMPtr<nsIAtom> localeAtom = do_GetAtom(locale);
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewFileURI(getter_AddRefs(uri), file);
    if (NS_SUCCEEDED(rv)) {
      mPatternFiles.Put(localeAtom, uri);
    }
  }
}

void
nsHyphenationManager::LoadAliases()
{
  nsIPrefBranch* prefRootBranch = Preferences::GetRootBranch();
  if (!prefRootBranch) {
    return;
  }
  PRUint32 prefCount;
  char **prefNames;
  nsresult rv = prefRootBranch->GetChildList(INTL_HYPHENATIONALIAS_PREFIX,
                                             &prefCount, &prefNames);
  if (NS_SUCCEEDED(rv) && prefCount > 0) {
    for (PRUint32 i = 0; i < prefCount; ++i) {
      nsAdoptingCString value = Preferences::GetCString(prefNames[i]);
      if (value) {
        nsCAutoString alias(prefNames[i]);
        alias.Cut(0, strlen(INTL_HYPHENATIONALIAS_PREFIX));
        ToLowerCase(alias);
        ToLowerCase(value);
        nsCOMPtr<nsIAtom> aliasAtom = do_GetAtom(alias);
        nsCOMPtr<nsIAtom> valueAtom = do_GetAtom(value);
        mHyphAliases.Put(aliasAtom, valueAtom);
      }
    }
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(prefCount, prefNames);
  }
}
