















#include <pthread.h>
#include <hardware/gps.h>

#include "GonkGPSGeolocationProvider.h"
#include "SystemWorkerManager.h"
#include "mozilla/Preferences.h"
#include "nsGeoPosition.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsINetworkManager.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsIDOMMobileConnection.h"
#include "nsThreadUtils.h"

#ifdef AGPS_TYPE_INVALID
#define AGPS_HAVE_DUAL_APN
#endif

#define DEBUG_GPS 0

using namespace mozilla;

static const int kDefaultPeriod = 1000; 




NS_IMPL_THREADSAFE_ISUPPORTS2(GonkGPSGeolocationProvider, nsIGeolocationProvider, nsIRILDataCallback)

GonkGPSGeolocationProvider* GonkGPSGeolocationProvider::sSingleton;
GpsCallbacks GonkGPSGeolocationProvider::mCallbacks = {
  sizeof(GpsCallbacks),
  LocationCallback,
  StatusCallback,
  SvStatusCallback,
  NmeaCallback,
  SetCapabilitiesCallback,
  AcquireWakelockCallback,
  ReleaseWakelockCallback,
  CreateThreadCallback,
#ifdef GPS_CAPABILITY_ON_DEMAND_TIME
  RequestUtcTimeCallback,
#endif
};

AGpsCallbacks
GonkGPSGeolocationProvider::mAGPSCallbacks = {
  AGPSStatusCallback,
  CreateThreadCallback,
};

AGpsRilCallbacks
GonkGPSGeolocationProvider::mAGPSRILCallbacks = {
  AGPSRILSetIDCallback,
  AGPSRILRefLocCallback,
  CreateThreadCallback,
};

void
GonkGPSGeolocationProvider::LocationCallback(GpsLocation* location)
{
  class UpdateLocationEvent : public nsRunnable {
  public:
    UpdateLocationEvent(nsGeoPosition* aPosition)
      : mPosition(aPosition)
    {}
    NS_IMETHOD Run() {
      nsRefPtr<GonkGPSGeolocationProvider> provider =
        GonkGPSGeolocationProvider::GetSingleton();
      nsCOMPtr<nsIGeolocationUpdate> callback = provider->mLocationCallback;
      if (callback) {
        callback->Update(mPosition);
      }
      return NS_OK;
    }
  private:
    nsRefPtr<nsGeoPosition> mPosition;
  };

  MOZ_ASSERT(location);

  nsRefPtr<nsGeoPosition> somewhere = new nsGeoPosition(location->latitude,
                                                        location->longitude,
                                                        location->altitude,
                                                        location->accuracy,
                                                        location->accuracy,
                                                        location->bearing,
                                                        location->speed,
                                                        location->timestamp);
  NS_DispatchToMainThread(new UpdateLocationEvent(somewhere));
}

void
GonkGPSGeolocationProvider::StatusCallback(GpsStatus* status)
{
}

void
GonkGPSGeolocationProvider::SvStatusCallback(GpsSvStatus* sv_info)
{
}

void
GonkGPSGeolocationProvider::NmeaCallback(GpsUtcTime timestamp, const char* nmea, int length)
{
#if DEBUG_GPS
  printf_stderr("*** nmea info\n");
  printf_stderr("timestamp:\t%lld\n", timestamp);
  printf_stderr("nmea:     \t%s\n", nmea);
  printf_stderr("length:   \t%d\n", length);
#endif
}

void
GonkGPSGeolocationProvider::SetCapabilitiesCallback(uint32_t capabilities)
{
  class UpdateCapabilitiesEvent : public nsRunnable {
  public:
    UpdateCapabilitiesEvent(uint32_t aCapabilities)
      : mCapabilities(aCapabilities)
    {}
    NS_IMETHOD Run() {
      nsRefPtr<GonkGPSGeolocationProvider> provider =
        GonkGPSGeolocationProvider::GetSingleton();

      provider->mSupportsScheduling = mCapabilities & GPS_CAPABILITY_SCHEDULING;
      provider->mSupportsMSB = mCapabilities & GPS_CAPABILITY_MSB;
      provider->mSupportsMSA = mCapabilities & GPS_CAPABILITY_MSA;
      provider->mSupportsSingleShot = mCapabilities & GPS_CAPABILITY_SINGLE_SHOT;
#ifdef GPS_CAPABILITY_ON_DEMAND_TIME
      provider->mSupportsTimeInjection = mCapabilities & GPS_CAPABILITY_ON_DEMAND_TIME;
#endif
      return NS_OK;
    }
  private:
    uint32_t mCapabilities;
  };

  NS_DispatchToMainThread(new UpdateCapabilitiesEvent(capabilities));
}

