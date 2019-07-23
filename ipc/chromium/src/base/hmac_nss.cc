



#include "base/hmac.h"

#include <nss.h>
#include <pk11pub.h>

#include "base/logging.h"
#include "base/nss_init.h"
#include "base/scoped_ptr.h"

namespace {

template <typename Type, void (*Destroyer)(Type*)>
struct NSSDestroyer {
  void operator()(Type* ptr) const {
    if (ptr)
      Destroyer(ptr);
  }
};

void DestroyContext(PK11Context* context) {
  PK11_DestroyContext(context, PR_TRUE);
}


typedef scoped_ptr_malloc<
    PK11SlotInfo, NSSDestroyer<PK11SlotInfo, PK11_FreeSlot> > ScopedNSSSlot;
typedef scoped_ptr_malloc<
    PK11SymKey, NSSDestroyer<PK11SymKey, PK11_FreeSymKey> > ScopedNSSSymKey;
typedef scoped_ptr_malloc<
    PK11Context, NSSDestroyer<PK11Context, DestroyContext> > ScopedNSSContext;

}  

namespace base {

struct HMACPlatformData {
  ScopedNSSSlot slot_;
  ScopedNSSSymKey sym_key_;
};

HMAC::HMAC(HashAlgorithm hash_alg)
    : hash_alg_(hash_alg), plat_(new HMACPlatformData()) {
  
  DCHECK(hash_alg_ == SHA1);
}

bool HMAC::Init(const unsigned char *key, int key_length) {
  base::EnsureNSSInit();

  if (hash_alg_ != SHA1) {
    NOTREACHED();
    return false;
  }

  if (plat_->slot_.get() || plat_->slot_.get()) {
    
    NOTREACHED();
    return false;
  }

  plat_->slot_.reset(PK11_GetBestSlot(CKM_SHA_1_HMAC, NULL));
  if (!plat_->slot_.get()) {
    NOTREACHED();
    return false;
  }

  SECItem key_item;
  key_item.type = siBuffer;
  key_item.data = const_cast<unsigned char*>(key);  
  key_item.len = key_length;

  plat_->sym_key_.reset(PK11_ImportSymKey(plat_->slot_.get(),
                                          CKM_SHA_1_HMAC,
                                          PK11_OriginUnwrap,
                                          CKA_SIGN,
                                          &key_item,
                                          NULL));
  if (!plat_->sym_key_.get()) {
    NOTREACHED();
    return false;
  }

  return true;
}

HMAC::~HMAC() {
}

bool HMAC::Sign(const std::string& data,
                unsigned char* digest,
                int digest_length) {
  if (!plat_->sym_key_.get()) {
    
    NOTREACHED();
    return false;
  }

  SECItem param = { siBuffer, NULL, 0 };
  ScopedNSSContext context(PK11_CreateContextBySymKey(CKM_SHA_1_HMAC,
                                                      CKA_SIGN,
                                                      plat_->sym_key_.get(),
                                                      &param));
  if (!context.get()) {
    NOTREACHED();
    return false;
  }

  if (PK11_DigestBegin(context.get()) != SECSuccess) {
    NOTREACHED();
    return false;
  }

  if (PK11_DigestOp(context.get(),
                    reinterpret_cast<const unsigned char*>(data.data()),
                    data.length()) != SECSuccess) {
    NOTREACHED();
    return false;
  }

  unsigned int len = 0;
  if (PK11_DigestFinal(context.get(),
                       digest, &len, digest_length) != SECSuccess) {
    NOTREACHED();
    return false;
  }

  return true;
}

}  
