





































#ifndef nsAnnotationService_h___
#define nsAnnotationService_h___

#include "nsIAnnotationService.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "nsServiceManagerUtils.h"

class nsAnnotationService : public nsIAnnotationService
{
public:
  nsAnnotationService();

  nsresult Init();

  static nsresult InitTables(mozIStorageConnection* aDBConn);

  



  static nsAnnotationService* GetAnnotationService()
  {
    if (! gAnnotationService) {
      
      
      nsresult rv;
      nsCOMPtr<nsIAnnotationService> serv(do_GetService("@mozilla.org/browser/annotation-service;1", &rv));
      NS_ENSURE_SUCCESS(rv, nsnull);

      
      
      NS_ASSERTION(gAnnotationService, "Annotation service creation failed");
    }
    
    
    return gAnnotationService;
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIANNOTATIONSERVICE

private:
  ~nsAnnotationService();

protected:
  nsCOMPtr<mozIStorageService> mDBService;
  nsCOMPtr<mozIStorageConnection> mDBConn;

  nsCOMPtr<mozIStorageStatement> mDBSetAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationNames;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationFromURI;
  nsCOMPtr<mozIStorageStatement> mDBGetAnnotationNameID;
  nsCOMPtr<mozIStorageStatement> mDBAddAnnotationName;
  nsCOMPtr<mozIStorageStatement> mDBAddAnnotation;
  nsCOMPtr<mozIStorageStatement> mDBRemoveAnnotation;

  nsCOMArray<nsIAnnotationObserver> mObservers;

  static nsAnnotationService* gAnnotationService;

  static const int kAnnoIndex_ID;
  static const int kAnnoIndex_Page;
  static const int kAnnoIndex_Name;
  static const int kAnnoIndex_MimeType;
  static const int kAnnoIndex_Content;
  static const int kAnnoIndex_Flags;
  static const int kAnnoIndex_Expiration;
  static const int kAnnoIndex_Type;

  nsresult HasAnnotationInternal(PRInt64 aURLID, const nsACString& aName,
                                 PRBool* hasAnnotation, PRInt64* annotationID);
  nsresult StartGetAnnotationFromURI(nsIURI* aURI,
                                     const nsACString& aName);
  nsresult StartSetAnnotation(nsIURI* aURI,
                              const nsACString& aName,
                              PRInt32 aFlags, PRInt32 aExpiration,
                              PRInt32 aType, mozIStorageStatement** aStatement);
  void CallSetObservers(nsIURI* aURI, const nsACString& aName);

  static nsresult MigrateFromAlpha1(mozIStorageConnection* aDBConn);
  nsresult GetPageAnnotationNamesTArray(nsIURI* aURI, nsTArray<nsCString>* aResult);
};

#endif 
