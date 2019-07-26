



#ifndef peerconnectionctx_h___h__
#define peerconnectionctx_h___h__

#include <string>

#include "mozilla/Attributes.h"
#include "CallControlManager.h"
#include "CC_Device.h"
#include "CC_DeviceInfo.h"
#include "CC_Call.h"
#include "CC_CallInfo.h"
#include "CC_Line.h"
#include "CC_LineInfo.h"
#include "CC_Observer.h"
#include "CC_FeatureInfo.h"

#include "StaticPtr.h"
#include "PeerConnectionImpl.h"

namespace mozilla {
class PeerConnectionCtxShutdown;
}

namespace sipcc {






class PeerConnectionCtx : public CSF::CC_Observer {
 public:
  static nsresult InitializeGlobal(nsIThread *mainThread);
  static PeerConnectionCtx* GetInstance();
  static bool isActive();
  static void Destroy();

  
  virtual void onDeviceEvent(ccapi_device_event_e deviceEvent, CSF::CC_DevicePtr device, CSF::CC_DeviceInfoPtr info);
  virtual void onFeatureEvent(ccapi_device_event_e deviceEvent, CSF::CC_DevicePtr device, CSF::CC_FeatureInfoPtr feature_info) {}
  virtual void onLineEvent(ccapi_line_event_e lineEvent, CSF::CC_LinePtr line, CSF::CC_LineInfoPtr info) {}
  virtual void onCallEvent(ccapi_call_event_e callEvent, CSF::CC_CallPtr call, CSF::CC_CallInfoPtr info);

  
  CSF::CC_CallPtr createCall();

  PeerConnectionImpl::SipccState sipcc_state() { return mSipccState; }

  
  friend class PeerConnectionImpl;
  friend class PeerConnectionWrapper;

 private:
  
  std::map<const std::string, PeerConnectionImpl *> mPeerConnections;

  PeerConnectionCtx() :  mSipccState(PeerConnectionImpl::kIdle),
                         mCCM(NULL), mDevice(NULL) {}
  
  PeerConnectionCtx(const PeerConnectionCtx& other) MOZ_DELETE;
  void operator=(const PeerConnectionCtx& other) MOZ_DELETE;
  virtual ~PeerConnectionCtx() {};

  nsresult Initialize();
  nsresult Cleanup();

  void ChangeSipccState(PeerConnectionImpl::SipccState aState) {
    mSipccState = aState;
  }

  virtual void onCallEvent_m(ccapi_call_event_e callEvent,
			     CSF::CC_CallPtr call,
			     CSF::CC_CallInfoPtr info);

  
  PeerConnectionImpl::SipccState mSipccState;  
  CSF::CallControlManagerPtr mCCM;
  CSF::CC_DevicePtr mDevice;

  static PeerConnectionCtx *gInstance;
  static nsIThread *gMainThread;
public:
  static StaticRefPtr<mozilla::PeerConnectionCtxShutdown> gPeerConnectionCtxShutdown;
};

}  

#endif
