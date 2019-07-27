





#ifndef mozilla_dom_TVServiceCallbacks_h
#define mozilla_dom_TVServiceCallbacks_h

#include "nsITVService.h"

#include "mozilla/dom/TVSourceBinding.h"

namespace mozilla {
namespace dom {

class Promise;
class TVChannel;
class TVManager;
class TVTuner;
class TVSource;

class TVServiceSourceSetterCallback MOZ_FINAL : public nsITVServiceCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITVSERVICECALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS(TVServiceSourceSetterCallback)

  TVServiceSourceSetterCallback(TVTuner* aTuner,
                                Promise* aPromise,
                                TVSourceType aSourceType);

private:
  ~TVServiceSourceSetterCallback();

  nsRefPtr<TVTuner> mTuner;
  nsRefPtr<Promise> mPromise;
  TVSourceType mSourceType;
};

class TVServiceChannelScanCallback MOZ_FINAL : public nsITVServiceCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITVSERVICECALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS(TVServiceChannelScanCallback)

  TVServiceChannelScanCallback(TVSource* aSource,
                               Promise* aPromise,
                               bool aIsScanning);

private:
  ~TVServiceChannelScanCallback();

  nsRefPtr<TVSource> mSource;
  nsRefPtr<Promise> mPromise;
  bool mIsScanning;
};

class TVServiceChannelSetterCallback MOZ_FINAL : public nsITVServiceCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITVSERVICECALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS(TVServiceChannelSetterCallback)

  TVServiceChannelSetterCallback(TVSource* aSource,
                                 Promise* aPromise,
                                 const nsAString& aChannelNumber);

private:
  ~TVServiceChannelSetterCallback();

  nsRefPtr<TVSource> mSource;
  nsRefPtr<Promise> mPromise;
  nsString mChannelNumber;
};

class TVServiceTunerGetterCallback MOZ_FINAL : public nsITVServiceCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITVSERVICECALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS(TVServiceTunerGetterCallback)

  explicit TVServiceTunerGetterCallback(TVManager* aManager);

private:
  ~TVServiceTunerGetterCallback();

  nsRefPtr<TVManager> mManager;
};

class TVServiceChannelGetterCallback MOZ_FINAL : public nsITVServiceCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITVSERVICECALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS(TVServiceChannelGetterCallback)

  TVServiceChannelGetterCallback(TVSource* aSource,
                                 Promise* aPromise);

private:
  ~TVServiceChannelGetterCallback();

  nsRefPtr<TVSource> mSource;
  nsRefPtr<Promise> mPromise;
};

class TVServiceProgramGetterCallback MOZ_FINAL : public nsITVServiceCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSITVSERVICECALLBACK
  NS_DECL_CYCLE_COLLECTION_CLASS(TVServiceProgramGetterCallback)

  
  
  
  TVServiceProgramGetterCallback(TVChannel* aChannel,
                                 Promise* aPromise,
                                 bool aIsSingular);

private:
  ~TVServiceProgramGetterCallback();

  nsRefPtr<TVChannel> mChannel;
  nsRefPtr<Promise> mPromise;
  bool mIsSingular;
};

} 
} 

#endif 
