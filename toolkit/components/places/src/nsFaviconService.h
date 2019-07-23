





































#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsIFaviconService.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"




#define MAX_FAVICON_SIZE 10240


class mozIStorageStatementCallback;


class FaviconLoadListener;

class nsFaviconService : public nsIFaviconService
{
public:
  nsFaviconService();
  nsresult Init();

  
  static nsresult InitTables(mozIStorageConnection* aDBConn);

  



  static nsFaviconService* GetFaviconService()
  {
    if (! gFaviconService) {
      
      
      nsresult rv;
      nsCOMPtr<nsIFaviconService> serv(do_GetService("@mozilla.org/browser/favicon-service;1", &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);

      
      
      NS_ASSERTION(gFaviconService, "Favicon service creation failed");
    }
    
    
    return gFaviconService;
  }

  
  nsresult DoSetAndLoadFaviconForPage(nsIURI* aPage, nsIURI* aFavicon,
                                      PRBool aForceReload);

  
  nsresult GetFaviconLinkForIconString(const nsCString& aIcon, nsIURI** aOutput);
  void GetFaviconSpecForIconString(const nsCString& aIcon, nsACString& aOutput);

  nsresult OptimizeFaviconImage(const PRUint8* aData, PRUint32 aDataLen,
                                const nsACString& aMimeType,
                                nsACString& aNewData, nsACString& aNewMimeType);

  









  nsresult GetFaviconDataAsync(nsIURI *aFaviconURI,
                               mozIStorageStatementCallback *aCallback);

  


  nsresult FinalizeStatements();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONSERVICE

private:
  ~nsFaviconService();

  nsCOMPtr<mozIStorageConnection> mDBConn; 

  nsCOMPtr<mozIStorageStatement> mDBGetURL; 
  nsCOMPtr<mozIStorageStatement> mDBGetData; 
  nsCOMPtr<mozIStorageStatement> mDBGetIconInfo;
  nsCOMPtr<mozIStorageStatement> mDBInsertIcon;
  nsCOMPtr<mozIStorageStatement> mDBUpdateIcon;
  nsCOMPtr<mozIStorageStatement> mDBSetPageFavicon;

  static nsFaviconService* gFaviconService;

  





  nsCOMPtr<nsIURI> mDefaultIcon;

  
  
  bool mExpirationRunning;

  
  
  
  
  PRInt32 mOptimizedIconDimension;

  PRUint32 mFailedFaviconSerial;
  nsDataHashtable<nsCStringHashKey, PRUint32> mFailedFavicons;

  nsresult SetFaviconUrlForPageInternal(nsIURI* aURI, nsIURI* aFavicon,
                                        PRBool* aHasData);

  nsresult UpdateBookmarkRedirectFavicon(nsIURI* aPage, nsIURI* aFavicon);
  void SendFaviconNotifications(nsIURI* aPage, nsIURI* aFaviconURI);

  friend class FaviconLoadListener;
};

#define FAVICON_DEFAULT_URL "chrome://mozapps/skin/places/defaultFavicon.png"
#define FAVICON_ERRORPAGE_URL "chrome://global/skin/icons/warning-16.png"
#define FAVICON_ANNOTATION_NAME "favicon"
