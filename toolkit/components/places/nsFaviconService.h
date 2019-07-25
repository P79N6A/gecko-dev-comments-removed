





































#ifndef nsFaviconService_h_
#define nsFaviconService_h_

#include "nsIFaviconService.h"
#include "mozIAsyncFavicons.h"

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsDataHashtable.h"
#include "nsServiceManagerUtils.h"
#include "nsToolkitCompsCID.h"
#include "Database.h"
#include "mozilla/storage.h"




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

  
  nsresult DoSetAndLoadFaviconForPage(nsIURI* aPageURI,
                                      nsIURI* aFaviconURI,
                                      bool aForceReload,
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

  









  void SendFaviconNotifications(nsIURI* aPageURI, nsIURI* aFaviconURI,
                                const nsACString& aGUID);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONSERVICE
  NS_DECL_MOZIASYNCFAVICONS

private:
  ~nsFaviconService();

  nsRefPtr<mozilla::places::Database> mDB;

  static nsFaviconService* gFaviconService;

  





  nsCOMPtr<nsIURI> mDefaultIcon;

  
  
  bool mFaviconsExpirationRunning;

  
  
  
  
  PRInt32 mOptimizedIconDimension;

  PRUint32 mFailedFaviconSerial;
  nsDataHashtable<nsCStringHashKey, PRUint32> mFailedFavicons;

  nsresult SetFaviconUrlForPageInternal(nsIURI* aURI, nsIURI* aFavicon,
                                        bool* aHasData);

  friend class FaviconLoadListener;

  
  
  nsresult GetDefaultFaviconData(nsCString& byteStr);

  
  
  nsCString mDefaultFaviconData;
};

#define FAVICON_ANNOTATION_NAME "favicon"

#endif 
