











































#include <assert.h>


#include <sphinxbase/prim_type.h>
#include <sphinxbase/err.h>
#include <sphinxbase/ckd_alloc.h>


#include "blkarray_list.h"


#define BLKARRAY_DEFAULT_MAXBLKS	16380
#define BLKARRAY_DEFAULT_BLKSIZE	16380


blkarray_list_t *
_blkarray_list_init(int32 maxblks, int32 blksize)
{
    blkarray_list_t *bl;

    if ((maxblks <= 0) || (blksize <= 0)) {
        E_ERROR("Cannot allocate %dx%d blkarray\n", maxblks, blksize);
        return NULL;
    }

    bl = (blkarray_list_t *) ckd_calloc(1, sizeof(blkarray_list_t));
    bl->ptr = (void ***) ckd_calloc(maxblks, sizeof(void **));
    bl->maxblks = maxblks;
    bl->blksize = blksize;
    bl->n_valid = 0;
    bl->cur_row = -1;           
    bl->cur_row_free = blksize; 

    return bl;
}


blkarray_list_t *
blkarray_list_init(void)
{
    return _blkarray_list_init(BLKARRAY_DEFAULT_MAXBLKS,
                               BLKARRAY_DEFAULT_BLKSIZE);
}

void
blkarray_list_free(blkarray_list_t *bl)
{
    blkarray_list_reset(bl);
    ckd_free(bl->ptr);
    ckd_free(bl);
}


int32
blkarray_list_append(blkarray_list_t * bl, void *data)
{
    int32 id;

    assert(bl);

    if (bl->cur_row_free >= bl->blksize) {
        
        bl->cur_row++;

        if (bl->cur_row >= bl->maxblks) {
            E_ERROR("Block array (%dx%d) exhausted\n",
                    bl->maxblks, bl->blksize);
            bl->cur_row--;
            return -1;
        }

        
        assert(bl->ptr[bl->cur_row] == NULL);
        bl->ptr[bl->cur_row] = (void **) ckd_malloc(bl->blksize *
                                                    sizeof(void *));

        bl->cur_row_free = 0;
    }

    bl->ptr[bl->cur_row][bl->cur_row_free] = data;
    (bl->cur_row_free)++;

    id = (bl->n_valid)++;
    assert(id >= 0);

    return id;
}


void
blkarray_list_reset(blkarray_list_t * bl)
{
    int32 i, j;

    
    for (i = 0; i < bl->cur_row; i++) {
        for (j = 0; j < bl->blksize; j++)
            ckd_free(bl->ptr[i][j]);

        ckd_free(bl->ptr[i]);
        bl->ptr[i] = NULL;
    }
    if (i == bl->cur_row) {     
        for (j = 0; j < bl->cur_row_free; j++)
            ckd_free(bl->ptr[i][j]);

        ckd_free(bl->ptr[i]);
        bl->ptr[i] = NULL;
    }

    bl->n_valid = 0;
    bl->cur_row = -1;
    bl->cur_row_free = bl->blksize;
}

void *
blkarray_list_get(blkarray_list_t *list, int32 n)
{
    int32 r, c;

    if (n >= blkarray_list_n_valid(list))
        return NULL;

    r = n / blkarray_list_blksize(list);
    c = n - (r * blkarray_list_blksize(list));

    return blkarray_list_ptr(list, r, c);
}
