




#ifndef GMPStorageChild_h_
#define GMPStorageChild_h_

#include "mozilla/gmp/PGMPStorageChild.h"
#include "gmp-storage.h"
#include "nsTHashtable.h"
#include "nsRefPtrHashtable.h"
#include "gmp-platform.h"

#include <queue>

namespace mozilla {
namespace gmp {

class GMPChild;
class GMPStorageChild;

class GMPRecordImpl : public GMPRecord
{
public:
  NS_INLINE_DECL_REFCOUNTING(GMPRecordImpl)

  GMPRecordImpl(GMPStorageChild* aOwner,
                const nsCString& aName,
                GMPRecordClient* aClient);

  
  virtual GMPErr Open() MOZ_OVERRIDE;
  virtual GMPErr Read() MOZ_OVERRIDE;
  virtual GMPErr Write(const uint8_t* aData,
                       uint32_t aDataSize) MOZ_OVERRIDE;
  virtual GMPErr Close() MOZ_OVERRIDE;

  const nsCString& Name() const { return mName; }

  void OpenComplete(GMPErr aStatus);
  void ReadComplete(GMPErr aStatus, const uint8_t* aBytes, uint32_t aLength);
  void WriteComplete(GMPErr aStatus);

  void MarkClosed();

private:
  ~GMPRecordImpl() {}
  const nsCString mName;
  GMPRecordClient* const mClient;
  GMPStorageChild* const mOwner;
  bool mIsClosed;
};

class GMPStorageChild : public PGMPStorageChild
{
public:
  NS_INLINE_DECL_REFCOUNTING(GMPStorageChild)

  explicit GMPStorageChild(GMPChild* aPlugin);

  GMPErr CreateRecord(const nsCString& aRecordName,
                      GMPRecord** aOutRecord,
                      GMPRecordClient* aClient);

  GMPErr Open(GMPRecordImpl* aRecord);

  GMPErr Read(GMPRecordImpl* aRecord);

  GMPErr Write(GMPRecordImpl* aRecord,
               const uint8_t* aData,
               uint32_t aDataSize);

  GMPErr Close(GMPRecordImpl* aRecord);

  GMPErr EnumerateRecords(RecvGMPRecordIteratorPtr aRecvIteratorFunc,
                          void* aUserArg);

protected:
  ~GMPStorageChild() {}

  
  virtual bool RecvOpenComplete(const nsCString& aRecordName,
                                const GMPErr& aStatus) MOZ_OVERRIDE;
  virtual bool RecvReadComplete(const nsCString& aRecordName,
                                const GMPErr& aStatus,
                                const InfallibleTArray<uint8_t>& aBytes) MOZ_OVERRIDE;
  virtual bool RecvWriteComplete(const nsCString& aRecordName,
                                 const GMPErr& aStatus) MOZ_OVERRIDE;
  virtual bool RecvRecordNames(const InfallibleTArray<nsCString>& aRecordNames,
                               const GMPErr& aStatus) MOZ_OVERRIDE;
  virtual bool RecvShutdown() MOZ_OVERRIDE;

private:
  nsRefPtrHashtable<nsCStringHashKey, GMPRecordImpl> mRecords;
  GMPChild* mPlugin;

  struct RecordIteratorContext {
    explicit RecordIteratorContext(RecvGMPRecordIteratorPtr aFunc,
                                   void* aUserArg)
      : mFunc(aFunc)
      , mUserArg(aUserArg)
    {}
    RecvGMPRecordIteratorPtr mFunc;
    void* mUserArg;
  };

  std::queue<RecordIteratorContext> mPendingRecordIterators;
  bool mShutdown;
};

} 
} 

#endif 
