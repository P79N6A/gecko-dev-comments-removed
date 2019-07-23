








































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerr.h"
#include "secerr.h"

#include "prtypes.h"
#include "blapi.h"
#include "secitem.h"
#include "mpi.h"
#include "mpprime.h"
#include "mplogic.h"
#include "secmpi.h"

#define MAX_ITERATIONS 1000  /* Maximum number of iterations of primegen */
#define PQG_Q_PRIMALITY_TESTS 18 /* from HAC table 4.4 */
#define PQG_P_PRIMALITY_TESTS 5  /* from HAC table 4.4 */

 
#define BITS_IN_Q 160






#ifdef FIPS_186_1_A5_TEST
static const unsigned char fips_186_1_a5_pqseed[] = {
    0xd5, 0x01, 0x4e, 0x4b, 0x60, 0xef, 0x2b, 0xa8,
    0xb6, 0x21, 0x1b, 0x40, 0x62, 0xba, 0x32, 0x24,
    0xe0, 0x42, 0x7d, 0xd3
};
#endif





static SECStatus
getPQseed(SECItem *seed, PRArenaPool* arena)
{
    SECStatus rv;

    if (!seed->data) {
        seed->data = (unsigned char*)PORT_ArenaZAlloc(arena, seed->len);
    }
    if (!seed->data) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
#ifdef FIPS_186_1_A5_TEST
    memcpy(seed->data, fips_186_1_a5_pqseed, seed->len);
    return SECSuccess;
#else
    rv = RNG_GenerateGlobalRandomBytes(seed->data, seed->len);
    





    seed->data[0] |= 0x80;
    return rv;
#endif
}





static SECStatus
generate_h_candidate(SECItem *hit, mp_int *H)
{
    SECStatus rv = SECSuccess;
    mp_err   err = MP_OKAY;
#ifdef FIPS_186_1_A5_TEST
    memset(hit->data, 0, hit->len);
    hit->data[hit->len-1] = 0x02;
#else
    rv = RNG_GenerateGlobalRandomBytes(hit->data, hit->len);
#endif
    if (rv)
	return SECFailure;
    err = mp_read_unsigned_octets(H, hit->data, hit->len);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return SECSuccess;
}





static SECStatus
addToSeedThenSHA(const SECItem * seed,
                 unsigned long   addend,
                 int             g,
                 unsigned char * shaOutBuf)
{
    SECItem str = { 0, 0, 0 };
    mp_int s, sum, modulus, tmp;
    mp_err    err = MP_OKAY;
    SECStatus rv  = SECSuccess;
    MP_DIGITS(&s)       = 0;
    MP_DIGITS(&sum)     = 0;
    MP_DIGITS(&modulus) = 0;
    MP_DIGITS(&tmp)     = 0;
    CHECK_MPI_OK( mp_init(&s) );
    CHECK_MPI_OK( mp_init(&sum) );
    CHECK_MPI_OK( mp_init(&modulus) );
    SECITEM_TO_MPINT(*seed, &s); 
    
    if (addend < MP_DIGIT_MAX) {
	CHECK_MPI_OK( mp_add_d(&s, (mp_digit)addend, &s) );
    } else {
	CHECK_MPI_OK( mp_init(&tmp) );
	CHECK_MPI_OK( mp_set_ulong(&tmp, addend) );
	CHECK_MPI_OK( mp_add(&s, &tmp, &s) );
    }
    CHECK_MPI_OK( mp_div_2d(&s, (mp_digit)g, NULL, &sum) );
    MPINT_TO_SECITEM(&sum, &str, NULL);
    rv = SHA1_HashBuf(shaOutBuf, str.data, str.len); 
cleanup:
    mp_clear(&s);
    mp_clear(&sum);
    mp_clear(&modulus);
    mp_clear(&tmp);
    if (str.data)
	SECITEM_ZfreeItem(&str, PR_FALSE);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return rv;
}





static SECStatus
makeQfromSeed(
      unsigned int  g,          
const SECItem   *   seed,       
      mp_int    *   Q)          
{
    unsigned char sha1[SHA1_LENGTH];
    unsigned char sha2[SHA1_LENGTH];
    unsigned char U[SHA1_LENGTH];
    SECStatus rv  = SECSuccess;
    mp_err    err = MP_OKAY;
    int i;
    



    CHECK_SEC_OK( SHA1_HashBuf(sha1, seed->data, seed->len) );
    CHECK_SEC_OK( addToSeedThenSHA(seed, 1, g, sha2) );
    for (i=0; i<SHA1_LENGTH; ++i) 
	U[i] = sha1[i] ^ sha2[i];
    





    U[0]             |= 0x80;  
    U[SHA1_LENGTH-1] |= 0x01;
    err = mp_read_unsigned_octets(Q, U, SHA1_LENGTH);
cleanup:
     memset(U, 0, SHA1_LENGTH);
     memset(sha1, 0, SHA1_LENGTH);
     memset(sha2, 0, SHA1_LENGTH);
     if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
     }
     return rv;
}




