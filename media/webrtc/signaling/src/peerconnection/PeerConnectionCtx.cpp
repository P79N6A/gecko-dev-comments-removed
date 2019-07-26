




#include "CallControlManager.h"
#include "CC_Device.h"
#include "CC_Call.h"
#include "CC_Observer.h"
#include "ccapi_call_info.h"
#include "CC_SIPCCCallInfo.h"
#include "ccapi_device_info.h"
#include "CC_SIPCCDeviceInfo.h"
#include "CSFLog.h"
#include "vcm.h"
#include "PeerConnectionImpl.h"
#include "PeerConnectionCtx.h"
#include "cpr_socket.h"

static const char* logTag = "PeerConnectionCtx";

namespace sipcc {

PeerConnectionCtx* PeerConnectionCtx::instance;

PeerConnectionCtx* PeerConnectionCtx::GetInstance() {
  if (instance)
    return instance;

  CSFLogDebug(logTag, "Creating PeerConnectionCtx");
  PeerConnectionCtx *ctx = new PeerConnectionCtx();

  nsresult res = ctx->Initialize();
  PR_ASSERT(NS_SUCCEEDED(res));
  if (!NS_SUCCEEDED(res))
    return NULL;

  instance = ctx;

  return instance;
}

void PeerConnectionCtx::Destroy() {
  instance->Cleanup();
  delete instance;
  instance = NULL;
}

nsresult PeerConnectionCtx::Initialize() {
  mCCM = CSF::CallControlManager::create();

  NS_ENSURE_TRUE(mCCM.get(), NS_ERROR_FAILURE);

  
  
  int codecMask = 0;
  codecMask |= VCM_CODEC_RESOURCE_G711;
  codecMask |= VCM_CODEC_RESOURCE_OPUS;
  
  
  
  
  mCCM->setAudioCodecs(codecMask);

  
  
  
  codecMask = 0;
  
  

  
  codecMask |= VCM_CODEC_RESOURCE_VP8;
  
  mCCM->setVideoCodecs(codecMask);

  if (!mCCM->startSDPMode())
    return NS_ERROR_FAILURE;

  mCCM->addCCObserver(this);
  mDevice = mCCM->getActiveDevice();
  NS_ENSURE_TRUE(mDevice.get(), NS_ERROR_FAILURE);
  ChangeSipccState(PeerConnectionImpl::kStarting);
  return NS_OK;
}

nsresult PeerConnectionCtx::Cleanup() {
  mCCM->destroy();
  mCCM->removeCCObserver(this);
  return NS_OK;
}

CSF::CC_CallPtr PeerConnectionCtx::createCall() {
  return mDevice->createCall();
}

void PeerConnectionCtx::onDeviceEvent(ccapi_device_event_e aDeviceEvent,
                                      CSF::CC_DevicePtr aDevice,
                                      CSF::CC_DeviceInfoPtr aInfo ) {
  CSFLogDebug(logTag, "onDeviceEvent()");
  cc_service_state_t state = aInfo->getServiceState();

  if (CC_STATE_INS == state) {
    
    if (PeerConnectionImpl::kStarting == mSipccState ||
        PeerConnectionImpl::kIdle == mSipccState) {
      ChangeSipccState(PeerConnectionImpl::kStarted);
    } else {
      CSFLogError(logTag, "%s PeerConnection in wrong state", __FUNCTION__);
      Cleanup();
      MOZ_ASSERT(PR_FALSE);
    }
  } else {
    Cleanup();
    NS_NOTREACHED("Unsupported Signaling State Transition");
  }
}


void PeerConnectionCtx::onCallEvent(ccapi_call_event_e aCallEvent,
                                    CSF::CC_CallPtr aCall,
                                    CSF::CC_CallInfoPtr aInfo) {
  CSFLogDebug(logTag, "onCallEvent()");
  mozilla::ScopedDeletePtr<PeerConnectionWrapper> pc(
    PeerConnectionImpl::AcquireInstance(
      aCall->getPeerConnection()));

  if (!pc)  
    return;

  CSFLogDebug(logTag, "Calling PC");
  pc->impl()->onCallEvent(aCallEvent, aCall, aInfo);
}

}  
