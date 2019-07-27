









#include "webrtc/base/messagedigest.h"

#include <string.h>

#include "webrtc/base/sslconfig.h"
#if SSL_USE_OPENSSL
#include "webrtc/base/openssldigest.h"
#else
#include "webrtc/base/md5digest.h"
#include "webrtc/base/sha1digest.h"
#endif
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/stringencode.h"

namespace rtc {


const char DIGEST_MD5[]     = "md5";
const char DIGEST_SHA_1[]   = "sha-1";
const char DIGEST_SHA_224[] = "sha-224";
const char DIGEST_SHA_256[] = "sha-256";
const char DIGEST_SHA_384[] = "sha-384";
const char DIGEST_SHA_512[] = "sha-512";

static const size_t kBlockSize = 64;  

MessageDigest* MessageDigestFactory::Create(const std::string& alg) {
#if SSL_USE_OPENSSL
  MessageDigest* digest = new OpenSSLDigest(alg);
  if (digest->Size() == 0) {  
    delete digest;
    digest = NULL;
  }
  return digest;
#else
  MessageDigest* digest = NULL;
  if (alg == DIGEST_MD5) {
    digest = new Md5Digest();
  } else if (alg == DIGEST_SHA_1) {
    digest = new Sha1Digest();
  }
  return digest;
#endif
}

bool IsFips180DigestAlgorithm(const std::string& alg) {
  
  
  
  
  
  return alg == DIGEST_SHA_1 ||
         alg == DIGEST_SHA_224 ||
         alg == DIGEST_SHA_256 ||
         alg == DIGEST_SHA_384 ||
         alg == DIGEST_SHA_512;
}

size_t ComputeDigest(MessageDigest* digest, const void* input, size_t in_len,
                     void* output, size_t out_len) {
  digest->Update(input, in_len);
  return digest->Finish(output, out_len);
}

size_t ComputeDigest(const std::string& alg, const void* input, size_t in_len,
                     void* output, size_t out_len) {
  scoped_ptr<MessageDigest> digest(MessageDigestFactory::Create(alg));
  return (digest) ?
      ComputeDigest(digest.get(), input, in_len, output, out_len) :
      0;
}

std::string ComputeDigest(MessageDigest* digest, const std::string& input) {
  scoped_ptr<char[]> output(new char[digest->Size()]);
  ComputeDigest(digest, input.data(), input.size(),
                output.get(), digest->Size());
  return hex_encode(output.get(), digest->Size());
}

bool ComputeDigest(const std::string& alg, const std::string& input,
                   std::string* output) {
  scoped_ptr<MessageDigest> digest(MessageDigestFactory::Create(alg));
  if (!digest) {
    return false;
  }
  *output = ComputeDigest(digest.get(), input);
  return true;
}

std::string ComputeDigest(const std::string& alg, const std::string& input) {
  std::string output;
  ComputeDigest(alg, input, &output);
  return output;
}


size_t ComputeHmac(MessageDigest* digest,
                   const void* key, size_t key_len,
                   const void* input, size_t in_len,
                   void* output, size_t out_len) {
  
  
  size_t block_len = kBlockSize;
  if (digest->Size() > 32) {
    return 0;
  }
  
  
  scoped_ptr<uint8[]> new_key(new uint8[block_len]);
  if (key_len > block_len) {
    ComputeDigest(digest, key, key_len, new_key.get(), block_len);
    memset(new_key.get() + digest->Size(), 0, block_len - digest->Size());
  } else {
    memcpy(new_key.get(), key, key_len);
    memset(new_key.get() + key_len, 0, block_len - key_len);
  }
  
  scoped_ptr<uint8[]> o_pad(new uint8[block_len]), i_pad(new uint8[block_len]);
  for (size_t i = 0; i < block_len; ++i) {
    o_pad[i] = 0x5c ^ new_key[i];
    i_pad[i] = 0x36 ^ new_key[i];
  }
  
  scoped_ptr<uint8[]> inner(new uint8[digest->Size()]);
  digest->Update(i_pad.get(), block_len);
  digest->Update(input, in_len);
  digest->Finish(inner.get(), digest->Size());
  
  digest->Update(o_pad.get(), block_len);
  digest->Update(inner.get(), digest->Size());
  return digest->Finish(output, out_len);
}

size_t ComputeHmac(const std::string& alg, const void* key, size_t key_len,
                   const void* input, size_t in_len,
                   void* output, size_t out_len) {
  scoped_ptr<MessageDigest> digest(MessageDigestFactory::Create(alg));
  if (!digest) {
    return 0;
  }
  return ComputeHmac(digest.get(), key, key_len,
                     input, in_len, output, out_len);
}

std::string ComputeHmac(MessageDigest* digest, const std::string& key,
                        const std::string& input) {
  scoped_ptr<char[]> output(new char[digest->Size()]);
  ComputeHmac(digest, key.data(), key.size(),
              input.data(), input.size(), output.get(), digest->Size());
  return hex_encode(output.get(), digest->Size());
}

bool ComputeHmac(const std::string& alg, const std::string& key,
                 const std::string& input, std::string* output) {
  scoped_ptr<MessageDigest> digest(MessageDigestFactory::Create(alg));
  if (!digest) {
    return false;
  }
  *output = ComputeHmac(digest.get(), key, input);
  return true;
}

std::string ComputeHmac(const std::string& alg, const std::string& key,
                        const std::string& input) {
  std::string output;
  ComputeHmac(alg, key, input, &output);
  return output;
}

}  
