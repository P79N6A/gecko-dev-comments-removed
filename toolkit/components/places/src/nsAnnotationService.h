





































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

  


  static nsAnnotationService *GetSingleton();

  


  nsresult Init();

  static nsresult InitTables(mozIStorageConnection* aDBConn);

  



  static nsAnnotationService * GetAnnotationService()
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

  nsCOMPtr<mozIStorageStatement> mDBSetAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBSetItemAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetItemAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationNames;
  nsCOMPtr<mozIStorageStatement> mDBGetItemAnnotationNames;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationFromURI;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationFromItemId;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationNameID;
  nsCOMPtr<mozIStorageStatement> mDBAddAnnotationName;
  nsCOMPtr<mozIStorageStatement> mDBAddAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBAddItemAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBRemoveAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBRemoveItemAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetItemsWithAnnotation;

  nsCOMArray<nsIAnnotationObserver> mObservers;

  static nsAnnotationService *gAnnotationService;

  static const int kAnnoIndex_ID;
  static const int kAnnoIndex_PageOrItem;
  static const int kAnnoIndex_Name;
  static const int kAnnoIndex_MimeType;
  static const int kAnnoIndex_Content;
  static const int kAnnoIndex_Flags;
  static const int kAnnoIndex_Expiration;
  static const int kAnnoIndex_Type;
  static const int kAnnoIndex_DateAdded;
  static const int kAnnoIndex_LastModified;

  nsresult HasAnnotationInternal(PRInt64 aFkId, PRBool aIsBookmarkId,
                                 const nsACString& aName, PRBool* hasAnnotation,
                                 PRInt64* annotationID);
  nsresult StartGetAnnotationFromURI(nsIURI* aURI,
                                     const nsACString& aName);
  nsresult StartGetAnnotationFromItemId(PRInt64 aItemId,
                                        const nsACString& aName);
  nsresult StartSetAnnotation(PRInt64 aFkId,
                              PRBool aIsItemAnnotation,
                              const nsACString& aName,
                              PRInt32 aFlags,
                              PRUint16 aExpiration,
                              PRUint16 aType,
                              mozIStorageStatement** aStatement);
  nsresult SetAnnotationStringInternal(PRInt64 aItemId,
                                       PRBool aIsItemAnnotation,
                                       const nsACString& aName,
                                       const nsAString& aValue,
                                       PRInt32 aFlags,
                                       PRUint16 aExpiration);
  nsresult SetAnnotationInt32Internal(PRInt64 aFkId,
                                      PRBool aIsItemAnnotation,
                                      const nsACString& aName,
                                      PRInt32 aValue,
                                      PRInt32 aFlags,
                                      PRUint16 aExpiration);
  nsresult SetAnnotationInt64Internal(PRInt64 aFkId,
                                      PRBool aIsItemAnnotation,
                                      const nsACString& aName,
                                      PRInt64 aValue,
                                      PRInt32 aFlags,
                                      PRUint16 aExpiration);
  nsresult SetAnnotationDoubleInternal(PRInt64 aFkId,
                                       PRBool aIsItemAnnotation,
                                       const nsACString& aName,
                                       double aValue,
                                       PRInt32 aFlags,
                                       PRUint16 aExpiration);
  nsresult SetAnnotationBinaryInternal(PRInt64 aFkId,
                                       PRBool aIsItemAnnotation,
                                       const nsACString& aName,
                                       const PRUint8 *aData,
                                       PRUint32 aDataLen,
                                       const nsACString& aMimeType,
                                       PRInt32 aFlags,
                                       PRUint16 aExpiration);
  nsresult RemoveAnnotationInternal(PRInt64 aFkId,
                                    PRBool aIsItemAnnotation,
                                    const nsACString& aName);
  static nsresult GetPlaceIdForURI(nsIURI* aURI, PRInt64* _retval,
                                   PRBool aAutoCreate = PR_TRUE);

  PRBool InPrivateBrowsingMode() const;

  void CallSetForPageObservers(nsIURI* aURI, const nsACString& aName);
  void CallSetForItemObservers(PRInt64 aItemId, const nsACString& aName);

public:
  nsresult GetPagesWithAnnotationCOMArray(const nsACString& aName,
                                          nsCOMArray<nsIURI>* aResults);
  nsresult GetItemsWithAnnotationTArray(const nsACString& aName,
                                        nsTArray<PRInt64>* aResult);
  nsresult GetAnnotationNamesTArray(PRInt64 aFkId, nsTArray<nsCString>* aResult,
                                    PRBool aIsFkItemId);
};

#endif 
