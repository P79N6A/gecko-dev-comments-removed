






































#ifndef nsAnnotationService_h___
#define nsAnnotationService_h___

#include "nsIAnnotationService.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "nsServiceManagerUtils.h"
#include "nsToolkitCompsCID.h"

class nsAnnotationService : public nsIAnnotationService
{
public:
  nsAnnotationService();

  


  static nsAnnotationService* GetSingleton();

  


  nsresult Init();

  static nsresult InitTables(mozIStorageConnection* aDBConn);

  static nsAnnotationService* GetAnnotationServiceIfAvailable() {
    return gAnnotationService;
  }

  



  static nsAnnotationService* GetAnnotationService()
  {
    if (!gAnnotationService) {
      nsCOMPtr<nsIAnnotationService> serv =
        do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID);
      NS_ENSURE_TRUE(serv, nsnull);
      NS_ASSERTION(gAnnotationService,
                   "Should have static instance pointer now");
    }
    return gAnnotationService;
  }

  


  nsresult FinalizeStatements();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIANNOTATIONSERVICE

private:
  ~nsAnnotationService();

protected:
  nsCOMPtr<mozIStorageService> mDBService;
  nsCOMPtr<mozIStorageConnection> mDBConn;

  


  mozIStorageStatement* GetStatement(const nsCOMPtr<mozIStorageStatement>& aStmt);
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationsForPage;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationsForItem;
  nsCOMPtr<mozIStorageStatement> mDBGetPageAnnotationValue;
  nsCOMPtr<mozIStorageStatement> mDBGetItemAnnotationValue;
  nsCOMPtr<mozIStorageStatement> mDBAddAnnotationName;
  nsCOMPtr<mozIStorageStatement> mDBAddPageAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBAddItemAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBRemovePageAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBRemoveItemAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetPagesWithAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetItemsWithAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBCheckPageAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBCheckItemAnnotation;

  nsCOMArray<nsIAnnotationObserver> mObservers;

  static nsAnnotationService* gAnnotationService;

  static const int kAnnoIndex_ID;
  static const int kAnnoIndex_PageOrItem;
  static const int kAnnoIndex_NameID;
  static const int kAnnoIndex_MimeType;
  static const int kAnnoIndex_Content;
  static const int kAnnoIndex_Flags;
  static const int kAnnoIndex_Expiration;
  static const int kAnnoIndex_Type;
  static const int kAnnoIndex_DateAdded;
  static const int kAnnoIndex_LastModified;

  nsresult HasAnnotationInternal(nsIURI* aURI,
                                 PRInt64 aItemId,
                                 const nsACString& aName,
                                 PRBool* _hasAnno);

  nsresult StartGetAnnotation(nsIURI* aURI,
                              PRInt64 aItemId,
                              const nsACString& aName,
                              mozIStorageStatement** _statement);

  nsresult StartSetAnnotation(nsIURI* aURI,
                              PRInt64 aItemId,
                              const nsACString& aName,
                              PRInt32 aFlags,
                              PRUint16 aExpiration,
                              PRUint16 aType,
                              mozIStorageStatement** _statement);

  nsresult SetAnnotationStringInternal(nsIURI* aURI,
                                       PRInt64 aItemId,
                                       const nsACString& aName,
                                       const nsAString& aValue,
                                       PRInt32 aFlags,
                                       PRUint16 aExpiration);
  nsresult SetAnnotationInt32Internal(nsIURI* aURI,
                                      PRInt64 aItemId,
                                      const nsACString& aName,
                                      PRInt32 aValue,
                                      PRInt32 aFlags,
                                      PRUint16 aExpiration);
  nsresult SetAnnotationInt64Internal(nsIURI* aURI,
                                      PRInt64 aItemId,
                                      const nsACString& aName,
                                      PRInt64 aValue,
                                      PRInt32 aFlags,
                                      PRUint16 aExpiration);
  nsresult SetAnnotationDoubleInternal(nsIURI* aURI,
                                       PRInt64 aItemId,
                                       const nsACString& aName,
                                       double aValue,
                                       PRInt32 aFlags,
                                       PRUint16 aExpiration);
  nsresult SetAnnotationBinaryInternal(nsIURI* aURI,
                                       PRInt64 aItemId,
                                       const nsACString& aName,
                                       const PRUint8* aData,
                                       PRUint32 aDataLen,
                                       const nsACString& aMimeType,
                                       PRInt32 aFlags,
                                       PRUint16 aExpiration);

  nsresult RemoveAnnotationInternal(nsIURI* aURI,
                                    PRInt64 aItemId,
                                    const nsACString& aName);

  PRBool InPrivateBrowsingMode() const;

  bool mShuttingDown;

public:
  nsresult GetPagesWithAnnotationCOMArray(const nsACString& aName,
                                          nsCOMArray<nsIURI>* _results);
  nsresult GetItemsWithAnnotationTArray(const nsACString& aName,
                                        nsTArray<PRInt64>* _result);
  nsresult GetAnnotationNamesTArray(nsIURI* aURI,
                                    PRInt64 aItemId,
                                    nsTArray<nsCString>* _result);
};

#endif 
