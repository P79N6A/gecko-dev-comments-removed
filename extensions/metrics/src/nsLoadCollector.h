





































#ifndef nsLoadCollector_h_
#define nsLoadCollector_h_
























#include "nsIMetricsCollector.h"
#include "nsIWebProgressListener.h"
#include "nsIWritablePropertyBag2.h"
#include "nsWeakReference.h"
#include "nsDataHashtable.h"
#include "nsAutoPtr.h"
#include "nsIDocumentObserver.h"
#include "nsPtrHashKey.h"

class nsIDocument;

class nsLoadCollector : public nsIMetricsCollector,
                        public nsIWebProgressListener,
                        public nsIDocumentObserver,
                        public nsSupportsWeakReference
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMETRICSCOLLECTOR
  NS_DECL_NSIWEBPROGRESSLISTENER
  NS_DECL_NSIDOCUMENTOBSERVER

  nsLoadCollector();
  nsresult Init();

 private:
  struct RequestEntry {
    nsCOMPtr<nsIWritablePropertyBag2> properties;
    PRTime startTime;
  };

  struct DocumentEntry {
    PRUint32 docID;
    PRUint32 windowID;
    PRBool subframe;
  };

  ~nsLoadCollector();

  
  static PLDHashOperator PR_CALLBACK
  RemoveDocumentFromMap(const nsIDocument *document,
                        DocumentEntry &entry, void *userData);

  
  nsDataHashtable<nsISupportsHashKey, RequestEntry> mRequestMap;

  
  nsDataHashtable< nsPtrHashKey<nsIDocument>, DocumentEntry > mDocumentMap;

  
  PRUint32 mNextDocID;
};

#define NS_LOADCOLLECTOR_CLASSNAME "Load Collector"
#define NS_LOADCOLLECTOR_CID \
{ 0xa97357a0, 0xa2f3, 0x4b1f, {0x93, 0xd3, 0x36, 0xdc, 0xb7, 0xee, 0x24, 0x63}}

#endif 
