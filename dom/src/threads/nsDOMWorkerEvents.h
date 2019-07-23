





































#ifndef __NSDOMWORKEREVENTS_H__
#define __NSDOMWORKEREVENTS_H__

#include "nsIClassInfo.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMProgressEvent.h"
#include "nsIDOMWorkers.h"
#include "nsIRunnable.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"

#include "nsDOMWorkerMacros.h"

class nsDOMWorkerXHRProxy;
class nsIXMLHttpRequest;
class nsIXPConnectWrappedNative;


#define NS_IDOMWORKERPRIVATEEVENT_IID                      \
{                                                          \
  0x4d5794d6,                                              \
  0x98ab,                                                  \
  0x4a6b,                                                  \
  { 0xad, 0x5a, 0x8e, 0xd1, 0xfa, 0x1d, 0x48, 0x39 }       \
}

class nsIDOMWorkerPrivateEvent : public nsIDOMEvent
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMWORKERPRIVATEEVENT_IID)
  virtual PRBool PreventDefaultCalled() = 0;
};

class nsDOMWorkerPrivateEvent : public nsIDOMWorkerPrivateEvent,
                                public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_NSIDOMEVENT_SPECIAL
  NS_DECL_NSICLASSINFO

  nsDOMWorkerPrivateEvent(nsIDOMEvent* aEvent);

  NS_IMETHOD PreventDefault();

  NS_IMETHOD InitEvent(const nsAString& aEventType,
                       PRBool aCanBubble,
                       PRBool aCancelable);

  virtual PRBool PreventDefaultCalled();

private:
  nsCOMPtr<nsIDOMEvent> mEvent;
  PRBool mPreventDefaultCalled;
};

class nsDOMWorkerEvent : public nsIDOMEvent,
                         public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENT
  NS_DECL_NSICLASSINFO

  nsDOMWorkerEvent()
  : mEventPhase(nsIDOMEvent::AT_TARGET), mTimeStamp(0), mBubbles(PR_FALSE),
    mCancelable(PR_FALSE), mPreventDefaultCalled(PR_FALSE) { }

  void SetTarget(nsIDOMEventTarget* aTarget) {
    mTarget = aTarget;
  }

protected:
  virtual ~nsDOMWorkerEvent() { }

  nsString mType;
  nsCOMPtr<nsIDOMEventTarget> mTarget;
  PRUint16 mEventPhase;
  DOMTimeStamp mTimeStamp;
  PRPackedBool mBubbles;
  PRPackedBool mCancelable;
  PRPackedBool mPreventDefaultCalled;
};

class nsDOMWorkerMessageEvent : public nsDOMWorkerEvent,
                                public nsIWorkerMessageEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIDOMEVENT(nsDOMWorkerEvent::)
  NS_DECL_NSIWORKERMESSAGEEVENT
  NS_FORWARD_NSICLASSINFO_NOGETINTERFACES(nsDOMWorkerEvent::)
  NS_DECL_NSICLASSINFO_GETINTERFACES

protected:
  nsString mData;
  nsString mOrigin;
  nsCOMPtr<nsISupports> mSource;
};

class nsDOMWorkerXHREvent : public nsDOMWorkerEvent,
                            public nsIRunnable,
                            public nsIDOMProgressEvent
{
  friend class nsDOMWorkerXHRProxy;
  friend class nsDOMWorkerXHREventTargetProxy;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
  NS_FORWARD_NSIDOMEVENT(nsDOMWorkerEvent::)
  NS_DECL_NSIDOMPROGRESSEVENT
  NS_FORWARD_NSICLASSINFO_NOGETINTERFACES(nsDOMWorkerEvent::)
  NS_DECL_NSICLASSINFO_GETINTERFACES

  nsDOMWorkerXHREvent(nsDOMWorkerXHRProxy* aXHRProxy);

  nsresult Init(PRUint32 aXHREventType,
                const nsAString& aType,
                nsIDOMEvent* aEvent);

  nsresult SnapshotXHRState(nsIXMLHttpRequest* aXHR);

  void EventHandled();

protected:
  nsRefPtr<nsDOMWorkerXHRProxy> mXHRProxy;
  nsCOMPtr<nsIXPConnectWrappedNative> mWorkerWN;
  PRUint32 mXHREventType;
  nsString mResponseText;
  nsCString mStatusText;
  nsresult mStatus;
  PRInt32 mReadyState;
  PRUint64 mLoaded;
  PRUint64 mTotal;
  PRInt32 mChannelID;
  PRPackedBool mUploadEvent;
  PRPackedBool mProgressEvent;
  PRPackedBool mLengthComputable;
};

#endif 
