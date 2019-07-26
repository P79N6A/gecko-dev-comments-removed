














































#include "sha1.h"

debug_module_t mod_sha1 = {
  0,                 
  "sha-1"            
};


#define S1(X)  ((X << 1)  | (X >> 31))
#define S5(X)  ((X << 5)  | (X >> 27))
#define S30(X) ((X << 30) | (X >> 2))

#define f0(B,C,D) ((B & C) | (~B & D))              
#define f1(B,C,D) (B ^ C ^ D)
#define f2(B,C,D) ((B & C) | (B & D) | (C & D))
#define f3(B,C,D) (B ^ C ^ D)







uint32_t SHA_K0 = 0x5A827999;   
uint32_t SHA_K1 = 0x6ED9EBA1;   
uint32_t SHA_K2 = 0x8F1BBCDC;   
uint32_t SHA_K3 = 0xCA62C1D6;   

void
sha1(const uint8_t *msg,  int octets_in_msg, uint32_t hash_value[5]) {
  sha1_ctx_t ctx;

  sha1_init(&ctx);
  sha1_update(&ctx, msg, octets_in_msg);
  sha1_final(&ctx, hash_value);

}













void
sha1_core(const uint32_t M[16], uint32_t hash_value[5]) {
  uint32_t H0;
  uint32_t H1;
  uint32_t H2;
  uint32_t H3;
  uint32_t H4;
  uint32_t W[80];
  uint32_t A, B, C, D, E, TEMP;
  int t;

  
  H0 = hash_value[0];
  H1 = hash_value[1];
  H2 = hash_value[2];
  H3 = hash_value[3];
  H4 = hash_value[4];

  

  W[0]  = be32_to_cpu(M[0]);
  W[1]  = be32_to_cpu(M[1]);
  W[2]  = be32_to_cpu(M[2]);
  W[3]  = be32_to_cpu(M[3]);
  W[4]  = be32_to_cpu(M[4]);
  W[5]  = be32_to_cpu(M[5]);
  W[6]  = be32_to_cpu(M[6]);
  W[7]  = be32_to_cpu(M[7]);
  W[8]  = be32_to_cpu(M[8]);
  W[9]  = be32_to_cpu(M[9]);
  W[10] = be32_to_cpu(M[10]);
  W[11] = be32_to_cpu(M[11]);
  W[12] = be32_to_cpu(M[12]);
  W[13] = be32_to_cpu(M[13]);
  W[14] = be32_to_cpu(M[14]);
  W[15] = be32_to_cpu(M[15]);
  TEMP = W[13] ^ W[8]  ^ W[2]  ^ W[0];  W[16] = S1(TEMP);
  TEMP = W[14] ^ W[9]  ^ W[3]  ^ W[1];  W[17] = S1(TEMP);
  TEMP = W[15] ^ W[10] ^ W[4]  ^ W[2];  W[18] = S1(TEMP);
  TEMP = W[16] ^ W[11] ^ W[5]  ^ W[3];  W[19] = S1(TEMP);
  TEMP = W[17] ^ W[12] ^ W[6]  ^ W[4];  W[20] = S1(TEMP);
  TEMP = W[18] ^ W[13] ^ W[7]  ^ W[5];  W[21] = S1(TEMP);
  TEMP = W[19] ^ W[14] ^ W[8]  ^ W[6];  W[22] = S1(TEMP);
  TEMP = W[20] ^ W[15] ^ W[9]  ^ W[7];  W[23] = S1(TEMP);
  TEMP = W[21] ^ W[16] ^ W[10] ^ W[8];  W[24] = S1(TEMP);
  TEMP = W[22] ^ W[17] ^ W[11] ^ W[9];  W[25] = S1(TEMP);
  TEMP = W[23] ^ W[18] ^ W[12] ^ W[10]; W[26] = S1(TEMP);
  TEMP = W[24] ^ W[19] ^ W[13] ^ W[11]; W[27] = S1(TEMP);
  TEMP = W[25] ^ W[20] ^ W[14] ^ W[12]; W[28] = S1(TEMP);
  TEMP = W[26] ^ W[21] ^ W[15] ^ W[13]; W[29] = S1(TEMP);
  TEMP = W[27] ^ W[22] ^ W[16] ^ W[14]; W[30] = S1(TEMP);
  TEMP = W[28] ^ W[23] ^ W[17] ^ W[15]; W[31] = S1(TEMP);

  
  for (t=32; t < 80; t++) {
    TEMP = W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16];
    W[t] = S1(TEMP);      
  }

  A = H0; B = H1; C = H2; D = H3; E = H4;

  for (t=0; t < 20; t++) {
    TEMP = S5(A) + f0(B,C,D) + E + W[t] + SHA_K0;
    E = D; D = C; C = S30(B); B = A; A = TEMP;
  }
  for (   ; t < 40; t++) {
    TEMP = S5(A) + f1(B,C,D) + E + W[t] + SHA_K1;
    E = D; D = C; C = S30(B); B = A; A = TEMP;
  }
  for (   ; t < 60; t++) {
    TEMP = S5(A) + f2(B,C,D) + E + W[t] + SHA_K2;
    E = D; D = C; C = S30(B); B = A; A = TEMP;
  }
  for (   ; t < 80; t++) {
    TEMP = S5(A) + f3(B,C,D) + E + W[t] + SHA_K3;
    E = D; D = C; C = S30(B); B = A; A = TEMP;
  }

  hash_value[0] = H0 + A;
  hash_value[1] = H1 + B;
  hash_value[2] = H2 + C;
  hash_value[3] = H3 + D;
  hash_value[4] = H4 + E;

  return;
}