static SECStatus
makePfromQandSeed(
      unsigned int  L,          
      unsigned int  offset,     
      unsigned int  g,          
const SECItem   *   seed,       
const mp_int    *   Q,          
      mp_int    *   P)          
{
    unsigned int  k;            
    unsigned int  n;            
    mp_digit      b;            
    unsigned char V_k[SHA1_LENGTH];
    mp_int        W, X, c, twoQ, V_n, tmp;
    mp_err    err = MP_OKAY;
    SECStatus rv  = SECSuccess;
    
    MP_DIGITS(&W)     = 0;
    MP_DIGITS(&X)     = 0;
    MP_DIGITS(&c)     = 0;
    MP_DIGITS(&twoQ)  = 0;
    MP_DIGITS(&V_n)   = 0;
    MP_DIGITS(&tmp)   = 0;
    CHECK_MPI_OK( mp_init(&W)    );
    CHECK_MPI_OK( mp_init(&X)    );
    CHECK_MPI_OK( mp_init(&c)    );
    CHECK_MPI_OK( mp_init(&twoQ) );
    CHECK_MPI_OK( mp_init(&tmp)  );
    CHECK_MPI_OK( mp_init(&V_n)  );
    
    n = (L - 1) / BITS_IN_Q;
    b = (L - 1) % BITS_IN_Q;
    









    for (k=0; k<n; ++k) { 
	


	CHECK_SEC_OK( addToSeedThenSHA(seed, offset + k, g, V_k) );
	


	OCTETS_TO_MPINT(V_k, &tmp, SHA1_LENGTH);      
	CHECK_MPI_OK( mpl_lsh(&tmp, &tmp, k*160) );   
	CHECK_MPI_OK( mp_add(&W, &tmp, &W) );         
    }
    


    CHECK_SEC_OK( addToSeedThenSHA(seed, offset + n, g, V_k) );
    OCTETS_TO_MPINT(V_k, &V_n, SHA1_LENGTH);          
    CHECK_MPI_OK( mp_div_2d(&V_n, b, NULL, &tmp) );   
    CHECK_MPI_OK( mpl_lsh(&tmp, &tmp, n*160) );       
    CHECK_MPI_OK( mp_add(&W, &tmp, &W) );             
    



    CHECK_MPI_OK( mpl_set_bit(&X, (mp_size)(L-1), 1) );    
    CHECK_MPI_OK( mp_add(&X, &W, &X) );                    
    




    CHECK_MPI_OK( mp_mul_2(Q, &twoQ) );                    
    CHECK_MPI_OK( mp_mod(&X, &twoQ, &c) );                 
    CHECK_MPI_OK( mp_sub_d(&c, 1, &c) );                   
    CHECK_MPI_OK( mp_sub(&X, &c, P) );                     
cleanup:
    mp_clear(&W);
    mp_clear(&X);
    mp_clear(&c);
    mp_clear(&twoQ);
    mp_clear(&V_n);
    mp_clear(&tmp);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return rv;
}




static SECStatus
makeGfromH(const mp_int *P,     
           const mp_int *Q,     
                 mp_int *H,     
                 mp_int *G,     
                 PRBool *passed)
{
    mp_int exp, pm1;
    mp_err err = MP_OKAY;
    SECStatus rv = SECSuccess;
    *passed = PR_FALSE;
    MP_DIGITS(&exp) = 0;
    MP_DIGITS(&pm1) = 0;
    CHECK_MPI_OK( mp_init(&exp) );
    CHECK_MPI_OK( mp_init(&pm1) );
    CHECK_MPI_OK( mp_sub_d(P, 1, &pm1) );        
    if ( mp_cmp(H, &pm1) >= 0)                   
	CHECK_MPI_OK( mp_sub(H, &pm1, H) );      
    



    
    if (mp_cmp_d(H, 1) <= 0) {
	rv = SECFailure;
	goto cleanup;
    }
    
    CHECK_MPI_OK( mp_div(&pm1, Q, &exp, NULL) );  
    CHECK_MPI_OK( mp_exptmod(H, &exp, P, G) );    
    
    if (mp_cmp_d(G, 1) <= 0) {
	rv = SECFailure;
	goto cleanup;
    }
    *passed = PR_TRUE;
cleanup:
    mp_clear(&exp);
    mp_clear(&pm1);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}

