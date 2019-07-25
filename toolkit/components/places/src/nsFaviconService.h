





































#include "nsCOMPtr.h"
#include "nsDataHashtable.h"
#include "nsIFaviconService.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"

#include "nsToolkitCompsCID.h"

#include "mozilla/storage.h"




#define MAX_FAVICON_SIZE 10240

namespace mozilla {
namespace places {

  enum FaviconStatementId {
    DB_GET_ICON_INFO_WITH_PAGE = 0
  , DB_INSERT_ICON = 1
  , DB_ASSOCIATE_ICONURI_TO_PAGEURI = 2
  };

} 
} 



#define MAX_ICON_FILESIZE(s) ((PRUint32) s*s*4)


class mozIStorageStatementCallback;

class FaviconLoadListener;

class nsFaviconService : public nsIFaviconService
{
public:
  nsFaviconService();

  


  static nsFaviconService* GetSingleton();

  


  nsresult Init();

  
  static nsresult InitTables(mozIStorageConnection* aDBConn);

  



  static nsFaviconService* GetFaviconService()
  {
    if (!gFaviconService) {
      nsCOMPtr<nsIFaviconService> serv =
        do_GetService(NS_FAVICONSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nsnull);
      NS_ASSERTION(gFaviconService, "Should have static instance pointer now");
    }
    return gFaviconService;
  }

  
  nsresult DoSetAndLoadFaviconForPage(nsIURI* aPageURI,
                                      nsIURI* aFaviconURI,
                                      PRBool aForceReload,
                                      nsIFaviconDataCallback* aCallback);

  
  nsresult GetFaviconLinkForIconString(const nsCString& aIcon, nsIURI** aOutput);
  void GetFaviconSpecForIconString(const nsCString& aIcon, nsACString& aOutput);

  nsresult OptimizeFaviconImage(const PRUint8* aData, PRUint32 aDataLen,
                                const nsACString& aMimeType,
                                nsACString& aNewData, nsACString& aNewMimeType);
  PRInt32 GetOptimizedIconDimension() { return mOptimizedIconDimension; }

  









  nsresult GetFaviconDataAsync(nsIURI* aFaviconURI,
                               mozIStorageStatementCallback* aCallback);

  








  void checkAndNotify(nsIURI* aPageURI, nsIURI* aFaviconURI);

  


  nsresult FinalizeStatements();

  mozIStorageStatement* GetStatementById(
    enum mozilla::places::FaviconStatementId aStatementId
  )
  {
    using namespace mozilla::places;
    switch(aStatementId) {
      case DB_GET_ICON_INFO_WITH_PAGE:
        return GetStatement(mDBGetIconInfoWithPage);
      case DB_INSERT_ICON:
        return GetStatement(mDBInsertIcon);
      case DB_ASSOCIATE_ICONURI_TO_PAGEURI:
        return GetStatement(mDBAssociateFaviconURIToPageURI);
    }
    return nsnull;
  }

  nsresult UpdateBookmarkRedirectFavicon(nsIURI* aPage, nsIURI* aFavicon);

  void SendFaviconNotifications(nsIURI* aPage, nsIURI* aFaviconURI);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONSERVICE

private:
  ~nsFaviconService();

  nsCOMPtr<mozIStorageConnection> mDBConn; 

  


  mozIStorageStatement* GetStatement(const nsCOMPtr<mozIStorageStatement>& aStmt);
  nsCOMPtr<mozIStorageStatement> mDBGetURL; 
  nsCOMPtr<mozIStorageStatement> mDBGetData; 
  nsCOMPtr<mozIStorageStatement> mDBGetIconInfo;
  nsCOMPtr<mozIStorageStatement> mDBGetIconInfoWithPage;
  nsCOMPtr<mozIStorageStatement> mDBInsertIcon;
  nsCOMPtr<mozIStorageStatement> mDBUpdateIcon;
  nsCOMPtr<mozIStorageStatement> mDBSetPageFavicon;
  nsCOMPtr<mozIStorageStatement> mDBAssociateFaviconURIToPageURI;
  nsCOMPtr<mozIStorageStatement> mDBRemoveOnDiskReferences;
  nsCOMPtr<mozIStorageStatement> mDBRemoveTempReferences;
  nsCOMPtr<mozIStorageStatement> mDBRemoveAllFavicons;

  static nsFaviconService* gFaviconService;

  





  nsCOMPtr<nsIURI> mDefaultIcon;

  
  
  bool mFaviconsExpirationRunning;

  
  
  
  
  PRInt32 mOptimizedIconDimension;

  PRUint32 mFailedFaviconSerial;
  nsDataHashtable<nsCStringHashKey, PRUint32> mFailedFavicons;

  nsresult SetFaviconUrlForPageInternal(nsIURI* aURI, nsIURI* aFavicon,
                                        PRBool* aHasData);

  friend class FaviconLoadListener;

  bool mShuttingDown;

  
  
  nsresult GetDefaultFaviconData(nsCString& byteStr);

  
  
  nsCString mDefaultFaviconData;
};

#define FAVICON_ANNOTATION_NAME "favicon"
