














































#define	SFMT_C_
#include "test/jemalloc_test.h"
#include "test/SFMT-params.h"

#if defined(JEMALLOC_BIG_ENDIAN) && !defined(BIG_ENDIAN64)
#define BIG_ENDIAN64 1
#endif
#if defined(__BIG_ENDIAN__) && !defined(__amd64) && !defined(BIG_ENDIAN64)
#define BIG_ENDIAN64 1
#endif
#if defined(HAVE_ALTIVEC) && !defined(BIG_ENDIAN64)
#define BIG_ENDIAN64 1
#endif
#if defined(ONLY64) && !defined(BIG_ENDIAN64)
  #if defined(__GNUC__)
    #error "-DONLY64 must be specified with -DBIG_ENDIAN64"
  #endif
#undef ONLY64
#endif



#if defined(HAVE_ALTIVEC)

union W128_T {
    vector unsigned int s;
    uint32_t u[4];
};

typedef union W128_T w128_t;

#elif defined(HAVE_SSE2)

union W128_T {
    __m128i si;
    uint32_t u[4];
};

typedef union W128_T w128_t;

#else


struct W128_T {
    uint32_t u[4];
};

typedef struct W128_T w128_t;

#endif

struct sfmt_s {
    
    w128_t sfmt[N];
    
    int idx;
    

    int initialized;
};







static uint32_t parity[4] = {PARITY1, PARITY2, PARITY3, PARITY4};




JEMALLOC_INLINE_C int idxof(int i);
#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))
JEMALLOC_INLINE_C void rshift128(w128_t *out,  w128_t const *in, int shift);
JEMALLOC_INLINE_C void lshift128(w128_t *out,  w128_t const *in, int shift);
#endif
JEMALLOC_INLINE_C void gen_rand_all(sfmt_t *ctx);
JEMALLOC_INLINE_C void gen_rand_array(sfmt_t *ctx, w128_t *array, int size);
JEMALLOC_INLINE_C uint32_t func1(uint32_t x);
JEMALLOC_INLINE_C uint32_t func2(uint32_t x);
static void period_certification(sfmt_t *ctx);
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
JEMALLOC_INLINE_C void swap(w128_t *array, int size);
#endif

#if defined(HAVE_ALTIVEC)
  #include "test/SFMT-alti.h"
#elif defined(HAVE_SSE2)
  #include "test/SFMT-sse2.h"
#endif





#ifdef ONLY64
JEMALLOC_INLINE_C int idxof(int i) {
    return i ^ 1;
}
#else
JEMALLOC_INLINE_C int idxof(int i) {
    return i;
}
#endif








#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))
#ifdef ONLY64
JEMALLOC_INLINE_C void rshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[2] << 32) | ((uint64_t)in->u[3]);
    tl = ((uint64_t)in->u[0] << 32) | ((uint64_t)in->u[1]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u[0] = (uint32_t)(ol >> 32);
    out->u[1] = (uint32_t)ol;
    out->u[2] = (uint32_t)(oh >> 32);
    out->u[3] = (uint32_t)oh;
}
#else
JEMALLOC_INLINE_C void rshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
    tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

    oh = th >> (shift * 8);
    ol = tl >> (shift * 8);
    ol |= th << (64 - shift * 8);
    out->u[1] = (uint32_t)(ol >> 32);
    out->u[0] = (uint32_t)ol;
    out->u[3] = (uint32_t)(oh >> 32);
    out->u[2] = (uint32_t)oh;
}
#endif








#ifdef ONLY64
JEMALLOC_INLINE_C void lshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[2] << 32) | ((uint64_t)in->u[3]);
    tl = ((uint64_t)in->u[0] << 32) | ((uint64_t)in->u[1]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u[0] = (uint32_t)(ol >> 32);
    out->u[1] = (uint32_t)ol;
    out->u[2] = (uint32_t)(oh >> 32);
    out->u[3] = (uint32_t)oh;
}
#else
JEMALLOC_INLINE_C void lshift128(w128_t *out, w128_t const *in, int shift) {
    uint64_t th, tl, oh, ol;

    th = ((uint64_t)in->u[3] << 32) | ((uint64_t)in->u[2]);
    tl = ((uint64_t)in->u[1] << 32) | ((uint64_t)in->u[0]);

    oh = th << (shift * 8);
    ol = tl << (shift * 8);
    oh |= tl >> (64 - shift * 8);
    out->u[1] = (uint32_t)(ol >> 32);
    out->u[0] = (uint32_t)ol;
    out->u[3] = (uint32_t)(oh >> 32);
    out->u[2] = (uint32_t)oh;
}
#endif
#endif









