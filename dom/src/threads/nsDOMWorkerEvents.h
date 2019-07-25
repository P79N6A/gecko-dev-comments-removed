





































#ifndef __NSDOMWORKEREVENTS_H__
#define __NSDOMWORKEREVENTS_H__

#include "nsIClassInfo.h"
#include "nsIDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMProgressEvent.h"
#include "nsIDOMWorkers.h"
#include "nsIRunnable.h"

#include "jsapi.h"
#include "jsutil.h"
#include "nsAutoJSValHolder.h"
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
  virtual bool PreventDefaultCalled() = 0;
};

#define NS_FORWARD_NSIDOMEVENT_SPECIAL                                        \
  NS_IMETHOD GetType(nsAString& aType)                                        \
    { return mEvent->GetType(aType); }                                        \
  NS_IMETHOD GetTarget(nsIDOMEventTarget** aTarget)                           \
    { return mEvent->GetTarget(aTarget); }                                    \
  NS_IMETHOD GetCurrentTarget(nsIDOMEventTarget** aCurrentTarget)             \
    { return mEvent->GetCurrentTarget(aCurrentTarget); }                      \
  NS_IMETHOD GetEventPhase(PRUint16* aEventPhase)                             \
    { return mEvent->GetEventPhase(aEventPhase); }                            \
  NS_IMETHOD GetBubbles(bool* aBubbles)                                     \
    { return mEvent->GetBubbles(aBubbles); }                                  \
  NS_IMETHOD GetCancelable(bool* aCancelable)                               \
    { return mEvent->GetCancelable(aCancelable); }                            \
  NS_IMETHOD GetTimeStamp(DOMTimeStamp* aTimeStamp)                           \
    { return mEvent->GetTimeStamp(aTimeStamp); }                              \
  NS_IMETHOD StopPropagation()                                                \
    { return mEvent->StopPropagation(); }

#define NS_FORWARD_NSIDOMPROGRESSEVENT_SPECIAL                                \
  NS_IMETHOD GetLengthComputable(bool* aLengthComputable)                   \
    { return mProgressEvent->GetLengthComputable(aLengthComputable); }        \
  NS_IMETHOD GetLoaded(PRUint64* aLoaded)                                     \
    { return mProgressEvent->GetLoaded(aLoaded); }                            \
  NS_IMETHOD GetTotal(PRUint64* aTotal)                                       \
    { return mProgressEvent->GetTotal(aTotal); }

#define NS_FORWARD_NSIWORKERMESSAGEEVENT_SPECIAL                              \
  NS_IMETHOD GetData(nsAString& aData)                                        \
    { return mMessageEvent->GetData(aData); }                                 \
  NS_IMETHOD GetOrigin(nsAString& aOrigin)                                    \
    { return mMessageEvent->GetOrigin(aOrigin); }                             \
  NS_IMETHOD GetSource(nsISupports** aSource)                                 \
    { return mMessageEvent->GetSource(aSource); }

#define NS_FORWARD_NSIWORKERERROREVENT_SPECIAL                                \
  NS_IMETHOD GetMessage(nsAString& aMessage)                                  \
    { return mErrorEvent->GetMessage(aMessage); }                             \
  NS_IMETHOD GetFilename(nsAString& aFilename)                                \
    { return mErrorEvent->GetFilename(aFilename); }                           \
  NS_IMETHOD GetLineno(PRUint32* aLineno)                                     \
    { return mErrorEvent->GetLineno(aLineno); }

