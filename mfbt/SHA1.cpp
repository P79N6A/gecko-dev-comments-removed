



#include <string.h>
#include "mozilla/SHA1.h"
#include "mozilla/Assertions.h"



#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MOZ_IS_LITTLE_ENDIAN
#endif
#else
#define MOZ_IS_LITTLE_ENDIAN
#endif

using namespace mozilla;

static inline uint32_t SHA_ROTL(uint32_t t, uint32_t n)
{
    return ((t << n) | (t >> (32 - n)));
}

#ifdef MOZ_IS_LITTLE_ENDIAN
static inline unsigned SHA_HTONL(unsigned x) {
  const unsigned int mask = 0x00FF00FF;
  x = (x << 16) | (x >> 16);
  return ((x & mask) << 8) | ((x >> 8) & mask);
}
#else
static inline unsigned SHA_HTONL(unsigned x) {
  return x;
}
#endif

static void shaCompress(volatile unsigned *X, const uint32_t * datain);

#define SHA_F1(X,Y,Z) ((((Y)^(Z))&(X))^(Z))
#define SHA_F2(X,Y,Z) ((X)^(Y)^(Z))
#define SHA_F3(X,Y,Z) (((X)&(Y))|((Z)&((X)|(Y))))
#define SHA_F4(X,Y,Z) ((X)^(Y)^(Z))

#define SHA_MIX(n,a,b,c)    XW(n) = SHA_ROTL(XW(a)^XW(b)^XW(c)^XW(n), 1)

SHA1Sum::SHA1Sum() : size(0), mDone(false)
{
  
  H[0] = 0x67452301L;
  H[1] = 0xefcdab89L;
  H[2] = 0x98badcfeL;
  H[3] = 0x10325476L;
  H[4] = 0xc3d2e1f0L;
}
































#define H2X 11 /* X[0] is H[11], and H[0] is X[-11] */
#define W2X  6 /* X[0] is W[6],  and W[0] is X[-6]  */




void SHA1Sum::update(const uint8_t *dataIn, uint32_t len)
{
  MOZ_ASSERT(!mDone);
  register unsigned int lenB;
  register unsigned int togo;

  if (!len)
    return;

  
  lenB = (unsigned int)(size) & 63U;

  size += len;

  


  if (lenB > 0) {
    togo = 64U - lenB;
    if (len < togo)
      togo = len;
    memcpy(u.b + lenB, dataIn, togo);
    len    -= togo;
    dataIn += togo;
    lenB    = (lenB + togo) & 63U;
    if (!lenB) {
      shaCompress(&H[H2X], u.w);
    }
  }
  while (len >= 64U) {
    len    -= 64U;
    shaCompress(&H[H2X], (uint32_t *)dataIn);
    dataIn += 64U;
  }
  if (len) {
    memcpy(u.b, dataIn, len);
  }
}





void SHA1Sum::finish(uint8_t hashout[20])
{
  MOZ_ASSERT(!mDone);
  register uint64_t size2 = size;
  register uint32_t lenB = (uint32_t)size2 & 63;

  static const uint8_t bulk_pad[64] = { 0x80,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  };

  



  update(bulk_pad, (((55+64) - lenB) & 63) + 1);
  MOZ_ASSERT(((uint32_t)size & 63) == 56);
  
  size2 <<= 3;
  u.w[14] = SHA_HTONL((uint32_t)(size2 >> 32));
  u.w[15] = SHA_HTONL((uint32_t)size2);
  shaCompress(&H[H2X], u.w);

  


  u.w[0] = SHA_HTONL(H[0]);
  u.w[1] = SHA_HTONL(H[1]);
  u.w[2] = SHA_HTONL(H[2]);
  u.w[3] = SHA_HTONL(H[3]);
  u.w[4] = SHA_HTONL(H[4]);
  memcpy(hashout, u.w, 20);
  mDone = true;
}
















































