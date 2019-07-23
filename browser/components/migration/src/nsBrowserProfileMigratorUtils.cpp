





































#include "nsBrowserProfileMigratorUtils.h"
#include "nsINavBookmarksService.h"
#include "nsBrowserCompsCID.h"
#include "nsToolkitCompsCID.h"
#include "nsIPlacesImportExportService.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "nsIProperties.h"
#include "nsIProfileMigrator.h"

#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsISupportsPrimitives.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsIRDFService.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsXPCOMCID.h"

#define MIGRATION_BUNDLE "chrome://browser/locale/migration/migration.properties"

#define BOOKMARKS_FILE_NAME NS_LITERAL_STRING("bookmarks.html")

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
                             PRBool aReplace, nsIFile* aSourceProfile, 
                             PRUint16* aResult)
{
  nsCOMPtr<nsIFile> sourceFile; 
  PRBool exists;
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

nsresult 
AnnotatePersonalToolbarFolder(nsIFile* aSourceBookmarksFile,
                              nsIFile* aTargetBookmarksFile,
                              const char* aToolbarFolderName)
{
  nsCOMPtr<nsIInputStream> fileInputStream;
  nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream),
                                           aSourceBookmarksFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                   aTargetBookmarksFile);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILineInputStream> lineInputStream =
    do_QueryInterface(fileInputStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString sourceBuffer;
  nsCAutoString targetBuffer;
  PRBool moreData = PR_FALSE;
  PRUint32 bytesWritten = 0;
  do {
    lineInputStream->ReadLine(sourceBuffer, &moreData);
    if (!moreData)
      break;

    PRInt32 nameOffset = sourceBuffer.Find(aToolbarFolderName);
    if (nameOffset >= 0) {
      
      
      NS_NAMED_LITERAL_CSTRING(folderPrefix, "<DT><H3 ");
      PRInt32 folderPrefixOffset = sourceBuffer.Find(folderPrefix);
      if (folderPrefixOffset >= 0)
        sourceBuffer.Insert(NS_LITERAL_CSTRING("PERSONAL_TOOLBAR_FOLDER=\"true\" "), 
                            folderPrefixOffset + folderPrefix.Length());
    }

    targetBuffer.Assign(sourceBuffer);
    targetBuffer.Append("\r\n");
    outputStream->Write(targetBuffer.get(), targetBuffer.Length(),
                        &bytesWritten);
  }
  while (1);
  
  outputStream->Close();
  
  return NS_OK;
}

nsresult
ImportBookmarksHTML(nsIFile* aBookmarksFile, 
                    PRBool aImportIntoRoot,
                    PRBool aOverwriteDefaults,
                    const PRUnichar* aImportSourceNameKey)
{
  nsresult rv;

  nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(aBookmarksFile));
  NS_ENSURE_TRUE(localFile, NS_ERROR_FAILURE);
  nsCOMPtr<nsIPlacesImportExportService> importer = do_GetService(NS_PLACESIMPORTEXPORTSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aImportIntoRoot) {
    rv = importer->ImportHTMLFromFile(localFile, aOverwriteDefaults);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  
  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> bundle;
  rv = bundleService->CreateBundle(MIGRATION_BUNDLE, getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString sourceName;
  bundle->GetStringFromName(aImportSourceNameKey, getter_Copies(sourceName));

  const PRUnichar* sourceNameStrings[] = { sourceName.get() };
  nsString importedBookmarksTitle;
  bundle->FormatStringFromName(NS_LITERAL_STRING("importedBookmarksFolder").get(),
                               sourceNameStrings, 1, 
                               getter_Copies(importedBookmarksTitle));

  
  nsCOMPtr<nsINavBookmarksService> bms =
    do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 root;
  rv = bms->GetBookmarksMenuFolder(&root);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 folder;
  rv = bms->CreateFolder(root, importedBookmarksTitle, -1, &folder);
  NS_ENSURE_SUCCESS(rv, rv);

  
  return importer->ImportHTMLFromFileToFolder(localFile, folder, PR_FALSE);
}

nsresult
InitializeBookmarks(nsIFile* aTargetProfile)
{
  nsCOMPtr<nsIFile> bookmarksFile;
  aTargetProfile->Clone(getter_AddRefs(bookmarksFile));
  bookmarksFile->Append(BOOKMARKS_FILE_NAME);
  
  nsresult rv = ImportBookmarksHTML(bookmarksFile, PR_TRUE, PR_TRUE, EmptyString().get());
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}
