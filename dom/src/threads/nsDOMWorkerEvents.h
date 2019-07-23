





































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
  NS_DECL_NSICLASSINFO_GETINTERFACES

protected:
  nsString mData;
  nsString mOrigin;
  nsCOMPtr<nsISupports> mSource;
};

class nsDOMWorkerProgressEvent : public nsDOMWorkerEvent,
                                 public nsIDOMProgressEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIDOMEVENT(nsDOMWorkerEvent::)
  NS_DECL_NSIDOMPROGRESSEVENT
  NS_DECL_NSICLASSINFO_GETINTERFACES

  nsDOMWorkerProgressEvent()
  : mLoaded(0), mTotal(0), mLengthComputable(PR_FALSE) { }

protected:
  PRUint64 mLoaded;
  PRUint64 mTotal;
  PRBool mLengthComputable;
};

class nsDOMWorkerXHRState
{
public:
  nsDOMWorkerXHRState()
  : responseTextResult(NS_OK), statusTextResult(NS_OK), status(NS_OK),
    statusResult(NS_OK), readyState(0), readyStateResult(NS_OK) { }

  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  nsString responseText;
  nsresult responseTextResult;

  nsCString statusText;
  nsresult statusTextResult;

  nsresult status;
  nsresult statusResult;

  PRInt32 readyState;
  nsresult readyStateResult;

protected:
  virtual ~nsDOMWorkerXHRState() { }

  nsAutoRefCnt mRefCnt;
};

enum SnapshotChoice {
  WANT_SNAPSHOT,
  NO_SNAPSHOT
};

class nsDOMWorkerXHREvent : public nsDOMWorkerProgressEvent,
                            public nsIRunnable
{
  friend class nsDOMWorkerXHRProxy;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICLASSINFO_GETINTERFACES

  enum SnapshotChoice {
    SNAPSHOT,
    NO_SNAPSHOT
  };

  nsDOMWorkerXHREvent(nsDOMWorkerXHRProxy* aXHRProxy);

  nsresult Init(PRUint32 aXHREventType,
                const nsAString& aType,
                nsIDOMEvent* aEvent,
                SnapshotChoice = SNAPSHOT);

  static void SnapshotXHRState(nsIXMLHttpRequest* aXHR,
                               nsDOMWorkerXHRState* aState);

  already_AddRefed<nsDOMWorkerXHRState> ForgetState() {
    return mState.forget();
  }

protected:
  nsDOMWorkerXHRState* GetState() {
    return mState;
  }

  nsRefPtr<nsDOMWorkerXHRProxy> mXHRProxy;
  nsCOMPtr<nsIXPConnectWrappedNative> mXHRWN;
  nsRefPtr<nsDOMWorkerXHRState> mState;
  PRUint32 mXHREventType;
  PRInt32 mChannelID;
  PRPackedBool mUploadEvent;
  PRPackedBool mProgressEvent;
};

#endif 
