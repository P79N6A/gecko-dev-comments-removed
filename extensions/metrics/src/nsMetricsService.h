





































#ifndef nsMetricsService_h__
#define nsMetricsService_h__

#include "nsIMetricsService.h"
#include "nsMetricsModule.h"
#include "nsMetricsConfig.h"
#include "nsIAboutModule.h"
#include "nsIStreamListener.h"
#include "nsIOutputStream.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "prio.h"
#include "prlog.h"
#include "nsIWritablePropertyBag2.h"
#include "nsDataHashtable.h"
#include "nsInterfaceHashtable.h"
#include "nsPtrHashKey.h"
#include "blapit.h"

class nsILocalFile;
class nsIDOMWindow;
class nsIDOMDocument;
class nsIDOMNode;
class nsIMetricsCollector;
class nsIDocShellTreeItem;

#ifdef PR_LOGGING


extern PRLogModuleInfo *gMetricsLog;
#endif
#define MS_LOG(args) PR_LOG(gMetricsLog, PR_LOG_DEBUG, args)
#define MS_LOG_ENABLED() PR_LOG_TEST(gMetricsLog, PR_LOG_DEBUG)


#define NS_METRICS_NAMESPACE "http://www.mozilla.org/metrics"





class nsMetricsService : public nsIMetricsService
                       , public nsIAboutModule
                       , public nsIStreamListener
                       , public nsIObserver
                       , public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMETRICSSERVICE
  NS_DECL_NSIABOUTMODULE
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITIMERCALLBACK
  
  
  
  
  static nsMetricsService* get();

  
  
  static NS_METHOD Create(nsISupports *outer, const nsIID &iid, void **result);

  
  nsresult LogEvent(const nsAString &eventName,
                    nsIWritablePropertyBag2 *eventProperties)
  {
    nsCOMPtr<nsIPropertyBag> bag = do_QueryInterface(eventProperties);
    NS_ENSURE_STATE(bag);
    return LogSimpleEvent(NS_LITERAL_STRING(NS_METRICS_NAMESPACE), eventName,
                          bag);
  }

  
  nsresult CreateEventItem(const nsAString &name, nsIMetricsEventItem **item)
  {
    return CreateEventItem(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                           name, item);
  }

  
  static PRUint32 GetWindowID(nsIDOMWindow *window);

  
  nsresult Init();

  
  const nsDataHashtable< nsPtrHashKey<nsIDOMWindow>, PRUint32 >&
  WindowMap() const
  {
    return mWindowMap;
  }

  
  nsresult HashUTF8(const nsCString &str, nsCString &hashed);

  
  
  nsresult HashUTF16(const nsString &str, nsCString &hashed) {
    return HashUTF8(NS_ConvertUTF16toUTF8(str), hashed);
  }

private:
  nsMetricsService();
  ~nsMetricsService();

  
  nsresult ProfileStartup();

  
  nsresult StartCollection();

  
  nsresult StopCollection();

  
  void EnableCollectors();
  
  
  nsresult CreateRoot();

  nsresult UploadData();
  nsresult GetDataFile(nsCOMPtr<nsILocalFile> *result);
  nsresult OpenDataFile(PRUint32 flags, PRFileDesc **result);
  nsresult GetDataFileForUpload(nsCOMPtr<nsILocalFile> *result);

  
  
  nsresult OpenCompleteXMLStream(nsILocalFile *dataFile,
                                 nsIInputStream **result);

  
  
  
  
  void InitUploadTimer(PRBool immediate);

  
  void GetConfigFile(nsIFile **result);

  
  
  void GetConfigTempFile(nsIFile **result);

  
  nsresult GenerateClientID(nsCString &clientID);

  
  PRBool IsEventEnabled(const nsAString &event) const
  {
    return mConfig.IsEventEnabled(NS_LITERAL_STRING(NS_METRICS_NAMESPACE),
                                  event);
  }

  
  nsresult BuildEventItem(nsIMetricsEventItem *item,
                          nsIDOMElement **itemElement);

  
  PRBool PersistEventCount();

  
  nsresult HashBytes(const PRUint8 *bytes, PRUint32 length,
                     nsACString &result);

  
  PRUint32 GetWindowIDInternal(nsIDOMWindow *window);

  
  
  
  
  
  PRBool LoadNewConfig(nsIFile *newConfig, nsIFile *oldConfig);

  
  void RemoveDataFile();

  
  PRInt32 GetRandomUploadInterval();

  static PLDHashOperator PR_CALLBACK
  PruneDisabledCollectors(const nsAString &key,
                          nsCOMPtr<nsIMetricsCollector> &value,
                          void *userData);

  static PLDHashOperator PR_CALLBACK
  DetachCollector(const nsAString &key,
                  nsIMetricsCollector *value, void *userData);

  static PLDHashOperator PR_CALLBACK
  NotifyNewLog(const nsAString &key,
               nsIMetricsCollector *value, void *userData);

  
  static nsresult FlushIntPref(const char *prefName, PRInt32 prefValue);
  static nsresult FlushCharPref(const char *prefName, const char *prefValue);
  static nsresult FlushClearPref(const char *prefName);

  
  static PRBool CollectionEnabled();

private:
  class BadCertListener;

  
  static nsMetricsService* sMetricsService;

  nsMetricsConfig mConfig;

  
  nsCOMPtr<nsIOutputStream> mConfigOutputStream;

  
  nsCOMPtr<nsIDOMDocument> mDocument;

  
  nsCOMPtr<nsIDOMNode> mRoot;

  
  MD5Context *mMD5Context;

  
  nsDataHashtable< nsPtrHashKey<nsIDOMWindow>, PRUint32 > mWindowMap;

  
  nsInterfaceHashtable<nsStringHashKey, nsIMetricsCollector> mCollectorMap;

  
  nsCOMPtr<nsITimer> mUploadTimer;

  
  
  static const PRUint32 kMaxRetries;

  
  
  
  
  
  
  static const PRUint32 kMetricsVersion;

  PRInt32 mEventCount;
  PRInt32 mSuspendCount;
  PRBool mUploading;
  nsString mSessionID;
  
  PRUint32 mNextWindowID;

  
  PRUint32 mRetryCount;
};

class nsMetricsUtils
{
public:
  
  static nsresult NewPropertyBag(nsIWritablePropertyBag2 **result);

  
  static nsresult AddChildItem(nsIMetricsEventItem *parent,
                               const nsAString &childName,
                               nsIPropertyBag *childProperties);

  
  
  
  static PRBool GetRandomNoise(void *buf, PRSize size);

  
  
  static nsresult CreateElement(nsIDOMDocument *ownerDoc,
                                const nsAString &tag, nsIDOMElement **element);

  
  static PRBool IsSubframe(nsIDocShellTreeItem *docShell);

  
  
  static PRUint32 FindWindowForNode(nsIDOMNode *node);
};

#endif  
