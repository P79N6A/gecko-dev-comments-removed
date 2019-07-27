





#include "RootCertificateTelemetryUtils.h"

#include "prlog.h"
#include "RootHashes.inc" 
#include "ScopedNSSTypes.h"
#include "mozilla/ArrayUtils.h"




#define UNKNOWN_ROOT  0
#define HASH_FAILURE -1

namespace mozilla { namespace psm { 

#if defined(PR_LOGGING)
PRLogModuleInfo* gPublicKeyPinningTelemetryLog =
  PR_NewLogModule("PublicKeyPinningTelemetryService");
#endif






class BinaryHashSearchArrayComparator
{
public:
  explicit BinaryHashSearchArrayComparator(const uint8_t* aTarget, size_t len)
    : mTarget(aTarget)
  {
    NS_ASSERTION(len == HASH_LEN, "Hashes should be of the same length.");
  }

  int operator()(const CertAuthorityHash val) const {
    return memcmp(mTarget, val.hash, HASH_LEN);
  }

private:
  const uint8_t* mTarget;
};



int32_t
RootCABinNumber(const SECItem* cert)
{
  Digest digest;

  
  nsresult rv = digest.DigestBuf(SEC_OID_SHA256, cert->data, cert->len);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return HASH_FAILURE;
  }

  
  size_t idx;

  PR_LOG(gPublicKeyPinningTelemetryLog, PR_LOG_DEBUG,
           ("pkpinTelem: First bytes %02hx %02hx %02hx %02hx\n",
            digest.get().data[0], digest.get().data[1], digest.get().data[2], digest.get().data[3]));

  if (mozilla::BinarySearchIf(ROOT_TABLE, 0, ArrayLength(ROOT_TABLE),
          BinaryHashSearchArrayComparator(
            reinterpret_cast<const uint8_t*>(digest.get().data), digest.get().len),
         &idx)) {

    PR_LOG(gPublicKeyPinningTelemetryLog, PR_LOG_DEBUG,
          ("pkpinTelem: Telemetry index was %lu, bin is %d\n",
           idx, ROOT_TABLE[idx].binNumber));
    return (int32_t) ROOT_TABLE[idx].binNumber;
  }

  
  return UNKNOWN_ROOT;
}




void
AccumulateTelemetryForRootCA(mozilla::Telemetry::ID probe, 
  const CERTCertificate* cert)
{
  int32_t binId = RootCABinNumber(&cert->derCert);

  if (binId != HASH_FAILURE) {
    Accumulate(probe, binId);
  }
}

} 
} 