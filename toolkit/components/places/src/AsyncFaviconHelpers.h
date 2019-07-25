



















































#ifndef AsyncFaviconHelpers_h_
#define AsyncFaviconHelpers_h_

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIURI.h"

#include "nsIFaviconService.h"
#include "Helpers.h"

#include "mozilla/storage.h"

#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIStreamListener.h"

#include "nsCycleCollectionParticipant.h"


#define FAVICONSTEP_FAKE_EMPTYPARAM

#define FAVICONSTEP_FAIL_IF_FALSE(_cond) \
  FAVICONSTEP_FAIL_IF_FALSE_RV(_cond, FAVICONSTEP_FAKE_EMPTYPARAM)

#define FAVICONSTEP_FAIL_IF_FALSE_RV(_cond, _rv) \
  PR_BEGIN_MACRO \
  if (!(_cond)) { \
    NS_WARNING("AsyncFaviconStep failed!"); \
    mStepper->Failure(); \
    return _rv; \
  } \
  PR_END_MACRO

#define FAVICONSTEP_CANCEL_IF_TRUE(_cond, _notify) \
  FAVICONSTEP_CANCEL_IF_TRUE_RV(_cond, _notify, FAVICONSTEP_FAKE_EMPTYPARAM)

#define FAVICONSTEP_CANCEL_IF_TRUE_RV(_cond, _notify, _rv) \
  PR_BEGIN_MACRO \
  if (_cond) { \
    mStepper->Cancel(_notify); \
    return _rv; \
  } \
  PR_END_MACRO

#define ICON_STATUS_UNKNOWN 0
#define ICON_STATUS_CHANGED 1 << 0
#define ICON_STATUS_SAVED 1 << 1
#define ICON_STATUS_ASSOCIATED 1 << 2

namespace mozilla {
namespace places {



class AsyncFaviconStepperInternal;
class AsyncFaviconStepper;






class AsyncFaviconStep : public AsyncStatementCallback
{
public:
  AsyncFaviconStep() {}

  





  void SetStepper(AsyncFaviconStepperInternal* aStepper) { mStepper = aStepper; }

  


  virtual void Run() {};

protected:
  nsCOMPtr<AsyncFaviconStepperInternal> mStepper;
};




enum AsyncFaviconStepperStatus {
  STEPPER_INITING = 0
, STEPPER_RUNNING = 1
, STEPPER_FAILED = 2
, STEPPER_COMPLETED = 3
, STEPPER_CANCELED = 4
};








class AsyncFaviconStepperInternal : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  





  AsyncFaviconStepperInternal(nsIFaviconDataCallback* aCallback);

  


  nsresult Step();

  



  void Failure();

  





  void Cancel(bool aNotify);

  nsCOMPtr<nsIFaviconDataCallback> mCallback;
  PRInt64 mPageId;
  nsCOMPtr<nsIURI> mPageURI;
  PRInt64 mIconId;
  nsCOMPtr<nsIURI> mIconURI;
  nsCString mData;
  nsCString mMimeType;
  PRTime mExpiration;
  bool mIsRevisit;
  PRUint16 mIconStatus; 

private:
  enum AsyncFaviconStepperStatus mStatus;
  nsCOMArray<AsyncFaviconStep> mSteps;

  friend class AsyncFaviconStepper;
};










class AsyncFaviconStepper : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  





  AsyncFaviconStepper(nsIFaviconDataCallback* aCallback);

  


  nsresult Start();

  





  nsresult AppendStep(AsyncFaviconStep* aStep);

  
  

  void SetPageId(PRInt64 aPageId) { mStepper->mPageId = aPageId; }
  PRInt64 GetPageId() { return mStepper->mPageId; }

  void SetPageURI(nsIURI* aURI) { mStepper->mPageURI = aURI; }
  already_AddRefed<nsIURI> GetPageURI() { return mStepper->mPageURI.forget(); }

  void SetIconId(PRInt64 aIconId) { mStepper->mIconId = aIconId; }
  PRInt64 GetIconId() { return mStepper->mIconId; }

  void SetIconURI(nsIURI* aURI) { mStepper->mIconURI = aURI; }
  already_AddRefed<nsIURI> GetIconURI() { return mStepper->mIconURI.forget(); }

  nsresult SetIconData(const nsACString& aMimeType,
                       const PRUint8* aData,
                       PRUint32 aDataLen);
  nsresult GetIconData(nsACString& aMimeType,
                       const PRUint8** aData,
                       PRUint32* aDataLen);

  void SetExpiration(PRTime aExpiration) { mStepper->mExpiration = aExpiration; }
  PRTime GetExpiration() { return mStepper->mExpiration; }

private:
  nsCOMPtr<AsyncFaviconStepperInternal> mStepper;
};







class GetEffectivePageStep : public AsyncFaviconStep
{
public:
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK

  GetEffectivePageStep();
  void Run();

private:
  void CheckPageAndProceed();

  PRUint8 mSubStep;
  bool mIsBookmarked;
};





class FetchDatabaseIconStep : public AsyncFaviconStep
{
public:
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK

  FetchDatabaseIconStep() {};
  void Run();
};






class EnsureDatabaseEntryStep : public AsyncFaviconStep
{
public:
  NS_IMETHOD HandleCompletion(PRUint16 aReason);
  NS_IMETHOD HandleError(mozIStorageError* aError);

  EnsureDatabaseEntryStep() {};
  void Run();
};

enum AsyncFaviconFetchMode {
  FETCH_NEVER = 0
, FETCH_IF_MISSING = 1
, FETCH_ALWAYS = 2
};






class FetchNetworkIconStep : public AsyncFaviconStep
                           , public nsIStreamListener
                           , public nsIInterfaceRequestor
                           , public nsIChannelEventSink
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(FetchNetworkIconStep, AsyncFaviconStep)
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK
  NS_DECL_NSIREQUESTOBSERVER

  FetchNetworkIconStep(enum AsyncFaviconFetchMode aFetchMode);
  void Run();

private:
  enum AsyncFaviconFetchMode mFetchMode;
  nsCOMPtr<nsIChannel> mChannel;
  nsCString mData;
};





class SetFaviconDataStep : public AsyncFaviconStep
{
public:
  NS_IMETHOD HandleCompletion(PRUint16 aReason);
  NS_IMETHOD HandleError(mozIStorageError* aError);

  SetFaviconDataStep() {};
  void Run();
};





class AssociateIconWithPageStep : public AsyncFaviconStep
{
public:
  NS_IMETHOD HandleCompletion(PRUint16 aReason);
  NS_IMETHOD HandleError(mozIStorageError* aError);

  AssociateIconWithPageStep() {};
  void Run();
};





class NotifyStep : public AsyncFaviconStep
{
public:
  NotifyStep() {};
  void Run();
};

} 
} 

#endif 