void
GonkGPSGeolocationProvider::AcquireWakelockCallback()
{
}

void
GonkGPSGeolocationProvider::ReleaseWakelockCallback()
{
}

typedef void *(*pthread_func)(void *);



pthread_t
GonkGPSGeolocationProvider::CreateThreadCallback(const char* name, void (*start)(void *), void* arg)
{
  pthread_t thread;
  pthread_attr_t attr;

  pthread_attr_init(&attr);

  


  pthread_create(&thread, &attr, reinterpret_cast<pthread_func>(start), arg);

  return thread;
}

void
GonkGPSGeolocationProvider::RequestUtcTimeCallback()
{
}

void
GonkGPSGeolocationProvider::AGPSStatusCallback(AGpsStatus* status)
{
  MOZ_ASSERT(status);

  class AGPSStatusEvent : public nsRunnable {
  public:
    AGPSStatusEvent(AGpsStatusValue aStatus)
      : mStatus(aStatus)
    {}
    NS_IMETHOD Run() {
      nsRefPtr<GonkGPSGeolocationProvider> provider =
        GonkGPSGeolocationProvider::GetSingleton();

      switch (mStatus) {
        case GPS_REQUEST_AGPS_DATA_CONN:
          provider->RequestDataConnection();
          break;
        case GPS_RELEASE_AGPS_DATA_CONN:
          provider->ReleaseDataConnection();
          break;
      }
      return NS_OK;
    }
  private:
    AGpsStatusValue mStatus;
  };

  NS_DispatchToMainThread(new AGPSStatusEvent(status->status));
}

void
GonkGPSGeolocationProvider::AGPSRILSetIDCallback(uint32_t flags)
{
  class RequestSetIDEvent : public nsRunnable {
  public:
    RequestSetIDEvent(uint32_t flags)
      : mFlags(flags)
    {}
    NS_IMETHOD Run() {
      nsRefPtr<GonkGPSGeolocationProvider> provider =
        GonkGPSGeolocationProvider::GetSingleton();
      provider->RequestSetID(mFlags);
      return NS_OK;
    }
  private:
    uint32_t mFlags;
  };

  NS_DispatchToMainThread(new RequestSetIDEvent(flags));
}

void
GonkGPSGeolocationProvider::AGPSRILRefLocCallback(uint32_t flags)
{
  nsRefPtr<GonkGPSGeolocationProvider> provider =
    GonkGPSGeolocationProvider::GetSingleton();

  if (flags & AGPS_RIL_REQUEST_REFLOC_CELLID) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(provider, &GonkGPSGeolocationProvider::SetReferenceLocation);
    NS_DispatchToMainThread(event);
  }
}

GonkGPSGeolocationProvider::GonkGPSGeolocationProvider()
  : mStarted(false)
  , mSupportsScheduling(false)
  , mSupportsMSB(false) 
  , mSupportsMSA(false) 
  , mSupportsSingleShot(false)
  , mSupportsTimeInjection(false)
  , mGpsInterface(nullptr)
{
}

GonkGPSGeolocationProvider::~GonkGPSGeolocationProvider()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mStarted, "Must call Shutdown before destruction");

  sSingleton = nullptr;
}

already_AddRefed<GonkGPSGeolocationProvider>
GonkGPSGeolocationProvider::GetSingleton()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!sSingleton)
    sSingleton = new GonkGPSGeolocationProvider();

  NS_ADDREF(sSingleton);
  return sSingleton;
}

const GpsInterface*
GonkGPSGeolocationProvider::GetGPSInterface()
{
  hw_module_t* module;

  if (hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module))
    return nullptr;

  hw_device_t* device;
  if (module->methods->open(module, GPS_HARDWARE_MODULE_ID, &device))
    return nullptr;

  gps_device_t* gps_device = (gps_device_t *)device;
  const GpsInterface* result = gps_device->get_gps_interface(gps_device);

  if (result->size != sizeof(GpsInterface)) {
    return nullptr;
  }
  return result;
}

