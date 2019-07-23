




































#ifndef _HASHT_H_
#define _HASHT_H_


typedef struct SECHashObjectStr SECHashObject;
typedef struct HASHContextStr HASHContext;





typedef enum {
    HASH_AlgNULL   = 0,
    HASH_AlgMD2    = 1,
    HASH_AlgMD5    = 2,
    HASH_AlgSHA1   = 3,
    HASH_AlgSHA256 = 4,
    HASH_AlgSHA384 = 5,
    HASH_AlgSHA512 = 6,
    HASH_AlgTOTAL
} HASH_HashType;




#define MD2_LENGTH	16
#define MD5_LENGTH	16
#define SHA1_LENGTH	20
#define SHA256_LENGTH 	32
#define SHA384_LENGTH 	48
#define SHA512_LENGTH 	64
#define HASH_LENGTH_MAX SHA512_LENGTH




struct SECHashObjectStr {
    unsigned int length;  
    void * (*create)(void);
    void * (*clone)(void *);
    void (*destroy)(void *, PRBool);
    void (*begin)(void *);
    void (*update)(void *, const unsigned char *, unsigned int);
    void (*end)(void *, unsigned char *, unsigned int *, unsigned int);
    unsigned int blocklength;  
    HASH_HashType type;
};

struct HASHContextStr {
    const struct SECHashObjectStr *hashobj;
    void *hash_context;
};





extern const SECHashObject SECHashObjects[];




extern const SECHashObject SECRawHashObjects[];

#endif 
