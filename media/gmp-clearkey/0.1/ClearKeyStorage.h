















#ifndef __ClearKeyStorage_h__
#define __ClearKeyStorage_h__

#include "gmp-errors.h"
#include "gmp-platform.h"
#include <string>
#include <vector>
#include <stdint.h>

class GMPTask;


void StoreData(const std::string& aRecordName,
               const std::vector<uint8_t>& aData,
               GMPTask* aOnSuccess,
               GMPTask* aOnFailure);

class ReadContinuation {
public:
  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aLength) = 0;
  virtual ~ReadContinuation() {}
};


void ReadData(const std::string& aSessionId,
              ReadContinuation* aContinuation);

GMPErr EnumRecordNames(RecvGMPRecordIteratorPtr aRecvIteratorFunc);

#endif 
