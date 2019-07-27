





#include "MozMtpStorage.h"
#include "MozMtpDatabase.h"
#include "MozMtpServer.h"

#include "base/message_loop.h"
#include "nsXULAppAPI.h"

BEGIN_MTP_NAMESPACE
using namespace android;

MozMtpStorage::MozMtpStorage(Volume* aVolume, MozMtpServer* aMozMtpServer)
  : mMozMtpServer(aMozMtpServer),
    mVolume(aVolume)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  
  
  
  mStorageID = mVolume->Id() << 16 | 1;

  MTP_LOG("Storage constructed for Volume %s mStorageID 0x%08x",
          aVolume->NameStr(), mStorageID);

  Volume::RegisterObserver(this);

  
  Notify(mVolume);
}

MozMtpStorage::~MozMtpStorage()
{
  MTP_LOG("Storage destructed for Volume %s mStorageID 0x%08x",
          mVolume->NameStr(), mStorageID);

  Volume::UnregisterObserver(this);
  if (mMtpStorage) {
    StorageUnavailable();
  }
}


void
MozMtpStorage::Notify(Volume* const& aVolume)
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  if (aVolume != mVolume) {
    
    return;
  }
  Volume::STATE volState = aVolume->State();

  MTP_LOG("Volume %s mStorageID 0x%08x state changed to %s SharingEnabled: %d",
          aVolume->NameStr(), mStorageID, aVolume->StateStr(),
          aVolume->IsSharingEnabled());

  if (mMtpStorage) {
    if (volState != nsIVolume::STATE_MOUNTED || !aVolume->IsSharingEnabled()) {
      
      
      StorageUnavailable();
    }
  } else {
    if (volState == nsIVolume::STATE_MOUNTED && aVolume->IsSharingEnabled()) {
      
      StorageAvailable();
    }
  }
}

void
MozMtpStorage::StorageAvailable()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());

  nsCString mountPoint = mVolume->MountPoint();

  MTP_LOG("Adding Volume %s mStorageID 0x%08x mountPoint %s to MozMtpDatabase",
          mVolume->NameStr(), mStorageID, mountPoint.get());

  nsRefPtr<MozMtpDatabase> db = mMozMtpServer->GetMozMtpDatabase();
  db->AddStorage(mStorageID, mountPoint.get(), mVolume->NameStr());

  MOZ_ASSERT(!mMtpStorage);

  
  
  

  

  mMtpStorage.reset(new MtpStorage(mStorageID,         
                                   mountPoint.get(),   
                                   mVolume->NameStr(), 
                                   1024uLL * 1024uLL,  
                                   true,               
                                   2uLL * 1024uLL * 1024uLL * 1024uLL)); 
  nsRefPtr<RefCountedMtpServer> server = mMozMtpServer->GetMtpServer();

  MTP_LOG("Adding Volume %s mStorageID 0x%08x mountPoint %s to MtpServer",
          mVolume->NameStr(), mStorageID, mountPoint.get());
  server->addStorage(mMtpStorage.get());
}

void
MozMtpStorage::StorageUnavailable()
{
  MOZ_ASSERT(MessageLoop::current() == XRE_GetIOMessageLoop());
  MOZ_ASSERT(mMtpStorage);

  MTP_LOG("Removing mStorageID 0x%08x from MtpServer", mStorageID);

  nsRefPtr<RefCountedMtpServer> server = mMozMtpServer->GetMtpServer();
  server->removeStorage(mMtpStorage.get());

  MTP_LOG("Removing mStorageID 0x%08x from MozMtpDatabse", mStorageID);

  nsRefPtr<MozMtpDatabase> db = mMozMtpServer->GetMozMtpDatabase();
  db->RemoveStorage(mStorageID);

  mMtpStorage = nullptr;
}

END_MTP_NAMESPACE


