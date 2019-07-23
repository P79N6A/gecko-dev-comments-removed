



#include "chrome/common/visitedlink_common.h"

#include "base/logging.h"
#include "base/md5.h"

const VisitedLinkCommon::Fingerprint VisitedLinkCommon::null_fingerprint_ = 0;
const VisitedLinkCommon::Hash VisitedLinkCommon::null_hash_ = -1;

VisitedLinkCommon::VisitedLinkCommon() :
    hash_table_(NULL),
    table_length_(0) {
}

VisitedLinkCommon::~VisitedLinkCommon() {
}



bool VisitedLinkCommon::IsVisited(const char* canonical_url,
                                  size_t url_len) const {
  if (url_len == 0)
    return false;
  if (!hash_table_ || table_length_ == 0)
    return false;
  return IsVisited(ComputeURLFingerprint(canonical_url, url_len));
}

bool VisitedLinkCommon::IsVisited(Fingerprint fingerprint) const {
  
  
  
  Hash first_hash = HashFingerprint(fingerprint);
  Hash cur_hash = first_hash;
  while (true) {
    Fingerprint cur_fingerprint = FingerprintAt(cur_hash);
    if (cur_fingerprint == null_fingerprint_)
      return false;  
    if (cur_fingerprint == fingerprint)
      return true;  

    
    
    cur_hash++;
    if (cur_hash == table_length_)
      cur_hash = 0;
    if (cur_hash == first_hash) {
      
      
      NOTREACHED();
      return false;
    }
  }
}










VisitedLinkCommon::Fingerprint VisitedLinkCommon::ComputeURLFingerprint(
    const char* canonical_url,
    size_t url_len,
    const uint8 salt[LINK_SALT_LENGTH]) {
  DCHECK(url_len > 0) << "Canonical URLs should not be empty";

  MD5Context ctx;
  MD5Init(&ctx);
  MD5Update(&ctx, salt, sizeof(salt));
  MD5Update(&ctx, canonical_url, url_len * sizeof(char));

  MD5Digest digest;
  MD5Final(&digest, &ctx);

  
  
  
  
  
  return bit_cast<Fingerprint, uint8[8]>(
      *reinterpret_cast<uint8(*)[8]>(&digest.a));
}
