




#include "GMPContentParent.h"
#include "GMPAudioDecoderParent.h"
#include "GMPDecryptorParent.h"
#include "GMPParent.h"
#include "GMPServiceChild.h"
#include "GMPVideoDecoderParent.h"
#include "GMPVideoEncoderParent.h"
#include "mozIGeckoMediaPluginService.h"
#include "prlog.h"

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetGMPLog();

#define LOGD(msg) PR_LOG(GetGMPLog(), PR_LOG_DEBUG, msg)
#define LOG(level, msg) PR_LOG(GetGMPLog(), (level), msg)
#else
#define LOGD(msg)
#define LOG(level, msg)
#endif

#ifdef __CLASS__
#undef __CLASS__
#endif
#define __CLASS__ "GMPContentParent"

namespace gmp {

GMPContentParent::GMPContentParent(GMPParent* aParent)
  : mParent(aParent)
{
  if (mParent) {
    SetDisplayName(mParent->GetDisplayName());
    SetPluginId(mParent->GetPluginId());
  }
}

GMPContentParent::~GMPContentParent()
{
  XRE_GetIOMessageLoop()->PostTask(FROM_HERE,
                                   new DeleteTask<Transport>(GetTransport()));
}

class ReleaseGMPContentParent : public nsRunnable
{
public:
  explicit ReleaseGMPContentParent(GMPContentParent* aToRelease)
    : mToRelease(aToRelease)
  {
  }

  NS_IMETHOD Run()
  {
    return NS_OK;
  }

private:
  nsRefPtr<GMPContentParent> mToRelease;
};

void
GMPContentParent::ActorDestroy(ActorDestroyReason aWhy)
{
  MOZ_ASSERT(mAudioDecoders.IsEmpty() &&
             mDecryptors.IsEmpty() &&
             mVideoDecoders.IsEmpty() &&
             mVideoEncoders.IsEmpty());
  NS_DispatchToCurrentThread(new ReleaseGMPContentParent(this));
}

void
GMPContentParent::CheckThread()
{
  MOZ_ASSERT(mGMPThread == NS_GetCurrentThread());
}

void
GMPContentParent::AudioDecoderDestroyed(GMPAudioDecoderParent* aDecoder)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  MOZ_ALWAYS_TRUE(mAudioDecoders.RemoveElement(aDecoder));
  CloseIfUnused();
}

void
GMPContentParent::VideoDecoderDestroyed(GMPVideoDecoderParent* aDecoder)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  
  unused << NS_WARN_IF(!mVideoDecoders.RemoveElement(aDecoder));
  CloseIfUnused();
}

void
GMPContentParent::VideoEncoderDestroyed(GMPVideoEncoderParent* aEncoder)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  
  unused << NS_WARN_IF(!mVideoEncoders.RemoveElement(aEncoder));
  CloseIfUnused();
}

void
GMPContentParent::DecryptorDestroyed(GMPDecryptorParent* aSession)
{
  MOZ_ASSERT(GMPThread() == NS_GetCurrentThread());

  MOZ_ALWAYS_TRUE(mDecryptors.RemoveElement(aSession));
  CloseIfUnused();
}

void
GMPContentParent::CloseIfUnused()
{
  if (mAudioDecoders.IsEmpty() &&
      mDecryptors.IsEmpty() &&
      mVideoDecoders.IsEmpty() &&
      mVideoEncoders.IsEmpty()) {
    nsRefPtr<GMPContentParent> toClose;
    if (mParent) {
      toClose = mParent->ForgetGMPContentParent();
    } else {
      toClose = this;
      nsRefPtr<GeckoMediaPluginServiceChild> gmp(
        GeckoMediaPluginServiceChild::GetSingleton());
      gmp->RemoveGMPContentParent(toClose);
    }
    NS_DispatchToCurrentThread(NS_NewRunnableMethod(toClose,
                                                    &GMPContentParent::Close));
  }
}

nsresult
GMPContentParent::GetGMPDecryptor(GMPDecryptorParent** aGMPDP)
{
  PGMPDecryptorParent* pdp = SendPGMPDecryptorConstructor();
  if (!pdp) {
    return NS_ERROR_FAILURE;
  }
  GMPDecryptorParent* dp = static_cast<GMPDecryptorParent*>(pdp);
  
  
  NS_ADDREF(dp);
  mDecryptors.AppendElement(dp);
  *aGMPDP = dp;

  return NS_OK;
}