void
sha1_init(sha1_ctx_t *ctx) {

  
  ctx->H[0] = 0x67452301;
  ctx->H[1] = 0xefcdab89;
  ctx->H[2] = 0x98badcfe;
  ctx->H[3] = 0x10325476;
  ctx->H[4] = 0xc3d2e1f0;

  
  ctx->octets_in_buffer = 0;

  
  ctx->num_bits_in_msg = 0;

}

void
sha1_update(sha1_ctx_t *ctx, const uint8_t *msg, int octets_in_msg) {
  int i;
  uint8_t *buf = (uint8_t *)ctx->M;

  
  ctx->num_bits_in_msg += octets_in_msg * 8;

  
  while (octets_in_msg > 0) {

    if (octets_in_msg + ctx->octets_in_buffer >= 64) {

      



      octets_in_msg -= (64 - ctx->octets_in_buffer);
      for (i=ctx->octets_in_buffer; i < 64; i++) 
	buf[i] = *msg++;
      ctx->octets_in_buffer = 0;

      

      debug_print(mod_sha1, "(update) running sha1_core()", NULL);

      sha1_core(ctx->M, ctx->H);

    } else {

      debug_print(mod_sha1, "(update) not running sha1_core()", NULL);

      for (i=ctx->octets_in_buffer; 
	   i < (ctx->octets_in_buffer + octets_in_msg); i++)
	buf[i] = *msg++;
      ctx->octets_in_buffer += octets_in_msg;
      octets_in_msg = 0;
    }

  }

}






void
sha1_final(sha1_ctx_t *ctx, uint32_t *output) {
  uint32_t A, B, C, D, E, TEMP;
  uint32_t W[80];  
  int i, t;

  



  {
    int tail = ctx->octets_in_buffer % 4;

    
    for (i=0; i < (ctx->octets_in_buffer+3)/4; i++) 
      W[i]  = be32_to_cpu(ctx->M[i]);

    
    switch (tail) {
    case (3):
      W[i-1] = (be32_to_cpu(ctx->M[i-1]) & 0xffffff00) | 0x80;
      W[i] = 0x0;
      break;
    case (2):      
      W[i-1] = (be32_to_cpu(ctx->M[i-1]) & 0xffff0000) | 0x8000;
      W[i] = 0x0;
      break;
    case (1):
      W[i-1] = (be32_to_cpu(ctx->M[i-1]) & 0xff000000) | 0x800000;
      W[i] = 0x0;
      break;
    case (0):
      W[i] = 0x80000000;
      break;
    }

    
    for (i++   ; i < 15; i++)
      W[i] = 0x0;

    





    if (ctx->octets_in_buffer < 56) 
      W[15] = ctx->num_bits_in_msg;
    else if (ctx->octets_in_buffer < 60)
      W[15] = 0x0;

    
    for (t=16; t < 80; t++) {
      TEMP = W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16];
      W[t] = S1(TEMP);
    }

    A = ctx->H[0]; 
    B = ctx->H[1]; 
    C = ctx->H[2]; 
    D = ctx->H[3]; 
    E = ctx->H[4];

    for (t=0; t < 20; t++) {
      TEMP = S5(A) + f0(B,C,D) + E + W[t] + SHA_K0;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }
    for (   ; t < 40; t++) {
      TEMP = S5(A) + f1(B,C,D) + E + W[t] + SHA_K1;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }
    for (   ; t < 60; t++) {
      TEMP = S5(A) + f2(B,C,D) + E + W[t] + SHA_K2;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }
    for (   ; t < 80; t++) {
      TEMP = S5(A) + f3(B,C,D) + E + W[t] + SHA_K3;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }

    ctx->H[0] += A;
    ctx->H[1] += B;
    ctx->H[2] += C;
    ctx->H[3] += D;
    ctx->H[4] += E;

  }

  debug_print(mod_sha1, "(final) running sha1_core()", NULL);

  if (ctx->octets_in_buffer >= 56) {

    debug_print(mod_sha1, "(final) running sha1_core() again", NULL);

    

    



    for (i=0; i < 15; i++)
      W[i] = 0x0;
    W[15] = ctx->num_bits_in_msg;

    
    for (t=16; t < 80; t++) {
      TEMP = W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16];
      W[t] = S1(TEMP);
    }

    A = ctx->H[0]; 
    B = ctx->H[1]; 
    C = ctx->H[2]; 
    D = ctx->H[3]; 
    E = ctx->H[4];

    for (t=0; t < 20; t++) {
      TEMP = S5(A) + f0(B,C,D) + E + W[t] + SHA_K0;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }
    for (   ; t < 40; t++) {
      TEMP = S5(A) + f1(B,C,D) + E + W[t] + SHA_K1;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }
    for (   ; t < 60; t++) {
      TEMP = S5(A) + f2(B,C,D) + E + W[t] + SHA_K2;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }
    for (   ; t < 80; t++) {
      TEMP = S5(A) + f3(B,C,D) + E + W[t] + SHA_K3;
      E = D; D = C; C = S30(B); B = A; A = TEMP;
    }

    ctx->H[0] += A;
    ctx->H[1] += B;
    ctx->H[2] += C;
    ctx->H[3] += D;
    ctx->H[4] += E;
  }

  
  output[0] = be32_to_cpu(ctx->H[0]);
  output[1] = be32_to_cpu(ctx->H[1]);
  output[2] = be32_to_cpu(ctx->H[2]);
  output[3] = be32_to_cpu(ctx->H[3]);
  output[4] = be32_to_cpu(ctx->H[4]);

  
  ctx->octets_in_buffer = 0;

  return;
}