SECStatus
PQG_ParamGen(unsigned int j, PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int L;            
    unsigned int seedBytes;

    if (j > 8 || !pParams || !pVfy) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    L = 512 + (j * 64);         
    seedBytes = L/8;
    return PQG_ParamGenSeedLen(j, seedBytes, pParams, pVfy);
}






SECStatus
PQG_ParamGenSeedLen(unsigned int j, unsigned int seedBytes,
                    PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int  L;        
    unsigned int  n;        
    unsigned int  b;        
    unsigned int  g;        
    unsigned int  counter;  
    unsigned int  offset;   
    SECItem      *seed;     
    PRArenaPool  *arena  = NULL;
    PQGParams    *params = NULL;
    PQGVerify    *verify = NULL;
    PRBool passed;
    SECItem hit = { 0, 0, 0 };
    mp_int P, Q, G, H, l;
    mp_err    err = MP_OKAY;
    SECStatus rv  = SECFailure;
    int iterations = 0;
    if (j > 8 || seedBytes < 20 || !pParams || !pVfy) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    params = (PQGParams *)PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (!params) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }
    params->arena = arena;
    
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(params->arena, PR_TRUE);
	return SECFailure;
    }
    verify = (PQGVerify *)PORT_ArenaZAlloc(arena, sizeof(PQGVerify));
    if (!verify) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	PORT_FreeArena(params->arena, PR_TRUE);
	return SECFailure;
    }
    verify->arena = arena;
    seed = &verify->seed;
    arena = NULL;
    
    MP_DIGITS(&P) = 0;
    MP_DIGITS(&Q) = 0;
    MP_DIGITS(&G) = 0;
    MP_DIGITS(&H) = 0;
    MP_DIGITS(&l) = 0;
    CHECK_MPI_OK( mp_init(&P) );
    CHECK_MPI_OK( mp_init(&Q) );
    CHECK_MPI_OK( mp_init(&G) );
    CHECK_MPI_OK( mp_init(&H) );
    CHECK_MPI_OK( mp_init(&l) );
    
    L = 512 + (j * 64);               
    n = (L - 1) / BITS_IN_Q;            
    b = (L - 1) % BITS_IN_Q;
    g = seedBytes * BITS_PER_BYTE;    
step_1:
    




    if (++iterations > MAX_ITERATIONS) {        
        PORT_SetError(SEC_ERROR_NEED_RANDOM);
        goto cleanup;
    }
    seed->len = seedBytes;
    CHECK_SEC_OK( getPQseed(seed, verify->arena) );
    








    CHECK_SEC_OK( makeQfromSeed(g, seed, &Q) );
    






    
    err = mpp_pprime(&Q, PQG_Q_PRIMALITY_TESTS);
    passed = (err == MP_YES) ? SECSuccess : SECFailure;
    


    if (passed != SECSuccess)
        goto step_1;
    


    counter = 0;
    offset  = 2;
step_7:
    













    CHECK_SEC_OK( makePfromQandSeed(L, offset, g, seed, &Q, &P) );
    



    CHECK_MPI_OK( mpl_set_bit(&l, (mp_size)(L-1), 1) ); 
    if (mp_cmp(&P, &l) < 0)
        goto step_13;
    



    
    err = mpp_pprime(&P, PQG_P_PRIMALITY_TESTS);
    passed = (err == MP_YES) ? SECSuccess : SECFailure;
    


    if (passed == SECSuccess)
        goto step_15;
step_13:
    


    counter++;
    offset += n + 1;
    


    if (counter >= 4096)
        goto step_1;
    goto step_7;
step_15:
    




    
    SECITEM_AllocItem(NULL, &hit, L/8); 
    if (!hit.data) goto cleanup;
    do {
	
	CHECK_SEC_OK( generate_h_candidate(&hit, &H) );
        CHECK_SEC_OK( makeGfromH(&P, &Q, &H, &G, &passed) );
    } while (passed != PR_TRUE);
    
    MPINT_TO_SECITEM(&P, &params->prime,    params->arena);
    MPINT_TO_SECITEM(&Q, &params->subPrime, params->arena);
    MPINT_TO_SECITEM(&G, &params->base,     params->arena);
    MPINT_TO_SECITEM(&H, &verify->h,        verify->arena);
    verify->counter = counter;
    *pParams = params;
    *pVfy = verify;
cleanup:
    mp_clear(&P);
    mp_clear(&Q);
    mp_clear(&G);
    mp_clear(&H);
    mp_clear(&l);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv) {
	PORT_FreeArena(params->arena, PR_TRUE);
	PORT_FreeArena(verify->arena, PR_TRUE);
    }
    if (hit.data) {
        SECITEM_FreeItem(&hit, PR_FALSE);
    }
    return rv;
}