nsIThread*
GMPContentParent::GMPThread()
{
  if (!mGMPThread) {
    nsCOMPtr<mozIGeckoMediaPluginService> mps = do_GetService("@mozilla.org/gecko-media-plugin-service;1");
    MOZ_ASSERT(mps);
    if (!mps) {
      return nullptr;
    }
    
    
    
    
    
    mps->GetThread(getter_AddRefs(mGMPThread));
    MOZ_ASSERT(mGMPThread);
  }

  return mGMPThread;
}

nsresult
GMPContentParent::GetGMPAudioDecoder(GMPAudioDecoderParent** aGMPAD)
{
  PGMPAudioDecoderParent* pvap = SendPGMPAudioDecoderConstructor();
  if (!pvap) {
    return NS_ERROR_FAILURE;
  }
  GMPAudioDecoderParent* vap = static_cast<GMPAudioDecoderParent*>(pvap);
  
  
  NS_ADDREF(vap);
  *aGMPAD = vap;
  mAudioDecoders.AppendElement(vap);

  return NS_OK;
}

nsresult
GMPContentParent::GetGMPVideoDecoder(GMPVideoDecoderParent** aGMPVD)
{
  
  PGMPVideoDecoderParent* pvdp = SendPGMPVideoDecoderConstructor();
  if (!pvdp) {
    return NS_ERROR_FAILURE;
  }
  GMPVideoDecoderParent *vdp = static_cast<GMPVideoDecoderParent*>(pvdp);
  
  
  NS_ADDREF(vdp);
  *aGMPVD = vdp;
  mVideoDecoders.AppendElement(vdp);

  return NS_OK;
}

nsresult
GMPContentParent::GetGMPVideoEncoder(GMPVideoEncoderParent** aGMPVE)
{
  
  PGMPVideoEncoderParent* pvep = SendPGMPVideoEncoderConstructor();
  if (!pvep) {
    return NS_ERROR_FAILURE;
  }
  GMPVideoEncoderParent *vep = static_cast<GMPVideoEncoderParent*>(pvep);
  
  
  NS_ADDREF(vep);
  *aGMPVE = vep;
  mVideoEncoders.AppendElement(vep);

  return NS_OK;
}

PGMPVideoDecoderParent*
GMPContentParent::AllocPGMPVideoDecoderParent()
{
  GMPVideoDecoderParent* vdp = new GMPVideoDecoderParent(this);
  NS_ADDREF(vdp);
  return vdp;
}

bool
GMPContentParent::DeallocPGMPVideoDecoderParent(PGMPVideoDecoderParent* aActor)
{
  GMPVideoDecoderParent* vdp = static_cast<GMPVideoDecoderParent*>(aActor);
  NS_RELEASE(vdp);
  return true;
}

PGMPVideoEncoderParent*
GMPContentParent::AllocPGMPVideoEncoderParent()
{
  GMPVideoEncoderParent* vep = new GMPVideoEncoderParent(this);
  NS_ADDREF(vep);
  return vep;
}

bool
GMPContentParent::DeallocPGMPVideoEncoderParent(PGMPVideoEncoderParent* aActor)
{
  GMPVideoEncoderParent* vep = static_cast<GMPVideoEncoderParent*>(aActor);
  NS_RELEASE(vep);
  return true;
}

PGMPDecryptorParent*
GMPContentParent::AllocPGMPDecryptorParent()
{
  GMPDecryptorParent* ksp = new GMPDecryptorParent(this);
  NS_ADDREF(ksp);
  return ksp;
}

bool
GMPContentParent::DeallocPGMPDecryptorParent(PGMPDecryptorParent* aActor)
{
  GMPDecryptorParent* ksp = static_cast<GMPDecryptorParent*>(aActor);
  NS_RELEASE(ksp);
  return true;
}

PGMPAudioDecoderParent*
GMPContentParent::AllocPGMPAudioDecoderParent()
{
  GMPAudioDecoderParent* vdp = new GMPAudioDecoderParent(this);
  NS_ADDREF(vdp);
  return vdp;
}

bool
GMPContentParent::DeallocPGMPAudioDecoderParent(PGMPAudioDecoderParent* aActor)
{
  GMPAudioDecoderParent* vdp = static_cast<GMPAudioDecoderParent*>(aActor);
  NS_RELEASE(vdp);
  return true;
}

} 
} 
