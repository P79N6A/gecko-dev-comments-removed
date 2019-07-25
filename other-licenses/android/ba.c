



























#include "linker.h"
#include "linker_debug.h"
#include "ba.h"

#undef min
#define min(a,b) ((a)<(b)?(a):(b))

#define BA_IS_FREE(index) (!(ba->bitmap[index].allocated))
#define BA_ORDER(index) ba->bitmap[index].order
#define BA_BUDDY_INDEX(index) ((index) ^ (1 << BA_ORDER(index)))
#define BA_NEXT_INDEX(index) ((index) + (1 << BA_ORDER(index)))
#define BA_OFFSET(index) ((index) * ba->min_alloc)
#define BA_START_ADDR(index) (BA_OFFSET(index) + ba->base)
#define BA_LEN(index) ((1 << BA_ORDER(index)) * ba->min_alloc)

static unsigned long ba_order(struct ba *ba, unsigned long len);

void ba_init(struct ba *ba)
{
    int i, index = 0;

    unsigned long max_order = ba_order(ba, ba->size);
    if (ba->max_order == 0 || ba->max_order > max_order)
        ba->max_order = max_order;

    for (i = sizeof(ba->num_entries) * 8 - 1; i >= 0; i--) {
        if (ba->num_entries &  1<<i) {
            BA_ORDER(index) = i;
            index = BA_NEXT_INDEX(index);
        }
    }
}

int ba_free(struct ba *ba, int index)
{
    int buddy, curr = index;

    
    ba->bitmap[curr].allocated = 0;
    



    do {
        buddy = BA_BUDDY_INDEX(curr);
        if (BA_IS_FREE(buddy) &&
                BA_ORDER(buddy) == BA_ORDER(curr)) {
            BA_ORDER(buddy)++;
            BA_ORDER(curr)++;
            curr = min(buddy, curr);
        } else {
            break;
        }
    } while (curr < ba->num_entries);

    return 0;
}

static unsigned long ba_order(struct ba *ba, unsigned long len)
{
    unsigned long i;

    len = (len + ba->min_alloc - 1) / ba->min_alloc;
    len--;
    for (i = 0; i < sizeof(len)*8; i++)
        if (len >> i == 0)
            break;
    return i;
}

int ba_allocate(struct ba *ba, unsigned long len)
{
    int curr = 0;
    int end = ba->num_entries;
    int best_fit = -1;
    unsigned long order = ba_order(ba, len);

    if (order > ba->max_order)
        return -1;

    



    while (curr < end) {
        if (BA_IS_FREE(curr)) {
            if (BA_ORDER(curr) == (unsigned char)order) {
                
                best_fit = curr;
                break;
            }
            if (BA_ORDER(curr) > (unsigned char)order &&
                (best_fit < 0 ||
                 BA_ORDER(curr) < BA_ORDER(best_fit)))
                best_fit = curr;
        }
        curr = BA_NEXT_INDEX(curr);
    }

    


    if (best_fit < 0)
        return -1;

    



    while (BA_ORDER(best_fit) > (unsigned char)order) {
        int buddy;
        BA_ORDER(best_fit) -= 1;
        buddy = BA_BUDDY_INDEX(best_fit);
        BA_ORDER(buddy) = BA_ORDER(best_fit);
    }
    ba->bitmap[best_fit].allocated = 1;
    return best_fit;
}

unsigned long ba_start_addr(struct ba *ba, int index)
{
    return BA_START_ADDR(index);
}

unsigned long ba_len(struct ba *ba, int index)
{
    return BA_LEN(index);
}