#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))
#ifdef ONLY64
JEMALLOC_INLINE_C void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
				w128_t *d) {
    w128_t x;
    w128_t y;

    lshift128(&x, a, SL2);
    rshift128(&y, c, SR2);
    r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK2) ^ y.u[0] 
	^ (d->u[0] << SL1);
    r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK1) ^ y.u[1] 
	^ (d->u[1] << SL1);
    r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK4) ^ y.u[2] 
	^ (d->u[2] << SL1);
    r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK3) ^ y.u[3] 
	^ (d->u[3] << SL1);
}
#else
JEMALLOC_INLINE_C void do_recursion(w128_t *r, w128_t *a, w128_t *b, w128_t *c,
				w128_t *d) {
    w128_t x;
    w128_t y;

    lshift128(&x, a, SL2);
    rshift128(&y, c, SR2);
    r->u[0] = a->u[0] ^ x.u[0] ^ ((b->u[0] >> SR1) & MSK1) ^ y.u[0] 
	^ (d->u[0] << SL1);
    r->u[1] = a->u[1] ^ x.u[1] ^ ((b->u[1] >> SR1) & MSK2) ^ y.u[1] 
	^ (d->u[1] << SL1);
    r->u[2] = a->u[2] ^ x.u[2] ^ ((b->u[2] >> SR1) & MSK3) ^ y.u[2] 
	^ (d->u[2] << SL1);
    r->u[3] = a->u[3] ^ x.u[3] ^ ((b->u[3] >> SR1) & MSK4) ^ y.u[3] 
	^ (d->u[3] << SL1);
}
#endif
#endif

#if (!defined(HAVE_ALTIVEC)) && (!defined(HAVE_SSE2))




JEMALLOC_INLINE_C void gen_rand_all(sfmt_t *ctx) {
    int i;
    w128_t *r1, *r2;

    r1 = &ctx->sfmt[N - 2];
    r2 = &ctx->sfmt[N - 1];
    for (i = 0; i < N - POS1; i++) {
	do_recursion(&ctx->sfmt[i], &ctx->sfmt[i], &ctx->sfmt[i + POS1], r1,
	  r2);
	r1 = r2;
	r2 = &ctx->sfmt[i];
    }
    for (; i < N; i++) {
	do_recursion(&ctx->sfmt[i], &ctx->sfmt[i], &ctx->sfmt[i + POS1 - N], r1,
	  r2);
	r1 = r2;
	r2 = &ctx->sfmt[i];
    }
}








