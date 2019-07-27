




#ifndef GMPStorageParent_h_
#define GMPStorageParent_h_

#include "mozilla/gmp/PGMPStorageParent.h"
#include "gmp-storage.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {
namespace gmp {

class GMPParent;

class GMPStorage {
public:
  virtual ~GMPStorage() {}

  virtual GMPErr Open(const nsCString& aRecordName) = 0;
  virtual bool IsOpen(const nsCString& aRecordName) = 0;
  virtual GMPErr Read(const nsCString& aRecordName,
                      nsTArray<uint8_t>& aOutBytes) = 0;
  virtual GMPErr Write(const nsCString& aRecordName,
                       const nsTArray<uint8_t>& aBytes) = 0;
  virtual GMPErr GetRecordNames(nsTArray<nsCString>& aOutRecordNames) = 0;
  virtual void Close(const nsCString& aRecordName) = 0;
};

class GMPStorageParent : public PGMPStorageParent {
public:
  NS_INLINE_DECL_REFCOUNTING(GMPStorageParent)
  GMPStorageParent(const nsCString& aNodeId,
                   GMPParent* aPlugin);

  nsresult Init();
  void Shutdown();

protected:
  virtual bool RecvOpen(const nsCString& aRecordName) MOZ_OVERRIDE;
  virtual bool RecvRead(const nsCString& aRecordName) MOZ_OVERRIDE;
  virtual bool RecvWrite(const nsCString& aRecordName,
                         const InfallibleTArray<uint8_t>& aBytes) MOZ_OVERRIDE;
  virtual bool RecvGetRecordNames() MOZ_OVERRIDE;
  virtual bool RecvClose(const nsCString& aRecordName) MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

private:
  ~GMPStorageParent() {}

  UniquePtr<GMPStorage> mStorage;

  const nsCString mNodeId;
  nsRefPtr<GMPParent> mPlugin;
  bool mShutdown;
};

} 
} 

#endif 
