





































#include "nsBrowserProfileMigratorUtils.h"
#include "nsINavBookmarksService.h"
#include "nsBrowserCompsCID.h"
#include "nsToolkitCompsCID.h"
#include "nsIPlacesImportExportService.h"
#include "nsIFile.h"
#include "nsIProperties.h"
#include "nsIProfileMigrator.h"

#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsISupportsPrimitives.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsIRDFService.h"
#include "nsIStringBundle.h"
#include "nsXPCOMCID.h"

#define MIGRATION_BUNDLE "chrome://browser/locale/migration/migration.properties"

#define DEFAULT_BOOKMARKS NS_LITERAL_CSTRING("resource:///defaults/profile/bookmarks.html")

void SetUnicharPref(const char* aPref, const nsAString& aValue,
                    nsIPrefBranch* aPrefs)
{
  nsCOMPtr<nsISupportsString> supportsString =
    do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);    
  if (supportsString) {
     supportsString->SetData(aValue); 
     aPrefs->SetComplexValue(aPref, NS_GET_IID(nsISupportsString),
                             supportsString);
  }
}

void SetProxyPref(const nsAString& aHostPort, const char* aPref, 
                  const char* aPortPref, nsIPrefBranch* aPrefs) 
{
  nsCOMPtr<nsIURI> uri;
  nsCAutoString host;
  PRInt32 portValue;

  
  if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(uri), aHostPort))
      && NS_SUCCEEDED(uri->GetHost(host))
      && !host.IsEmpty()
      && NS_SUCCEEDED(uri->GetPort(&portValue))) {
    SetUnicharPref(aPref, NS_ConvertUTF8toUTF16(host), aPrefs);
    aPrefs->SetIntPref(aPortPref, portValue);
  }
  else {
    nsAutoString hostPort(aHostPort);  
    PRInt32 portDelimOffset = hostPort.RFindChar(':');
    if (portDelimOffset > 0) {
      SetUnicharPref(aPref, Substring(hostPort, 0, portDelimOffset), aPrefs);
      nsAutoString port(Substring(hostPort, portDelimOffset + 1));
      nsresult stringErr;
      portValue = port.ToInteger(&stringErr);
      if (NS_SUCCEEDED(stringErr))
        aPrefs->SetIntPref(aPortPref, portValue);
    }
    else
      SetUnicharPref(aPref, hostPort, aPrefs); 
  }
}

void ParseOverrideServers(const nsAString& aServers, nsIPrefBranch* aBranch)
{
  
  
  
  
  nsAutoString override(aServers);
  PRInt32 left = 0, right = 0;
  for (;;) {
    right = override.FindChar(';', right);
    const nsAString& host = Substring(override, left, 
                                      (right < 0 ? override.Length() : right) - left);
    if (host.EqualsLiteral("<local>"))
      override.Replace(left, 7, NS_LITERAL_STRING("localhost,127.0.0.1"));
    if (right < 0)
      break;
    left = right + 1;
    override.Replace(right, 1, NS_LITERAL_STRING(","));
  }
  SetUnicharPref("network.proxy.no_proxies_on", override, aBranch); 
}

void GetMigrateDataFromArray(MigrationData* aDataArray, PRInt32 aDataArrayLength, 
                             bool aReplace, nsIFile* aSourceProfile, 
                             PRUint16* aResult)
{
  nsCOMPtr<nsIFile> sourceFile; 
  bool exists;
  MigrationData* cursor;
  MigrationData* end = aDataArray + aDataArrayLength;
  for (cursor = aDataArray; cursor < end && cursor->fileName; ++cursor) {
    
    
    
    if (aReplace || !cursor->replaceOnly) {
      aSourceProfile->Clone(getter_AddRefs(sourceFile));
      sourceFile->Append(nsDependentString(cursor->fileName));
      sourceFile->Exists(&exists);
      if (exists)
        *aResult |= cursor->sourceFlag;
    }
    NS_Free(cursor->fileName);
    cursor->fileName = nsnull;
  }
}

void
GetProfilePath(nsIProfileStartup* aStartup, nsCOMPtr<nsIFile>& aProfileDir)
{
  if (aStartup) {
    aStartup->GetDirectory(getter_AddRefs(aProfileDir));
  }
  else {
    nsCOMPtr<nsIProperties> dirSvc
      (do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
    if (dirSvc) {
      dirSvc->Get(NS_APP_USER_PROFILE_50_DIR, NS_GET_IID(nsIFile),
                  (void**) getter_AddRefs(aProfileDir));
    }
  }
}

