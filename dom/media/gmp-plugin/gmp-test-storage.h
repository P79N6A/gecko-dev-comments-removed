




#ifndef TEST_GMP_STORAGE_H__
#define TEST_GMP_STORAGE_H__

#include "gmp-errors.h"
#include "gmp-platform.h"
#include <string>

class ReadContinuation {
public:
  virtual void ReadComplete(GMPErr aErr, const std::string& aData) = 0;
};



GMPErr
ReadRecord(const std::string& aRecordName,
           ReadContinuation* aContinuation);



GMPErr
WriteRecord(const std::string& aRecordName,
            const std::string& aData,
            GMPTask* aContinuation);

GMPErr
WriteRecord(const std::string& aRecordName,
            const uint8_t* aData,
            uint32_t aNumBytes,
            GMPTask* aContinuation);

GMPErr
GMPOpenRecord(const char* aName,
              uint32_t aNameLength,
              GMPRecord** aOutRecord,
              GMPRecordClient* aClient);

GMPErr
GMPRunOnMainThread(GMPTask* aTask);

class OpenContinuation {
public:
  virtual void OpenComplete(GMPErr aStatus, GMPRecord* aRecord) = 0;
};

GMPErr
GMPOpenRecord(const std::string& aRecordName,
           OpenContinuation* aContinuation);

#endif 
