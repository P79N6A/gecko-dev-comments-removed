




#ifndef GMPStorageParent_h_
#define GMPStorageParent_h_

#include "mozilla/gmp/PGMPStorageParent.h"
#include "gmp-storage.h"
#include "nsTHashtable.h"
#include "nsDataHashtable.h"
#include "prio.h"

namespace mozilla {
namespace gmp {

class GMPParent;

class GMPStorageParent : public PGMPStorageParent {
public:
  NS_INLINE_DECL_REFCOUNTING(GMPStorageParent)
  GMPStorageParent(const nsString& aOrigin, GMPParent* aPlugin);

  void Shutdown();

protected:
  virtual bool RecvOpen(const nsCString& aRecordName) MOZ_OVERRIDE;
  virtual bool RecvRead(const nsCString& aRecordName) MOZ_OVERRIDE;
  virtual bool RecvWrite(const nsCString& aRecordName,
                         const InfallibleTArray<uint8_t>& aBytes) MOZ_OVERRIDE;
  virtual bool RecvClose(const nsCString& aRecordName) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  nsDataHashtable<nsCStringHashKey, PRFileDesc*> mFiles;
  const nsString mOrigin;
  nsRefPtr<GMPParent> mPlugin;
  bool mShutdown;
};

} 
} 

#endif 