SECStatus   
PQG_VerifyParams(const PQGParams *params, 
                 const PQGVerify *vfy, SECStatus *result)
{
    SECStatus rv = SECSuccess;
    int passed;
    unsigned int g, n, L, offset;
    mp_int P, Q, G, P_, Q_, G_, r, h;
    mp_err err = MP_OKAY;
    int j;
#define CHECKPARAM(cond)      \
    if (!(cond)) {            \
	*result = SECFailure; \
	goto cleanup;         \
    }
    if (!params || !vfy || !result) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    MP_DIGITS(&P) = 0;
    MP_DIGITS(&Q) = 0;
    MP_DIGITS(&G) = 0;
    MP_DIGITS(&P_) = 0;
    MP_DIGITS(&Q_) = 0;
    MP_DIGITS(&G_) = 0;
    MP_DIGITS(&r) = 0;
    MP_DIGITS(&h) = 0;
    CHECK_MPI_OK( mp_init(&P) );
    CHECK_MPI_OK( mp_init(&Q) );
    CHECK_MPI_OK( mp_init(&G) );
    CHECK_MPI_OK( mp_init(&P_) );
    CHECK_MPI_OK( mp_init(&Q_) );
    CHECK_MPI_OK( mp_init(&G_) );
    CHECK_MPI_OK( mp_init(&r) );
    CHECK_MPI_OK( mp_init(&h) );
    *result = SECSuccess;
    SECITEM_TO_MPINT(params->prime,    &P);
    SECITEM_TO_MPINT(params->subPrime, &Q);
    SECITEM_TO_MPINT(params->base,     &G);
    
    CHECKPARAM( mpl_significant_bits(&Q) == 160 );
    
    L = mpl_significant_bits(&P);
    j = PQG_PBITS_TO_INDEX(L);
    CHECKPARAM( j >= 0 && j <= 8 );
    
    CHECKPARAM( mp_cmp(&G, &P) < 0 );
    
    CHECK_MPI_OK( mp_mod(&P, &Q, &r) );
    CHECKPARAM( mp_cmp_d(&r, 1) == 0 );
    
    CHECKPARAM( mpp_pprime(&Q, PQG_Q_PRIMALITY_TESTS) == MP_YES );
    
    CHECKPARAM( mpp_pprime(&P, PQG_P_PRIMALITY_TESTS) == MP_YES );
    
    
    CHECKPARAM( vfy->counter < 4096 );
    
    g = vfy->seed.len * 8;
    CHECKPARAM( g >= 160 && g < 2048 );
    
    CHECK_SEC_OK( makeQfromSeed(g, &vfy->seed, &Q_) );
    CHECKPARAM( mp_cmp(&Q, &Q_) == 0 );
    
    n = (L - 1) / BITS_IN_Q;
    offset = vfy->counter * (n + 1) + 2;
    CHECK_SEC_OK( makePfromQandSeed(L, offset, g, &vfy->seed, &Q, &P_) );
    CHECKPARAM( mp_cmp(&P, &P_) == 0 );
    
    if (vfy->h.len == 0) goto cleanup;
    
    SECITEM_TO_MPINT(vfy->h, &h);
    CHECK_MPI_OK( mpl_set_bit(&P, 0, 0) ); 
    CHECKPARAM( mp_cmp_d(&h, 1) > 0 && mp_cmp(&h, &P) );
    CHECK_MPI_OK( mpl_set_bit(&P, 0, 1) ); 
    
    CHECK_SEC_OK( makeGfromH(&P, &Q, &h, &G_, &passed) );
    CHECKPARAM( passed && mp_cmp(&G, &G_) == 0 );
cleanup:
    mp_clear(&P);
    mp_clear(&Q);
    mp_clear(&G);
    mp_clear(&P_);
    mp_clear(&Q_);
    mp_clear(&G_);
    mp_clear(&r);
    mp_clear(&h);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}




void
PQG_DestroyParams(PQGParams *params)
{
    if (params == NULL) 
    	return;
    if (params->arena != NULL) {
	PORT_FreeArena(params->arena, PR_FALSE);	
    } else {
	SECITEM_FreeItem(&params->prime,    PR_FALSE); 
	SECITEM_FreeItem(&params->subPrime, PR_FALSE); 
	SECITEM_FreeItem(&params->base,     PR_FALSE); 
	PORT_Free(params);
    }
}





void
PQG_DestroyVerify(PQGVerify *vfy)
{
    if (vfy == NULL) 
    	return;
    if (vfy->arena != NULL) {
	PORT_FreeArena(vfy->arena, PR_FALSE);	
    } else {
	SECITEM_FreeItem(&vfy->seed,   PR_FALSE); 
	SECITEM_FreeItem(&vfy->h,      PR_FALSE); 
	PORT_Free(vfy);
    }
}
