





#ifndef mozilla_system_mozmtpstorage_h__
#define mozilla_system_mozmtpstorage_h__

#include "MozMtpCommon.h"

#include "nsAutoPtr.h"
#include "mozilla/UniquePtr.h"

#include "Volume.h"

BEGIN_MTP_NAMESPACE
using namespace android;

class MozMtpServer;

class MozMtpStorage : public Volume::EventObserver
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MozMtpStorage)

  MozMtpStorage(Volume* aVolume, MozMtpServer* aMozMtpServer);

  typedef nsTArray<nsRefPtr<MozMtpStorage> > Array;

private:
  virtual ~MozMtpStorage();
  virtual void Notify(Volume* const& aEvent);

  void StorageAvailable();
  void StorageUnavailable();

  nsRefPtr<MozMtpServer>  mMozMtpServer;
  UniquePtr<MtpStorage>   mMtpStorage;
  RefPtr<Volume>          mVolume;
  MtpStorageID            mStorageID;
};

END_MTP_NAMESPACE

#endif  


