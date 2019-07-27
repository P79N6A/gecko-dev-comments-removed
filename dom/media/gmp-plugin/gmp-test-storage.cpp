




#include "gmp-test-storage.h"
#include <vector>

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"

class WriteRecordClient : public GMPRecordClient {
public:
  GMPErr Init(GMPRecord* aRecord,
              GMPTask* aOnSuccess,
              GMPTask* aOnFailure,
              const uint8_t* aData,
              uint32_t aDataSize) {
    mRecord = aRecord;
    mOnSuccess = aOnSuccess;
    mOnFailure = aOnFailure;
    mData.insert(mData.end(), aData, aData + aDataSize);
    return mRecord->Open();
  }

  virtual void OpenComplete(GMPErr aStatus) MOZ_OVERRIDE {
    if (GMP_SUCCEEDED(aStatus)) {
      mRecord->Write(mData.size() ? &mData.front() : nullptr, mData.size());
    } else {
      GMPRunOnMainThread(mOnFailure);
      mOnSuccess->Destroy();
    }
  }

  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aDataSize) MOZ_OVERRIDE {}

  virtual void WriteComplete(GMPErr aStatus) MOZ_OVERRIDE {
    
    
    
    
    
    mRecord->Close();
    if (GMP_SUCCEEDED(aStatus)) {
      GMPRunOnMainThread(mOnSuccess);
      mOnFailure->Destroy();
    } else {
      GMPRunOnMainThread(mOnFailure);
      mOnSuccess->Destroy();
    }
    delete this;
  }

private:
  GMPRecord* mRecord;
  GMPTask* mOnSuccess;
  GMPTask* mOnFailure;
  std::vector<uint8_t> mData;
};

GMPErr
WriteRecord(const std::string& aRecordName,
            const uint8_t* aData,
            uint32_t aNumBytes,
            GMPTask* aOnSuccess,
            GMPTask* aOnFailure)
{
  GMPRecord* record;
  WriteRecordClient* client = new WriteRecordClient();
  auto err = GMPOpenRecord(aRecordName.c_str(),
                           aRecordName.size(),
                           &record,
                           client);
  if (GMP_FAILED(err)) {
    GMPRunOnMainThread(aOnFailure);
    aOnSuccess->Destroy();
    return err;
  }
  return client->Init(record, aOnSuccess, aOnFailure, aData, aNumBytes);
}

GMPErr
WriteRecord(const std::string& aRecordName,
            const std::string& aData,
            GMPTask* aOnSuccess,
            GMPTask* aOnFailure)
{
  return WriteRecord(aRecordName,
                     (const uint8_t*)aData.c_str(),
                     aData.size(),
                     aOnSuccess,
                     aOnFailure);
}

class ReadRecordClient : public GMPRecordClient {
public:
  GMPErr Init(GMPRecord* aRecord,
              ReadContinuation* aContinuation) {
    mRecord = aRecord;
    mContinuation = aContinuation;
    return mRecord->Open();
  }

  virtual void OpenComplete(GMPErr aStatus) MOZ_OVERRIDE {
    auto err = mRecord->Read();
    if (GMP_FAILED(err)) {
      mContinuation->ReadComplete(err, "");
      delete this;
    }
  }

  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aDataSize) MOZ_OVERRIDE {
    
    
    
    
    
    mRecord->Close();
    std::string data((const char*)aData, aDataSize);
    mContinuation->ReadComplete(GMPNoErr, data);
    delete this;
  }

  virtual void WriteComplete(GMPErr aStatus) MOZ_OVERRIDE {
  }

private:
  GMPRecord* mRecord;
  ReadContinuation* mContinuation;
};

GMPErr
ReadRecord(const std::string& aRecordName,
           ReadContinuation* aContinuation)
{
  MOZ_ASSERT(aContinuation);
  GMPRecord* record;
  ReadRecordClient* client = new ReadRecordClient();
  auto err = GMPOpenRecord(aRecordName.c_str(),
                           aRecordName.size(),
                           &record,
                           client);
  if (GMP_FAILED(err)) {
    return err;
  }
  return client->Init(record, aContinuation);
}

extern GMPPlatformAPI* g_platform_api; 

GMPErr
GMPOpenRecord(const char* aName,
              uint32_t aNameLength,
              GMPRecord** aOutRecord,
              GMPRecordClient* aClient)
{
  MOZ_ASSERT(g_platform_api);
  return g_platform_api->createrecord(aName, aNameLength, aOutRecord, aClient);
}

GMPErr
GMPRunOnMainThread(GMPTask* aTask)
{
  MOZ_ASSERT(g_platform_api);
  return g_platform_api->runonmainthread(aTask);
}

class OpenRecordClient : public GMPRecordClient {
public:
  



  static void Open(const std::string& aRecordName,
            OpenContinuation* aContinuation) {
    MOZ_ASSERT(aContinuation);
    (new OpenRecordClient(aContinuation))->Do(aRecordName);
  }

  virtual void OpenComplete(GMPErr aStatus) MOZ_OVERRIDE {
    Done(aStatus);
  }

  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aDataSize) MOZ_OVERRIDE {
    MOZ_CRASH("Should not reach here.");
  }

  virtual void WriteComplete(GMPErr aStatus) MOZ_OVERRIDE {
    MOZ_CRASH("Should not reach here.");
  }

private:
  explicit OpenRecordClient(OpenContinuation* aContinuation)
    : mRecord(nullptr), mContinuation(aContinuation) {}

  void Do(const std::string& aName) {
    auto err = GMPOpenRecord(aName.c_str(), aName.size(), &mRecord, this);
    if (GMP_FAILED(err) ||
        GMP_FAILED(err = mRecord->Open())) {
      Done(err);
    }
  }

  void Done(GMPErr err) {
    
    mContinuation->OpenComplete(err, mRecord);
    delete mContinuation;
    delete this;
  }

  GMPRecord* mRecord;
  OpenContinuation* mContinuation;
};

void
GMPOpenRecord(const std::string& aRecordName,
              OpenContinuation* aContinuation)
{
  OpenRecordClient::Open(aRecordName, aContinuation);
}

GMPErr
GMPEnumRecordNames(RecvGMPRecordIteratorPtr aRecvIteratorFunc,
                   void* aUserArg)
{
  return g_platform_api->getrecordenumerator(aRecvIteratorFunc, aUserArg);
}
