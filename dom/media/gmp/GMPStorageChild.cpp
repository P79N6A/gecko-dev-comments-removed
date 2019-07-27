




#include "GMPStorageChild.h"
#include "GMPChild.h"
#include "gmp-storage.h"

namespace mozilla {
namespace gmp {

GMPRecordImpl::GMPRecordImpl(GMPStorageChild* aOwner,
                             const nsCString& aName,
                             GMPRecordClient* aClient)
  : mName(aName)
  , mClient(aClient)
  , mOwner(aOwner)
  , mIsClosed(true)
{
}

GMPErr
GMPRecordImpl::Open()
{
  if (!mIsClosed) {
    return GMPRecordInUse;
  }
  return mOwner->Open(this);
}

void
GMPRecordImpl::OpenComplete(GMPErr aStatus)
{
  mIsClosed = false;
  mClient->OpenComplete(aStatus);
}

GMPErr
GMPRecordImpl::Read()
{
  if (mIsClosed) {
    return GMPClosedErr;
  }
  return mOwner->Read(this);
}

void
GMPRecordImpl::ReadComplete(GMPErr aStatus,
                            const uint8_t* aBytes,
                            uint32_t aLength)
{
  mClient->ReadComplete(aStatus, aBytes, aLength);
}

GMPErr
GMPRecordImpl::Write(const uint8_t* aData, uint32_t aDataSize)
{
  if (mIsClosed) {
    return GMPClosedErr;
  }
  return mOwner->Write(this, aData, aDataSize);
}

void
GMPRecordImpl::WriteComplete(GMPErr aStatus)
{
  mClient->WriteComplete(aStatus);
}

GMPErr
GMPRecordImpl::Close()
{
  nsRefPtr<GMPRecordImpl> kungfuDeathGrip(this);

  if (!mIsClosed) {
    
    mOwner->Close(this);
    
    MOZ_ASSERT(mIsClosed);
  }

  
  Release();

  return GMPNoErr;
}

void
GMPRecordImpl::MarkClosed()
{
  mIsClosed = true;
}

GMPStorageChild::GMPStorageChild(GMPChild* aPlugin)
  : mPlugin(aPlugin)
  , mShutdown(false)
{
  MOZ_ASSERT(mPlugin->GMPMessageLoop() == MessageLoop::current());
}

GMPErr
GMPStorageChild::CreateRecord(const nsCString& aRecordName,
                              GMPRecord** aOutRecord,
                              GMPRecordClient* aClient)
{
  if (mPlugin->GMPMessageLoop() != MessageLoop::current()) {
    NS_WARNING("GMP used GMPStorage on non-main thread.");
    return GMPGenericErr;
  }
  if (mShutdown) {
    NS_WARNING("GMPStorage used after it's been shutdown!");
    return GMPClosedErr;
  }
  MOZ_ASSERT(aRecordName.Length() && aOutRecord);
  nsRefPtr<GMPRecordImpl> record(new GMPRecordImpl(this, aRecordName, aClient));
  mRecords.Put(aRecordName, record); 

  
  
  
  record.forget(aOutRecord);

  return GMPNoErr;
}

GMPErr
GMPStorageChild::Open(GMPRecordImpl* aRecord)
{
  if (mPlugin->GMPMessageLoop() != MessageLoop::current()) {
    NS_WARNING("GMP used GMPStorage on non-main thread.");
    return GMPGenericErr;
  }
  if (mShutdown) {
    NS_WARNING("GMPStorage used after it's been shutdown!");
    return GMPClosedErr;
  }
  if (!SendOpen(aRecord->Name())) {
    Close(aRecord);
    return GMPClosedErr;
  }
  return GMPNoErr;
}

GMPErr
GMPStorageChild::Read(GMPRecordImpl* aRecord)
{
  if (mPlugin->GMPMessageLoop() != MessageLoop::current()) {
    NS_WARNING("GMP used GMPStorage on non-main thread.");
    return GMPGenericErr;
  }
  if (mShutdown) {
    NS_WARNING("GMPStorage used after it's been shutdown!");
    return GMPClosedErr;
  }
  if (!SendRead(aRecord->Name())) {
    Close(aRecord);
    return GMPClosedErr;
  }
  return GMPNoErr;
}

GMPErr
GMPStorageChild::Write(GMPRecordImpl* aRecord,
                       const uint8_t* aData,
                       uint32_t aDataSize)
{
  if (mPlugin->GMPMessageLoop() != MessageLoop::current()) {
    NS_WARNING("GMP used GMPStorage on non-main thread.");
    return GMPGenericErr;
  }
  if (mShutdown) {
    NS_WARNING("GMPStorage used after it's been shutdown!");
    return GMPClosedErr;
  }
  if (aDataSize > GMP_MAX_RECORD_SIZE) {
    return GMPQuotaExceededErr;
  }
  nsTArray<uint8_t> data;
  data.AppendElements(aData, aDataSize);
  if (!SendWrite(aRecord->Name(), data)) {
    Close(aRecord);
    return GMPClosedErr;
  }
  return GMPNoErr;
}

GMPErr
GMPStorageChild::Close(GMPRecordImpl* aRecord)
{
  if (mPlugin->GMPMessageLoop() != MessageLoop::current()) {
    NS_WARNING("GMP used GMPStorage on non-main thread.");
    return GMPGenericErr;
  }
  if (!mRecords.Contains(aRecord->Name())) {
    
    return GMPClosedErr;
  }

  GMPErr rv = GMPNoErr;
  if (!mShutdown && !SendClose(aRecord->Name())) {
    rv = GMPGenericErr;
  }

  aRecord->MarkClosed();
  mRecords.Remove(aRecord->Name());

  return rv;
}

bool
GMPStorageChild::RecvOpenComplete(const nsCString& aRecordName,
                                  const GMPErr& aStatus)
{
  if (mShutdown) {
    return true;
  }
  nsRefPtr<GMPRecordImpl> record;
  if (!mRecords.Get(aRecordName, getter_AddRefs(record)) || !record) {
    
    return true;
  }
  record->OpenComplete(aStatus);
  if (GMP_FAILED(aStatus)) {
    Close(record);
  }
  return true;
}

bool
GMPStorageChild::RecvReadComplete(const nsCString& aRecordName,
                                  const GMPErr& aStatus,
                                  const InfallibleTArray<uint8_t>& aBytes)
{
  if (mShutdown) {
    return true;
  }
  nsRefPtr<GMPRecordImpl> record;
  if (!mRecords.Get(aRecordName, getter_AddRefs(record)) || !record) {
    
    return true;
  }
  record->ReadComplete(aStatus,
                       aBytes.Elements(),
                       aBytes.Length());
  if (GMP_FAILED(aStatus)) {
    Close(record);
  }
  return true;
}

bool
GMPStorageChild::RecvWriteComplete(const nsCString& aRecordName,
                                   const GMPErr& aStatus)
{
  if (mShutdown) {
    return true;
  }
  nsRefPtr<GMPRecordImpl> record;
  if (!mRecords.Get(aRecordName, getter_AddRefs(record)) || !record) {
    
    return true;
  }
  record->WriteComplete(aStatus);
  if (GMP_FAILED(aStatus)) {
    Close(record);
  }
  return true;
}

bool
GMPStorageChild::RecvShutdown()
{
  
  
  
  mShutdown = true;
  return true;
}

} 
} 