JEMALLOC_INLINE_C void gen_rand_array(sfmt_t *ctx, w128_t *array, int size) {
    int i, j;
    w128_t *r1, *r2;

    r1 = &ctx->sfmt[N - 2];
    r2 = &ctx->sfmt[N - 1];
    for (i = 0; i < N - POS1; i++) {
	do_recursion(&array[i], &ctx->sfmt[i], &ctx->sfmt[i + POS1], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (; i < N; i++) {
	do_recursion(&array[i], &ctx->sfmt[i], &array[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (; i < size - N; i++) {
	do_recursion(&array[i], &array[i - N], &array[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &array[i];
    }
    for (j = 0; j < 2 * N - size; j++) {
	ctx->sfmt[j] = array[j + size - N];
    }
    for (; i < size; i++, j++) {
	do_recursion(&array[i], &array[i - N], &array[i + POS1 - N], r1, r2);
	r1 = r2;
	r2 = &array[i];
	ctx->sfmt[j] = array[i];
    }
}
#endif

#if defined(BIG_ENDIAN64) && !defined(ONLY64) && !defined(HAVE_ALTIVEC)
JEMALLOC_INLINE_C void swap(w128_t *array, int size) {
    int i;
    uint32_t x, y;

    for (i = 0; i < size; i++) {
	x = array[i].u[0];
	y = array[i].u[2];
	array[i].u[0] = array[i].u[1];
	array[i].u[2] = array[i].u[3];
	array[i].u[1] = x;
	array[i].u[3] = y;
    }
}
#endif






static uint32_t func1(uint32_t x) {
    return (x ^ (x >> 27)) * (uint32_t)1664525UL;
}







static uint32_t func2(uint32_t x) {
    return (x ^ (x >> 27)) * (uint32_t)1566083941UL;
}




static void period_certification(sfmt_t *ctx) {
    int inner = 0;
    int i, j;
    uint32_t work;
    uint32_t *psfmt32 = &ctx->sfmt[0].u[0];

    for (i = 0; i < 4; i++)
	inner ^= psfmt32[idxof(i)] & parity[i];
    for (i = 16; i > 0; i >>= 1)
	inner ^= inner >> i;
    inner &= 1;
    
    if (inner == 1) {
	return;
    }
    
    for (i = 0; i < 4; i++) {
	work = 1;
	for (j = 0; j < 32; j++) {
	    if ((work & parity[i]) != 0) {
		psfmt32[idxof(i)] ^= work;
		return;
	    }
	    work = work << 1;
	}
    }
}









const char *get_idstring(void) {
    return IDSTR;
}






int get_min_array_size32(void) {
    return N32;
}






int get_min_array_size64(void) {
    return N64;
}

#ifndef ONLY64





uint32_t gen_rand32(sfmt_t *ctx) {
    uint32_t r;
    uint32_t *psfmt32 = &ctx->sfmt[0].u[0];

    assert(ctx->initialized);
    if (ctx->idx >= N32) {
	gen_rand_all(ctx);
	ctx->idx = 0;
    }
    r = psfmt32[ctx->idx++];
    return r;
}


uint32_t gen_rand32_range(sfmt_t *ctx, uint32_t limit) {
    uint32_t ret, above;

    above = 0xffffffffU - (0xffffffffU % limit);
    while (1) {
        ret = gen_rand32(ctx);
        if (ret < above) {
            ret %= limit;
            break;
        }
    }
    return ret;
}
#endif







uint64_t gen_rand64(sfmt_t *ctx) {
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    uint32_t r1, r2;
    uint32_t *psfmt32 = &ctx->sfmt[0].u[0];
#else
    uint64_t r;
    uint64_t *psfmt64 = (uint64_t *)&ctx->sfmt[0].u[0];
#endif

    assert(ctx->initialized);
    assert(ctx->idx % 2 == 0);

    if (ctx->idx >= N32) {
	gen_rand_all(ctx);
	ctx->idx = 0;
    }
#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    r1 = psfmt32[ctx->idx];
    r2 = psfmt32[ctx->idx + 1];
    ctx->idx += 2;
    return ((uint64_t)r2 << 32) | r1;
#else
    r = psfmt64[ctx->idx / 2];
    ctx->idx += 2;
    return r;
#endif
}


uint64_t gen_rand64_range(sfmt_t *ctx, uint64_t limit) {
    uint64_t ret, above;

    above = 0xffffffffffffffffLLU - (0xffffffffffffffffLLU  % limit);
    while (1) {
        ret = gen_rand64(ctx);
        if (ret < above) {
            ret %= limit;
            break;
        }
    }
    return ret;
}

#ifndef ONLY64

























void fill_array32(sfmt_t *ctx, uint32_t *array, int size) {
    assert(ctx->initialized);
    assert(ctx->idx == N32);
    assert(size % 4 == 0);
    assert(size >= N32);

    gen_rand_array(ctx, (w128_t *)array, size / 4);
    ctx->idx = N32;
}
#endif


























void fill_array64(sfmt_t *ctx, uint64_t *array, int size) {
    assert(ctx->initialized);
    assert(ctx->idx == N32);
    assert(size % 2 == 0);
    assert(size >= N64);

    gen_rand_array(ctx, (w128_t *)array, size / 2);
    ctx->idx = N32;

#if defined(BIG_ENDIAN64) && !defined(ONLY64)
    swap((w128_t *)array, size /2);
#endif
}







sfmt_t *init_gen_rand(uint32_t seed) {
    void *p;
    sfmt_t *ctx;
    int i;
    uint32_t *psfmt32;

    if (posix_memalign(&p, sizeof(w128_t), sizeof(sfmt_t)) != 0) {
	return NULL;
    }
    ctx = (sfmt_t *)p;
    psfmt32 = &ctx->sfmt[0].u[0];

    psfmt32[idxof(0)] = seed;
    for (i = 1; i < N32; i++) {
	psfmt32[idxof(i)] = 1812433253UL * (psfmt32[idxof(i - 1)] 
					    ^ (psfmt32[idxof(i - 1)] >> 30))
	    + i;
    }
    ctx->idx = N32;
    period_certification(ctx);
    ctx->initialized = 1;

    return ctx;
}







sfmt_t *init_by_array(uint32_t *init_key, int key_length) {
    void *p;
    sfmt_t *ctx;
    int i, j, count;
    uint32_t r;
    int lag;
    int mid;
    int size = N * 4;
    uint32_t *psfmt32;

    if (posix_memalign(&p, sizeof(w128_t), sizeof(sfmt_t)) != 0) {
	return NULL;
    }
    ctx = (sfmt_t *)p;
    psfmt32 = &ctx->sfmt[0].u[0];

    if (size >= 623) {
	lag = 11;
    } else if (size >= 68) {
	lag = 7;
    } else if (size >= 39) {
	lag = 5;
    } else {
	lag = 3;
    }
    mid = (size - lag) / 2;

    memset(ctx->sfmt, 0x8b, sizeof(ctx->sfmt));
    if (key_length + 1 > N32) {
	count = key_length + 1;
    } else {
	count = N32;
    }
    r = func1(psfmt32[idxof(0)] ^ psfmt32[idxof(mid)] 
	      ^ psfmt32[idxof(N32 - 1)]);
    psfmt32[idxof(mid)] += r;
    r += key_length;
    psfmt32[idxof(mid + lag)] += r;
    psfmt32[idxof(0)] = r;

    count--;
    for (i = 1, j = 0; (j < count) && (j < key_length); j++) {
	r = func1(psfmt32[idxof(i)] ^ psfmt32[idxof((i + mid) % N32)] 
		  ^ psfmt32[idxof((i + N32 - 1) % N32)]);
	psfmt32[idxof((i + mid) % N32)] += r;
	r += init_key[j] + i;
	psfmt32[idxof((i + mid + lag) % N32)] += r;
	psfmt32[idxof(i)] = r;
	i = (i + 1) % N32;
    }
    for (; j < count; j++) {
	r = func1(psfmt32[idxof(i)] ^ psfmt32[idxof((i + mid) % N32)] 
		  ^ psfmt32[idxof((i + N32 - 1) % N32)]);
	psfmt32[idxof((i + mid) % N32)] += r;
	r += i;
	psfmt32[idxof((i + mid + lag) % N32)] += r;
	psfmt32[idxof(i)] = r;
	i = (i + 1) % N32;
    }
    for (j = 0; j < N32; j++) {
	r = func2(psfmt32[idxof(i)] + psfmt32[idxof((i + mid) % N32)] 
		  + psfmt32[idxof((i + N32 - 1) % N32)]);
	psfmt32[idxof((i + mid) % N32)] ^= r;
	r -= i;
	psfmt32[idxof((i + mid + lag) % N32)] ^= r;
	psfmt32[idxof(i)] = r;
	i = (i + 1) % N32;
    }

    ctx->idx = N32;
    period_certification(ctx);
    ctx->initialized = 1;

    return ctx;
}

void fini_gen_rand(sfmt_t *ctx) {
    assert(ctx != NULL);

    ctx->initialized = 0;
    free(ctx);
}
