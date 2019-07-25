





































#ifndef nsFaviconService_h_
#define nsFaviconService_h_

#include "nsIFaviconService.h"
#include "mozIAsyncFavicons.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsDataHashtable.h"
#include "nsServiceManagerUtils.h"

#include "nsToolkitCompsCID.h"

#include "mozilla/storage.h"
#include "mozilla/storage/StatementCache.h"




#define MAX_FAVICON_SIZE 10240



#define MAX_ICON_FILESIZE(s) ((PRUint32) s*s*4)


class mozIStorageStatementCallback;

class FaviconLoadListener;

class nsFaviconService : public nsIFaviconService
                       , public mozIAsyncFavicons
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

  void SendFaviconNotifications(nsIURI* aPage, nsIURI* aFaviconURI);

  




  mozilla::storage::StatementCache<mozIStorageStatement> mSyncStatements;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONSERVICE
  NS_DECL_MOZIASYNCFAVICONS

private:
  ~nsFaviconService();

  nsCOMPtr<mozIStorageConnection> mDBConn; 

  


  mozIStorageStatement* GetStatement(const nsCOMPtr<mozIStorageStatement>& aStmt);
  nsCOMPtr<mozIStorageStatement> mDBGetURL; 
  nsCOMPtr<mozIStorageStatement> mDBGetData; 
  nsCOMPtr<mozIStorageStatement> mDBGetIconInfo;
  nsCOMPtr<mozIStorageStatement> mDBInsertIcon;
  nsCOMPtr<mozIStorageStatement> mDBUpdateIcon;
  nsCOMPtr<mozIStorageStatement> mDBSetPageFavicon;
  nsCOMPtr<mozIStorageStatement> mDBRemoveOnDiskReferences;
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

#endif 
