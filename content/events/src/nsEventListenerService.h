




































#ifndef nsEventListenerService_h__
#define nsEventListenerService_h__
#include "nsIEventListenerService.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsString.h"
#include "nsPIDOMEventTarget.h"
#include "nsCycleCollectionParticipant.h"

class nsEventListenerInfo : public nsIEventListenerInfo
{
public:
  nsEventListenerInfo(const nsAString& aType, nsIDOMEventListener* aListener,
                      PRBool aCapturing, PRBool aAllowsUntrusted,
                      PRBool aInSystemEventGroup)
  : mType(aType), mListener(aListener), mCapturing(aCapturing),
    mAllowsUntrusted(aAllowsUntrusted),
    mInSystemEventGroup(aInSystemEventGroup) {}
  virtual ~nsEventListenerInfo() {}
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsEventListenerInfo)
  NS_DECL_NSIEVENTLISTENERINFO
protected:
  nsString                      mType;
  
  nsRefPtr<nsIDOMEventListener> mListener;
  PRPackedBool                  mCapturing;
  PRPackedBool                  mAllowsUntrusted;
  PRPackedBool                  mInSystemEventGroup;
};

class nsEventListenerService : public nsIEventListenerService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTLISTENERSERVICE
};
#endif
