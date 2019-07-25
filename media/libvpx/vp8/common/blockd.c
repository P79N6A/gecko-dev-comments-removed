










#include "blockd.h"
#include "vpx_mem/vpx_mem.h"

void vp8_setup_temp_context(TEMP_CONTEXT *t, ENTROPY_CONTEXT *a, ENTROPY_CONTEXT *l, int count)
{
    vpx_memcpy(t->l, l, sizeof(ENTROPY_CONTEXT) * count);
    vpx_memcpy(t->a, a, sizeof(ENTROPY_CONTEXT) * count);
}

const int vp8_block2left[25] = { 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 0, 0, 1, 1, 0, 0, 1, 1, 0};
const int vp8_block2above[25] = { 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 2, 3, 0, 1, 0, 1, 0, 1, 0, 1, 0};
const int vp8_block2type[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 1};
const int vp8_block2context[25] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3};