static void
shaCompress(volatile unsigned *X, const uint32_t *inbuf)
{
  register unsigned A, B, C, D, E;


#define XH(n) X[n-H2X]
#define XW(n) X[n-W2X]

#define K0 0x5a827999L
#define K1 0x6ed9eba1L
#define K2 0x8f1bbcdcL
#define K3 0xca62c1d6L

#define SHA_RND1(a,b,c,d,e,n) \
  a = SHA_ROTL(b,5)+SHA_F1(c,d,e)+a+XW(n)+K0; c=SHA_ROTL(c,30) 
#define SHA_RND2(a,b,c,d,e,n) \
  a = SHA_ROTL(b,5)+SHA_F2(c,d,e)+a+XW(n)+K1; c=SHA_ROTL(c,30) 
#define SHA_RND3(a,b,c,d,e,n) \
  a = SHA_ROTL(b,5)+SHA_F3(c,d,e)+a+XW(n)+K2; c=SHA_ROTL(c,30) 
#define SHA_RND4(a,b,c,d,e,n) \
  a = SHA_ROTL(b,5)+SHA_F4(c,d,e)+a+XW(n)+K3; c=SHA_ROTL(c,30) 

#define LOAD(n) XW(n) = SHA_HTONL(inbuf[n])

  A = XH(0);
  B = XH(1);
  C = XH(2);
  D = XH(3);
  E = XH(4);

  LOAD(0);		   SHA_RND1(E,A,B,C,D, 0);
  LOAD(1);		   SHA_RND1(D,E,A,B,C, 1);
  LOAD(2);		   SHA_RND1(C,D,E,A,B, 2);
  LOAD(3);		   SHA_RND1(B,C,D,E,A, 3);
  LOAD(4);		   SHA_RND1(A,B,C,D,E, 4);
  LOAD(5);		   SHA_RND1(E,A,B,C,D, 5);
  LOAD(6);		   SHA_RND1(D,E,A,B,C, 6);
  LOAD(7);		   SHA_RND1(C,D,E,A,B, 7);
  LOAD(8);		   SHA_RND1(B,C,D,E,A, 8);
  LOAD(9);		   SHA_RND1(A,B,C,D,E, 9);
  LOAD(10);		   SHA_RND1(E,A,B,C,D,10);
  LOAD(11);		   SHA_RND1(D,E,A,B,C,11);
  LOAD(12);		   SHA_RND1(C,D,E,A,B,12);
  LOAD(13);		   SHA_RND1(B,C,D,E,A,13);
  LOAD(14);		   SHA_RND1(A,B,C,D,E,14);
  LOAD(15);		   SHA_RND1(E,A,B,C,D,15);

  SHA_MIX( 0, 13,  8,  2); SHA_RND1(D,E,A,B,C, 0);
  SHA_MIX( 1, 14,  9,  3); SHA_RND1(C,D,E,A,B, 1);
  SHA_MIX( 2, 15, 10,  4); SHA_RND1(B,C,D,E,A, 2);
  SHA_MIX( 3,  0, 11,  5); SHA_RND1(A,B,C,D,E, 3);

  SHA_MIX( 4,  1, 12,  6); SHA_RND2(E,A,B,C,D, 4);
  SHA_MIX( 5,  2, 13,  7); SHA_RND2(D,E,A,B,C, 5);
  SHA_MIX( 6,  3, 14,  8); SHA_RND2(C,D,E,A,B, 6);
  SHA_MIX( 7,  4, 15,  9); SHA_RND2(B,C,D,E,A, 7);
  SHA_MIX( 8,  5,  0, 10); SHA_RND2(A,B,C,D,E, 8);
  SHA_MIX( 9,  6,  1, 11); SHA_RND2(E,A,B,C,D, 9);
  SHA_MIX(10,  7,  2, 12); SHA_RND2(D,E,A,B,C,10);
  SHA_MIX(11,  8,  3, 13); SHA_RND2(C,D,E,A,B,11);
  SHA_MIX(12,  9,  4, 14); SHA_RND2(B,C,D,E,A,12);
  SHA_MIX(13, 10,  5, 15); SHA_RND2(A,B,C,D,E,13);
  SHA_MIX(14, 11,  6,  0); SHA_RND2(E,A,B,C,D,14);
  SHA_MIX(15, 12,  7,  1); SHA_RND2(D,E,A,B,C,15);

  SHA_MIX( 0, 13,  8,  2); SHA_RND2(C,D,E,A,B, 0);
  SHA_MIX( 1, 14,  9,  3); SHA_RND2(B,C,D,E,A, 1);
  SHA_MIX( 2, 15, 10,  4); SHA_RND2(A,B,C,D,E, 2);
  SHA_MIX( 3,  0, 11,  5); SHA_RND2(E,A,B,C,D, 3);
  SHA_MIX( 4,  1, 12,  6); SHA_RND2(D,E,A,B,C, 4);
  SHA_MIX( 5,  2, 13,  7); SHA_RND2(C,D,E,A,B, 5);
  SHA_MIX( 6,  3, 14,  8); SHA_RND2(B,C,D,E,A, 6);
  SHA_MIX( 7,  4, 15,  9); SHA_RND2(A,B,C,D,E, 7);

  SHA_MIX( 8,  5,  0, 10); SHA_RND3(E,A,B,C,D, 8);
  SHA_MIX( 9,  6,  1, 11); SHA_RND3(D,E,A,B,C, 9);
  SHA_MIX(10,  7,  2, 12); SHA_RND3(C,D,E,A,B,10);
  SHA_MIX(11,  8,  3, 13); SHA_RND3(B,C,D,E,A,11);
  SHA_MIX(12,  9,  4, 14); SHA_RND3(A,B,C,D,E,12);
  SHA_MIX(13, 10,  5, 15); SHA_RND3(E,A,B,C,D,13);
  SHA_MIX(14, 11,  6,  0); SHA_RND3(D,E,A,B,C,14);
  SHA_MIX(15, 12,  7,  1); SHA_RND3(C,D,E,A,B,15);

  SHA_MIX( 0, 13,  8,  2); SHA_RND3(B,C,D,E,A, 0);
  SHA_MIX( 1, 14,  9,  3); SHA_RND3(A,B,C,D,E, 1);
  SHA_MIX( 2, 15, 10,  4); SHA_RND3(E,A,B,C,D, 2);
  SHA_MIX( 3,  0, 11,  5); SHA_RND3(D,E,A,B,C, 3);
  SHA_MIX( 4,  1, 12,  6); SHA_RND3(C,D,E,A,B, 4);
  SHA_MIX( 5,  2, 13,  7); SHA_RND3(B,C,D,E,A, 5);
  SHA_MIX( 6,  3, 14,  8); SHA_RND3(A,B,C,D,E, 6);
  SHA_MIX( 7,  4, 15,  9); SHA_RND3(E,A,B,C,D, 7);
  SHA_MIX( 8,  5,  0, 10); SHA_RND3(D,E,A,B,C, 8);
  SHA_MIX( 9,  6,  1, 11); SHA_RND3(C,D,E,A,B, 9);
  SHA_MIX(10,  7,  2, 12); SHA_RND3(B,C,D,E,A,10);
  SHA_MIX(11,  8,  3, 13); SHA_RND3(A,B,C,D,E,11);

  SHA_MIX(12,  9,  4, 14); SHA_RND4(E,A,B,C,D,12);
  SHA_MIX(13, 10,  5, 15); SHA_RND4(D,E,A,B,C,13);
  SHA_MIX(14, 11,  6,  0); SHA_RND4(C,D,E,A,B,14);
  SHA_MIX(15, 12,  7,  1); SHA_RND4(B,C,D,E,A,15);

  SHA_MIX( 0, 13,  8,  2); SHA_RND4(A,B,C,D,E, 0);
  SHA_MIX( 1, 14,  9,  3); SHA_RND4(E,A,B,C,D, 1);
  SHA_MIX( 2, 15, 10,  4); SHA_RND4(D,E,A,B,C, 2);
  SHA_MIX( 3,  0, 11,  5); SHA_RND4(C,D,E,A,B, 3);
  SHA_MIX( 4,  1, 12,  6); SHA_RND4(B,C,D,E,A, 4);
  SHA_MIX( 5,  2, 13,  7); SHA_RND4(A,B,C,D,E, 5);
  SHA_MIX( 6,  3, 14,  8); SHA_RND4(E,A,B,C,D, 6);
  SHA_MIX( 7,  4, 15,  9); SHA_RND4(D,E,A,B,C, 7);
  SHA_MIX( 8,  5,  0, 10); SHA_RND4(C,D,E,A,B, 8);
  SHA_MIX( 9,  6,  1, 11); SHA_RND4(B,C,D,E,A, 9);
  SHA_MIX(10,  7,  2, 12); SHA_RND4(A,B,C,D,E,10);
  SHA_MIX(11,  8,  3, 13); SHA_RND4(E,A,B,C,D,11);
  SHA_MIX(12,  9,  4, 14); SHA_RND4(D,E,A,B,C,12);
  SHA_MIX(13, 10,  5, 15); SHA_RND4(C,D,E,A,B,13);
  SHA_MIX(14, 11,  6,  0); SHA_RND4(B,C,D,E,A,14);
  SHA_MIX(15, 12,  7,  1); SHA_RND4(A,B,C,D,E,15);

  XH(0) += A;
  XH(1) += B;
  XH(2) += C;
  XH(3) += D;
  XH(4) += E;
}
