

















































#ifndef SFMT_SSE2_H
#define SFMT_SSE2_H










JEMALLOC_ALWAYS_INLINE __m128i mm_recursion(__m128i *a, __m128i *b, 
				   __m128i c, __m128i d, __m128i mask) {
    __m128i v, x, y, z;
    
    x = _mm_load_si128(a);
    y = _mm_srli_epi32(*b, SR1);
    z = _mm_srli_si128(c, SR2);
    v = _mm_slli_epi32(d, SL1);
    z = _mm_xor_si128(z, x);
    z = _mm_xor_si128(z, v);
    x = _mm_slli_si128(x, SL2);
    y = _mm_and_si128(y, mask);
    z = _mm_xor_si128(z, x);
    z = _mm_xor_si128(z, y);
    return z;
}





JEMALLOC_INLINE void gen_rand_all(sfmt_t *ctx) {
    int i;
    __m128i r, r1, r2, mask;
    mask = _mm_set_epi32(MSK4, MSK3, MSK2, MSK1);

    r1 = _mm_load_si128(&ctx->sfmt[N - 2].si);
    r2 = _mm_load_si128(&ctx->sfmt[N - 1].si);
    for (i = 0; i < N - POS1; i++) {
	r = mm_recursion(&ctx->sfmt[i].si, &ctx->sfmt[i + POS1].si, r1, r2,
	  mask);
	_mm_store_si128(&ctx->sfmt[i].si, r);
	r1 = r2;
	r2 = r;
    }
    for (; i < N; i++) {
	r = mm_recursion(&ctx->sfmt[i].si, &ctx->sfmt[i + POS1 - N].si, r1, r2,
	  mask);
	_mm_store_si128(&ctx->sfmt[i].si, r);
	r1 = r2;
	r2 = r;
    }
}








JEMALLOC_INLINE void gen_rand_array(sfmt_t *ctx, w128_t *array, int size) {
    int i, j;
    __m128i r, r1, r2, mask;
    mask = _mm_set_epi32(MSK4, MSK3, MSK2, MSK1);

    r1 = _mm_load_si128(&ctx->sfmt[N - 2].si);
    r2 = _mm_load_si128(&ctx->sfmt[N - 1].si);
    for (i = 0; i < N - POS1; i++) {
	r = mm_recursion(&ctx->sfmt[i].si, &ctx->sfmt[i + POS1].si, r1, r2,
	  mask);
	_mm_store_si128(&array[i].si, r);
	r1 = r2;
	r2 = r;
    }
    for (; i < N; i++) {
	r = mm_recursion(&ctx->sfmt[i].si, &array[i + POS1 - N].si, r1, r2,
	  mask);
	_mm_store_si128(&array[i].si, r);
	r1 = r2;
	r2 = r;
    }
    
    for (; i < size - N; i++) {
	r = mm_recursion(&array[i - N].si, &array[i + POS1 - N].si, r1, r2,
			 mask);
	_mm_store_si128(&array[i].si, r);
	r1 = r2;
	r2 = r;
    }
    for (j = 0; j < 2 * N - size; j++) {
	r = _mm_load_si128(&array[j + size - N].si);
	_mm_store_si128(&ctx->sfmt[j].si, r);
    }
    for (; i < size; i++) {
	r = mm_recursion(&array[i - N].si, &array[i + POS1 - N].si, r1, r2,
			 mask);
	_mm_store_si128(&array[i].si, r);
	_mm_store_si128(&ctx->sfmt[j++].si, r);
	r1 = r2;
	r2 = r;
    }
}

#endif
