



























































#include "sphinxbase/bitvec.h"

bitvec_t *
bitvec_realloc(bitvec_t *vec,
	       size_t old_len,
               size_t new_len)
{
    bitvec_t *new_vec;
    size_t old_size = bitvec_size(old_len);
    size_t new_size = bitvec_size(new_len);
    
    new_vec = ckd_realloc(vec, new_size * sizeof(bitvec_t));
    if (new_size > old_size)
	memset(new_vec + old_size, 0, (new_size - old_size) * sizeof(bitvec_t));

    return new_vec;
}

size_t
bitvec_count_set(bitvec_t *vec, size_t len)
{
    size_t words, bits, w, b, n;
    bitvec_t *v;

    words = len / BITVEC_BITS;
    bits = len % BITVEC_BITS;
    v = vec;
    n = 0;
    for (w = 0; w < words; ++w, ++v) {
        if (*v == 0)
            continue;
        for (b = 0; b < BITVEC_BITS; ++b)
            if (*v & (1<<b))
                ++n;
    }
    for (b = 0; b < bits; ++b)
        if (*v & (1<<b))
            ++n;

    return n;
}