void
GonkGPSGeolocationProvider::RequestDataConnection()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mRIL) {
    return;
  }

  
  
  const nsAdoptingString& apnName = Preferences::GetString("geo.gps.apn.name");
  const nsAdoptingString& apnUser = Preferences::GetString("geo.gps.apn.user");
  const nsAdoptingString& apnPass = Preferences::GetString("geo.gps.apn.password");
  if (apnName && apnUser && apnPass) {
    mCid.Truncate();
    mRIL->SetupDataCall(1 ,
                        apnName, apnUser, apnPass,
                        3 ,
                        NS_LITERAL_STRING("IP") );
  }
}

void
GonkGPSGeolocationProvider::ReleaseDataConnection()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mRIL) {
    return;
  }

  if (mCid.IsEmpty()) {
    
    return;
  }
  mRIL->DeactivateDataCall(mCid, NS_LITERAL_STRING("Close SUPL session"));
}

void
GonkGPSGeolocationProvider::RequestSetID(uint32_t flags)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mRIL) {
    return;
  }

  AGpsSetIDType type = AGPS_SETID_TYPE_NONE;

  nsCOMPtr<nsIRilContext> rilCtx;
  mRIL->GetRilContext(getter_AddRefs(rilCtx));

  if (rilCtx) {
    nsCOMPtr<nsIICCRecords> icc;
    rilCtx->GetIcc(getter_AddRefs(icc));
    if (icc) {
      nsAutoString id;
      if (flags & AGPS_RIL_REQUEST_SETID_IMSI) {
        type = AGPS_SETID_TYPE_IMSI;
        icc->GetImsi(id);
      }
      if (flags & AGPS_RIL_REQUEST_SETID_MSISDN) {
        type = AGPS_SETID_TYPE_MSISDN;
        icc->GetMsisdn(id);
      }
      NS_ConvertUTF16toUTF8 idBytes(id);
      mAGpsRilInterface->set_set_id(type, idBytes.get());
    }
  }
}

void
GonkGPSGeolocationProvider::SetReferenceLocation()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mRIL) {
    return;
  }

  nsCOMPtr<nsIRilContext> rilCtx;
  mRIL->GetRilContext(getter_AddRefs(rilCtx));

  AGpsRefLocation location;

  
  location.type = AGPS_REF_LOCATION_TYPE_UMTS_CELLID;

  if (rilCtx) {
    nsCOMPtr<nsIICCRecords> icc;
    rilCtx->GetIcc(getter_AddRefs(icc));
    if (icc) {
      icc->GetMcc(&location.u.cellID.mcc);
      icc->GetMnc(&location.u.cellID.mnc);
    }
    nsCOMPtr<nsIDOMMozMobileConnectionInfo> voice;
    rilCtx->GetVoice(getter_AddRefs(voice));
    if (voice) {
      nsCOMPtr<nsIDOMMozMobileCellInfo> cell;
      voice->GetCell(getter_AddRefs(cell));
      if (cell) {
        cell->GetGsmLocationAreaCode(&location.u.cellID.lac);
        cell->GetGsmCellId(&location.u.cellID.cid);
      }
    }
    if (mAGpsRilInterface) {
      mAGpsRilInterface->set_ref_location(&location, sizeof(location));
    }
  }
}

void
GonkGPSGeolocationProvider::Init()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  mGpsInterface = GetGPSInterface();
  if (!mGpsInterface) {
    return;
  }

  if (mGpsInterface->init(&mCallbacks) != 0) {
    return;
  }

  mAGpsInterface =
    static_cast<const AGpsInterface*>(mGpsInterface->get_extension(AGPS_INTERFACE));
  if (mAGpsInterface) {
    mAGpsInterface->init(&mAGPSCallbacks);
  }

  mAGpsRilInterface =
    static_cast<const AGpsRilInterface*>(mGpsInterface->get_extension(AGPS_RIL_INTERFACE));
  if (mAGpsRilInterface) {
    mAGpsRilInterface->init(&mAGPSRILCallbacks);
  }

  NS_DispatchToMainThread(NS_NewRunnableMethod(this, &GonkGPSGeolocationProvider::StartGPS));
}

