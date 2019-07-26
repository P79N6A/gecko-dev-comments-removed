


















































#ifndef SFMT_ALTI_H
#define SFMT_ALTI_H









JEMALLOC_ALWAYS_INLINE
vector unsigned int vec_recursion(vector unsigned int a,
						vector unsigned int b,
						vector unsigned int c,
						vector unsigned int d) {

    const vector unsigned int sl1 = ALTI_SL1;
    const vector unsigned int sr1 = ALTI_SR1;
#ifdef ONLY64
    const vector unsigned int mask = ALTI_MSK64;
    const vector unsigned char perm_sl = ALTI_SL2_PERM64;
    const vector unsigned char perm_sr = ALTI_SR2_PERM64;
#else
    const vector unsigned int mask = ALTI_MSK;
    const vector unsigned char perm_sl = ALTI_SL2_PERM;
    const vector unsigned char perm_sr = ALTI_SR2_PERM;
#endif
    vector unsigned int v, w, x, y, z;
    x = vec_perm(a, (vector unsigned int)perm_sl, perm_sl);
    v = a;
    y = vec_sr(b, sr1);
    z = vec_perm(c, (vector unsigned int)perm_sr, perm_sr);
    w = vec_sl(d, sl1);
    z = vec_xor(z, w);
    y = vec_and(y, mask);
    v = vec_xor(v, x);
    z = vec_xor(z, y);
    z = vec_xor(z, v);
    return z;
}





JEMALLOC_INLINE void gen_rand_all(sfmt_t *ctx) {
    int i;
    vector unsigned int r, r1, r2;

    r1 = ctx->sfmt[N - 2].s;
    r2 = ctx->sfmt[N - 1].s;
    for (i = 0; i < N - POS1; i++) {
	r = vec_recursion(ctx->sfmt[i].s, ctx->sfmt[i + POS1].s, r1, r2);
	ctx->sfmt[i].s = r;
	r1 = r2;
	r2 = r;
    }
    for (; i < N; i++) {
	r = vec_recursion(ctx->sfmt[i].s, ctx->sfmt[i + POS1 - N].s, r1, r2);
	ctx->sfmt[i].s = r;
	r1 = r2;
	r2 = r;
    }
}








JEMALLOC_INLINE void gen_rand_array(sfmt_t *ctx, w128_t *array, int size) {
    int i, j;
    vector unsigned int r, r1, r2;

    r1 = ctx->sfmt[N - 2].s;
    r2 = ctx->sfmt[N - 1].s;
    for (i = 0; i < N - POS1; i++) {
	r = vec_recursion(ctx->sfmt[i].s, ctx->sfmt[i + POS1].s, r1, r2);
	array[i].s = r;
	r1 = r2;
	r2 = r;
    }
    for (; i < N; i++) {
	r = vec_recursion(ctx->sfmt[i].s, array[i + POS1 - N].s, r1, r2);
	array[i].s = r;
	r1 = r2;
	r2 = r;
    }
    
    for (; i < size - N; i++) {
	r = vec_recursion(array[i - N].s, array[i + POS1 - N].s, r1, r2);
	array[i].s = r;
	r1 = r2;
	r2 = r;
    }
    for (j = 0; j < 2 * N - size; j++) {
	ctx->sfmt[j].s = array[j + size - N].s;
    }
    for (; i < size; i++) {
	r = vec_recursion(array[i - N].s, array[i + POS1 - N].s, r1, r2);
	array[i].s = r;
	ctx->sfmt[j++].s = r;
	r1 = r2;
	r2 = r;
    }
}

#ifndef ONLY64
#if defined(__APPLE__)
#define ALTI_SWAP (vector unsigned char) \
	(4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11)
#else
#define ALTI_SWAP {4, 5, 6, 7, 0, 1, 2, 3, 12, 13, 14, 15, 8, 9, 10, 11}
#endif







JEMALLOC_INLINE void swap(w128_t *array, int size) {
    int i;
    const vector unsigned char perm = ALTI_SWAP;

    for (i = 0; i < size; i++) {
	array[i].s = vec_perm(array[i].s, (vector unsigned int)perm, perm);
    }
}
#endif

#endif
