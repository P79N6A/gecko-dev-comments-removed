




#ifndef TEST_GMP_STORAGE_H__
#define TEST_GMP_STORAGE_H__

#include "gmp-errors.h"
#include "gmp-platform.h"
#include <string>

class ReadContinuation {
public:
  virtual ~ReadContinuation() {}
  virtual void ReadComplete(GMPErr aErr, const std::string& aData) = 0;
};



GMPErr
ReadRecord(const std::string& aRecordName,
           ReadContinuation* aContinuation);



GMPErr
WriteRecord(const std::string& aRecordName,
            const std::string& aData,
            GMPTask* aOnSuccess,
            GMPTask* aOnFailure);

GMPErr
WriteRecord(const std::string& aRecordName,
            const uint8_t* aData,
            uint32_t aNumBytes,
            GMPTask* aOnSuccess,
            GMPTask* aOnFailure);

GMPErr
GMPOpenRecord(const char* aName,
              uint32_t aNameLength,
              GMPRecord** aOutRecord,
              GMPRecordClient* aClient);

GMPErr
GMPRunOnMainThread(GMPTask* aTask);

class OpenContinuation {
public:
  virtual ~OpenContinuation() {}
  virtual void OpenComplete(GMPErr aStatus, GMPRecord* aRecord) = 0;
};

void
GMPOpenRecord(const std::string& aRecordName,
              OpenContinuation* aContinuation);

GMPErr
GMPEnumRecordNames(RecvGMPRecordIteratorPtr aRecvIteratorFunc,
                   void* aUserArg);

#endif 
