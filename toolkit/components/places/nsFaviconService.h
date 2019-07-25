




#ifndef nsFaviconService_h_
#define nsFaviconService_h_

#include "nsIFaviconService.h"
#include "mozIAsyncFavicons.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsDataHashtable.h"
#include "nsServiceManagerUtils.h"
#include "nsTHashtable.h"
#include "nsToolkitCompsCID.h"
#include "nsURIHashKey.h"
#include "nsITimer.h"
#include "Database.h"
#include "mozilla/storage.h"

#include "AsyncFaviconHelpers.h"




#define MAX_FAVICON_SIZE 10240



#define MAX_ICON_FILESIZE(s) ((PRUint32) s*s*4)


class mozIStorageStatementCallback;

class UnassociatedIconHashKey : public nsURIHashKey
{
public:
  UnassociatedIconHashKey(const nsIURI* aURI)
  : nsURIHashKey(aURI)
  {
  }
  UnassociatedIconHashKey(const UnassociatedIconHashKey& aOther)
  : nsURIHashKey(aOther)
  {
    NS_NOTREACHED("Do not call me!");
  }
  mozilla::places::IconData iconData;
  PRTime created;
};

class nsFaviconService : public nsIFaviconService
                       , public mozIAsyncFavicons
                       , public nsITimerCallback
{
public:
  nsFaviconService();

  


  static nsFaviconService* GetSingleton();

  


  nsresult Init();

  static nsFaviconService* GetFaviconServiceIfAvailable() {
    return gFaviconService;
  }

  



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

  
  nsresult GetFaviconLinkForIconString(const nsCString& aIcon, nsIURI** aOutput);
  void GetFaviconSpecForIconString(const nsCString& aIcon, nsACString& aOutput);

  nsresult OptimizeFaviconImage(const PRUint8* aData, PRUint32 aDataLen,
                                const nsACString& aMimeType,
                                nsACString& aNewData, nsACString& aNewMimeType);
  PRInt32 GetOptimizedIconDimension() { return mOptimizedIconDimension; }

  









  nsresult GetFaviconDataAsync(nsIURI* aFaviconURI,
                               mozIStorageStatementCallback* aCallback);

  









  void SendFaviconNotifications(nsIURI* aPageURI, nsIURI* aFaviconURI,
                                const nsACString& aGUID);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONSERVICE
  NS_DECL_MOZIASYNCFAVICONS
  NS_DECL_NSITIMERCALLBACK

private:
  ~nsFaviconService();

  nsRefPtr<mozilla::places::Database> mDB;

  nsCOMPtr<nsITimer> mExpireUnassociatedIconsTimer;

  static nsFaviconService* gFaviconService;

  





  nsCOMPtr<nsIURI> mDefaultIcon;

  
  
  bool mFaviconsExpirationRunning;

  
  
  
  
  PRInt32 mOptimizedIconDimension;

  PRUint32 mFailedFaviconSerial;
  nsDataHashtable<nsCStringHashKey, PRUint32> mFailedFavicons;

  
  friend class mozilla::places::AsyncFetchAndSetIconForPage;
  friend class mozilla::places::RemoveIconDataCacheEntry;
  nsTHashtable<UnassociatedIconHashKey> mUnassociatedIcons;

  
  
  nsresult GetDefaultFaviconData(nsCString& byteStr);

  
  
  nsCString mDefaultFaviconData;
};

#define FAVICON_ANNOTATION_NAME "favicon"

#endif 
