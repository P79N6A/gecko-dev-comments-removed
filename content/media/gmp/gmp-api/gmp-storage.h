















#ifndef GMP_STORAGE_h_
#define GMP_STORAGE_h_

#include "gmp-errors.h"
#include <stdint.h>







class GMPRecord {
public:

  
  
  virtual GMPErr Open() = 0;

  
  
  
  virtual GMPErr Read() = 0;

  
  
  
  virtual GMPErr Write(const uint8_t* aData, uint32_t aDataSize) = 0;

  
  
  
  virtual GMPErr Close() = 0;

  virtual ~GMPRecord() {}
};



class GMPRecordClient {
 public:

  
  
  
  
  
  
  virtual void OpenComplete(GMPErr aStatus) = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aDataSize) = 0;

  
  
  
  
  
  
  virtual void WriteComplete(GMPErr aStatus) = 0;

  virtual ~GMPRecordClient() {}
};

#endif 
