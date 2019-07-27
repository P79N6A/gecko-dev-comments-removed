































#ifndef GMP_ERRORS_h_
#define GMP_ERRORS_h_

typedef enum {
  GMPNoErr = 0,
  GMPGenericErr = 1,
  GMPClosedErr = 2,
  GMPAllocErr = 3,
  GMPNotImplementedErr = 4,
  GMPNotClosedErr = 5,
  GMPQuotaExceededErr = 6,
  GMPLastErr 
} GMPErr;

#define GMP_SUCCEEDED(x) ((x) == GMPNoErr)
#define GMP_FAILED(x) ((x) != GMPNoErr)

#endif 
