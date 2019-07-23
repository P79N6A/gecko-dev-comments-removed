





































#ifndef nsMetricsConfig_h__
#define nsMetricsConfig_h__

#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class nsIDOMElement;
class nsIFile;
class nsILocalFile;
template<class E> class nsTArray;

class nsMetricsConfig
{
public:
  nsMetricsConfig();
  ~nsMetricsConfig() {}

  


  PRBool Init();

  


  void Reset();

  


  nsresult Load(nsIFile *file);

  


  nsresult Save(nsILocalFile *file);

  



  PRBool IsEventEnabled(const nsAString &eventNS,
                        const nsAString &eventName) const;

  


  void SetEventEnabled(const nsAString &eventNS,
                       const nsAString &eventName, PRBool enabled);

  



  void GetEvents(nsTArray<nsString> &events);

  


  void ClearEvents();

  


  PRInt32 EventLimit() const {
    NS_ASSERTION(mEventSet.IsInitialized(),
                 "nsMetricsConfig::Init not called");
    return mEventLimit;
  }

  


  void SetEventLimit(PRInt32 limit) {
    NS_ASSERTION(mEventSet.IsInitialized(),
                 "nsMetricsConfig::Init not called");
    mEventLimit = limit;
  }

  


  PRInt32 UploadInterval() const {
    NS_ASSERTION(mEventSet.IsInitialized(),
                 "nsMetricsConfig::Init not called");
    return mUploadInterval;
  }

  


  void SetUploadInterval(PRInt32 uploadInterval) {
    NS_ASSERTION(mEventSet.IsInitialized(),
                 "nsMetricsConfig::Init not called");
    mUploadInterval = uploadInterval;
  }

  


  PRBool HasConfig() const {
    NS_ASSERTION(mEventSet.IsInitialized(),
                 "nsMetricsConfig::Init not called");
    return mHasConfig;
  }

private:
  typedef void (nsMetricsConfig::*ForEachChildElementCallback)(nsIDOMElement *);

  
  void ForEachChildElement(nsIDOMElement *elem, ForEachChildElementCallback cb);

  void ProcessToplevelElement(nsIDOMElement *elem);
  void ProcessConfigChild(nsIDOMElement *elem);
  void ProcessCollectorElement(nsIDOMElement *elem);

  static PLDHashOperator PR_CALLBACK CopyKey(nsStringHashKey *key,
                                             void *userData);

  nsTHashtable<nsStringHashKey> mEventSet;
  nsDataHashtable<nsStringHashKey,nsString> mNSURIToPrefixMap;
  PRInt32 mEventLimit;
  PRInt32 mUploadInterval;
  PRBool mHasConfig;
};

#endif  