void
GonkGPSGeolocationProvider::StartGPS()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGpsInterface);

  int32_t update = Preferences::GetInt("geo.default.update", kDefaultPeriod);

  if (mSupportsMSA || mSupportsMSB) {
    SetupAGPS();
  }

  int positionMode = GPS_POSITION_MODE_STANDALONE;
  bool singleShot = false;

  
  if (singleShot && mSupportsMSA) {
    positionMode = GPS_POSITION_MODE_MS_ASSISTED;
  } else if (mSupportsMSB) {
    positionMode = GPS_POSITION_MODE_MS_BASED;
  }
  if (!mSupportsScheduling) {
    update = kDefaultPeriod;
  }

  mGpsInterface->set_position_mode(positionMode,
                                   GPS_POSITION_RECURRENCE_PERIODIC,
                                   update, 0, 0);
#if DEBUG_GPS
  
  mGpsInterface->delete_aiding_data(GPS_DELETE_ALL);
#endif

  mGpsInterface->start();
}

void
GonkGPSGeolocationProvider::SetupAGPS()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mAGpsInterface);

  const nsAdoptingCString& suplServer = Preferences::GetCString("geo.gps.supl_server");
  int32_t suplPort = Preferences::GetInt("geo.gps.supl_port", -1);
  if (!suplServer.IsEmpty() && suplPort > 0) {
    mAGpsInterface->set_server(AGPS_TYPE_SUPL, suplServer.get(), suplPort);
  } else {
    NS_WARNING("Cannot get SUPL server settings");
    return;
  }

  
  nsIInterfaceRequestor* ireq = dom::gonk::SystemWorkerManager::GetInterfaceRequestor();
  if (ireq) {
    mRIL = do_GetInterface(ireq);
    if (mRIL) {
      mRIL->RegisterDataCallCallback(this);
    }
  }

  return;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Startup()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mStarted) {
    return NS_OK;
  }

  if (!mInitThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mInitThread));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mInitThread->Dispatch(NS_NewRunnableMethod(this, &GonkGPSGeolocationProvider::Init),
                        NS_DISPATCH_NORMAL);

  mStarted = true;
  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Watch(nsIGeolocationUpdate* aCallback)
{
  MOZ_ASSERT(NS_IsMainThread());

  mLocationCallback = aCallback;
  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mInitThread);

  if (!mStarted) {
    return NS_OK;
  }
  mStarted = false;

  if (mRIL) {
    mRIL->UnregisterDataCallCallback(this);
  }

  mInitThread->Dispatch(NS_NewRunnableMethod(this, &GonkGPSGeolocationProvider::ShutdownGPS),
                        NS_DISPATCH_NORMAL);

  return NS_OK;
}

void
GonkGPSGeolocationProvider::ShutdownGPS()
{
  MOZ_ASSERT(!mStarted, "Should only be called after Shutdown");

  if (mGpsInterface) {
    mGpsInterface->stop();
    mGpsInterface->cleanup();
  }

  mInitThread = nullptr;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::SetHighAccuracy(bool)
{
  return NS_OK;
}



NS_IMETHODIMP
GonkGPSGeolocationProvider::DataCallStateChanged(nsIRILDataCallInfo* aDataCall)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aDataCall);
  MOZ_ASSERT(mAGpsInterface);
  nsCOMPtr<nsIRILDataCallInfo> datacall = aDataCall;

  uint32_t callState;
  nsresult rv = datacall->GetState(&callState);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString apn;
  rv = datacall->GetApn(apn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = datacall->GetCid(mCid);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF16toUTF8 currentApn(apn);
  const nsAdoptingCString& agpsApn = Preferences::GetCString("geo.gps.apn.name");

  
  if (currentApn == agpsApn) {
    switch (callState) {
      case nsINetworkInterface::NETWORK_STATE_CONNECTED:
#ifdef AGPS_HAVE_DUAL_APN
        mAGpsInterface->data_conn_open(AGPS_TYPE_SUPL,
                                       agpsApn.get(),
                                       AGPS_APN_BEARER_IPV4);
#else
        mAGpsInterface->data_conn_open(agpsApn.get());
#endif
        break;
      case nsINetworkInterface::NETWORK_STATE_DISCONNECTED:
#ifdef AGPS_HAVE_DUAL_APN
        mAGpsInterface->data_conn_closed(AGPS_TYPE_SUPL);
#else
        mAGpsInterface->data_conn_closed();
#endif
        break;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::ReceiveDataCallList(nsIRILDataCallInfo** aDataCalls,
                                                uint32_t aLength)
{
  return NS_OK;
}