class nsDOMWorkerPrivateEvent : public nsIDOMWorkerPrivateEvent,
                                public nsIDOMProgressEvent,
                                public nsIWorkerMessageEvent,
                                public nsIWorkerErrorEvent,
                                public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_FORWARD_NSIDOMEVENT_SPECIAL
  NS_FORWARD_NSIWORKERMESSAGEEVENT_SPECIAL
  NS_FORWARD_NSIDOMPROGRESSEVENT_SPECIAL
  NS_FORWARD_NSIWORKERERROREVENT_SPECIAL
  NS_DECL_NSICLASSINFO

  nsDOMWorkerPrivateEvent(nsIDOMEvent* aEvent);

  NS_IMETHOD PreventDefault();

  NS_IMETHOD InitEvent(const nsAString& aEventType,
                       bool aCanBubble,
                       bool aCancelable);

  NS_IMETHOD InitProgressEvent(const nsAString& aTypeArg,
                               bool aCanBubbleArg,
                               bool aCancelableArg,
                               bool aLengthComputableArg,
                               PRUint64 aLoadedArg,
                               PRUint64 aTotalArg); 

  NS_IMETHOD InitMessageEvent(const nsAString& aTypeArg,
                              bool aCanBubbleArg,
                              bool aCancelableArg,
                              const nsAString& aDataArg,
                              const nsAString& aOriginArg,
                              nsISupports* aSourceArg);

  NS_IMETHOD InitErrorEvent(const nsAString& aTypeArg,
                            bool aCanBubbleArg,
                            bool aCancelableArg,
                            const nsAString& aMessageArg,
                            const nsAString& aFilenameArg,
                            PRUint32 aLinenoArg);

  NS_IMETHOD GetDefaultPrevented(bool* aRetVal);

  virtual bool PreventDefaultCalled();

private:
  nsCOMPtr<nsIDOMEvent> mEvent;
  nsCOMPtr<nsIDOMProgressEvent> mProgressEvent;
  nsCOMPtr<nsIWorkerMessageEvent> mMessageEvent;
  nsCOMPtr<nsIWorkerErrorEvent> mErrorEvent;
  bool mPreventDefaultCalled;
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

  bool PreventDefaultCalled() {
    return bool(mPreventDefaultCalled);
  }

protected:
  virtual ~nsDOMWorkerEvent() { }

  nsString mType;
  nsCOMPtr<nsIDOMEventTarget> mTarget;
  PRUint16 mEventPhase;
  DOMTimeStamp mTimeStamp;
  bool mBubbles;
  bool mCancelable;
  bool mPreventDefaultCalled;
};

class nsDOMWorkerMessageEvent : public nsDOMWorkerEvent,
                                public nsIWorkerMessageEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIDOMEVENT(nsDOMWorkerEvent::)
  NS_DECL_NSIWORKERMESSAGEEVENT
  NS_DECL_NSICLASSINFO_GETINTERFACES

  nsDOMWorkerMessageEvent() : mData(nsnull) { }
  ~nsDOMWorkerMessageEvent();

  nsresult SetJSData(JSContext* aCx,
                     JSAutoStructuredCloneBuffer& aBuffer,
                     nsTArray<nsCOMPtr<nsISupports> >& aWrappedNatives);

protected:
  nsString mOrigin;
  nsCOMPtr<nsISupports> mSource;

  nsAutoJSValHolder mDataVal;
  uint64* mData;
  size_t mDataLen;
  nsTArray<nsCOMPtr<nsISupports> > mWrappedNatives;
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
  bool mLengthComputable;
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

  PRUint16 readyState;
  nsresult readyStateResult;

protected:
  virtual ~nsDOMWorkerXHRState() { }

  nsAutoRefCnt mRefCnt;
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
  bool mUploadEvent;
  bool mProgressEvent;
};

class nsDOMWorkerErrorEvent : public nsDOMWorkerEvent,
                              public nsIWorkerErrorEvent
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIDOMEVENT(nsDOMWorkerEvent::)
  NS_DECL_NSIWORKERERROREVENT
  NS_DECL_NSICLASSINFO_GETINTERFACES

  nsDOMWorkerErrorEvent()
  : mLineno(0) { }

protected:
  nsString mMessage;
  nsString mFilename;
  PRUint32 mLineno;
};

#endif 
