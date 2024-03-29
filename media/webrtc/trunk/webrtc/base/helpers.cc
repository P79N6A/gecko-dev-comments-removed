









#include "webrtc/base/helpers.h"

#include <limits>

#if defined(FEATURE_ENABLE_SSL)
#include "webrtc/base/sslconfig.h"
#if defined(SSL_USE_OPENSSL)
#include <openssl/rand.h>
#elif defined(SSL_USE_NSS_RNG)
#include "pk11func.h"
#else
#if defined(WEBRTC_WIN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ntsecapi.h>
#endif  
#endif  
#endif  

#include "webrtc/base/base64.h"
#include "webrtc/base/basictypes.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/timeutils.h"


#undef max

namespace rtc {


class RandomGenerator {
 public:
  virtual ~RandomGenerator() {}
  virtual bool Init(const void* seed, size_t len) = 0;
  virtual bool Generate(void* buf, size_t len) = 0;
};

#if defined(SSL_USE_OPENSSL)

class SecureRandomGenerator : public RandomGenerator {
 public:
  SecureRandomGenerator() {}
  ~SecureRandomGenerator() {}
  virtual bool Init(const void* seed, size_t len) {
    return true;
  }
  virtual bool Generate(void* buf, size_t len) {
    return (RAND_bytes(reinterpret_cast<unsigned char*>(buf), len) > 0);
  }
};

#elif defined(SSL_USE_NSS_RNG)

class SecureRandomGenerator : public RandomGenerator {
 public:
  SecureRandomGenerator() {}
  ~SecureRandomGenerator() {}
  virtual bool Init(const void* seed, size_t len) {
    return true;
  }
  virtual bool Generate(void* buf, size_t len) {
    return (PK11_GenerateRandom(reinterpret_cast<unsigned char*>(buf),
                                static_cast<int>(len)) == SECSuccess);
  }
};

#else
#if defined(WEBRTC_WIN)
class SecureRandomGenerator : public RandomGenerator {
 public:
  SecureRandomGenerator() : advapi32_(NULL), rtl_gen_random_(NULL) {}
  ~SecureRandomGenerator() {
    FreeLibrary(advapi32_);
  }

  virtual bool Init(const void* seed, size_t seed_len) {
    
    
    
    if (rtl_gen_random_) {
      return true;
    }

    advapi32_ = LoadLibrary(L"advapi32.dll");
    if (!advapi32_) {
      return false;
    }

    rtl_gen_random_ = reinterpret_cast<RtlGenRandomProc>(
        GetProcAddress(advapi32_, "SystemFunction036"));
    if (!rtl_gen_random_) {
      FreeLibrary(advapi32_);
      return false;
    }

    return true;
  }
  virtual bool Generate(void* buf, size_t len) {
    if (!rtl_gen_random_ && !Init(NULL, 0)) {
      return false;
    }
    return (rtl_gen_random_(buf, static_cast<int>(len)) != FALSE);
  }

 private:
  typedef BOOL (WINAPI *RtlGenRandomProc)(PVOID, ULONG);
  HINSTANCE advapi32_;
  RtlGenRandomProc rtl_gen_random_;
};

#elif !defined(FEATURE_ENABLE_SSL)


class SecureRandomGenerator : public RandomGenerator {
 public:
  virtual bool Init(const void* seed, size_t len) {
    if (len >= 4) {
      srand(*reinterpret_cast<const int*>(seed));
    } else {
      srand(*reinterpret_cast<const char*>(seed));
    }
    return true;
  }
  virtual bool Generate(void* buf, size_t len) {
    char* bytes = reinterpret_cast<char*>(buf);
    for (size_t i = 0; i < len; ++i) {
      bytes[i] = static_cast<char>(rand());
    }
    return true;
  }
};

#else

#error No SSL implementation has been selected!

#endif  
#endif


class TestRandomGenerator : public RandomGenerator {
 public:
  TestRandomGenerator() : seed_(7) {
  }
  ~TestRandomGenerator() {
  }
  virtual bool Init(const void* seed, size_t len) {
    return true;
  }
  virtual bool Generate(void* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
      static_cast<uint8*>(buf)[i] = static_cast<uint8>(GetRandom());
    }
    return true;
  }

 private:
  int GetRandom() {
    return ((seed_ = seed_ * 214013L + 2531011L) >> 16) & 0x7fff;
  }
  int seed_;
};


static const char BASE64[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
  'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

namespace {



scoped_ptr<RandomGenerator>& GetGlobalRng() {
  LIBJINGLE_DEFINE_STATIC_LOCAL(scoped_ptr<RandomGenerator>, global_rng,
                                (new SecureRandomGenerator()));
  return global_rng;
}

RandomGenerator& Rng() {
  return *GetGlobalRng();
}

}  

void SetRandomTestMode(bool test) {
  if (!test) {
    GetGlobalRng().reset(new SecureRandomGenerator());
  } else {
    GetGlobalRng().reset(new TestRandomGenerator());
  }
}

bool InitRandom(int seed) {
  return InitRandom(reinterpret_cast<const char*>(&seed), sizeof(seed));
}

bool InitRandom(const char* seed, size_t len) {
  if (!Rng().Init(seed, len)) {
    LOG(LS_ERROR) << "Failed to init random generator!";
    return false;
  }
  return true;
}

std::string CreateRandomString(size_t len) {
  std::string str;
  CreateRandomString(len, &str);
  return str;
}

bool CreateRandomString(size_t len,
                        const char* table, int table_size,
                        std::string* str) {
  str->clear();
  scoped_ptr<uint8[]> bytes(new uint8[len]);
  if (!Rng().Generate(bytes.get(), len)) {
    LOG(LS_ERROR) << "Failed to generate random string!";
    return false;
  }
  str->reserve(len);
  for (size_t i = 0; i < len; ++i) {
    str->push_back(table[bytes[i] % table_size]);
  }
  return true;
}

bool CreateRandomString(size_t len, std::string* str) {
  return CreateRandomString(len, BASE64, 64, str);
}

bool CreateRandomString(size_t len, const std::string& table,
                        std::string* str) {
  return CreateRandomString(len, table.c_str(),
                            static_cast<int>(table.size()), str);
}

uint32 CreateRandomId() {
  uint32 id;
  if (!Rng().Generate(&id, sizeof(id))) {
    LOG(LS_ERROR) << "Failed to generate random id!";
  }
  return id;
}

uint64 CreateRandomId64() {
  return static_cast<uint64>(CreateRandomId()) << 32 | CreateRandomId();
}

uint32 CreateRandomNonZeroId() {
  uint32 id;
  do {
    id = CreateRandomId();
  } while (id == 0);
  return id;
}

double CreateRandomDouble() {
  return CreateRandomId() / (std::numeric_limits<uint32>::max() +
      std::numeric_limits<double>::epsilon());
}

}  
