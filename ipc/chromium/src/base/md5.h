



#ifndef BASE_MD5_H__
#define BASE_MD5_H__

#include <string>


















typedef struct MD5Digest_struct {
  unsigned char a[16];
} MD5Digest;



typedef char MD5Context[88];



void MD5Sum(const void* data, size_t length, MD5Digest* digest);



void MD5Init(MD5Context* context);




void MD5Update(MD5Context* context, const void* buf, size_t len);


void MD5Final(MD5Digest* digest, MD5Context* pCtx);


std::string MD5DigestToBase16(const MD5Digest& digest);


std::string MD5String(const std::string& str);

#endif 
