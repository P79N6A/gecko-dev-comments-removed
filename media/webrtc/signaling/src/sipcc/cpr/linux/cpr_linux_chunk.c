































































































































#include "cpr_types.h"
#include "cpr_linux_align.h"
#include "cpr_linux_chunk.h"
#include "cpr_assert.h"
#include "cpr_debug.h"
#include "cpr_locks.h"
#include "cpr_stdlib.h"
#include "cpr_linux_memory_api.h"

#ifndef MALLOC
#error "Need MALLOC to be defined"
#endif







#ifdef CPR_CHUNK_DEBUG
#define CHUNK_DEBUG if (cprInfo) buginf
#else
#define CHUNK_DEBUG(fmt, arg)
#endif

#define CHUNK_PRIVATE_MEMPOOL_ARG
#define CHUNK_PRIVATE_MEMPOOL_VAR
#define CHUNK_PRIVATE_MEMPOOL_ARG_REF(chunk)
#define CHUNK_PRIVATE_MEMPOOL_ASSIGN(chunk)








#define MAX_CHUNK_POISON  0x100


#define MEM_ALIGN_BYTES 0x2








#define MEMASSERT(x)  cpr_assert_debug(((int)x & (MEM_ALIGN_BYTES - 1)) == 0)

#define CHUNK_CALLER_PC __builtin_return_address(0)






static boolean chunks_initialized = FALSE;
static uint32_t chunks_created = 0;
static uint32_t chunks_destroyed = 0;
static uint32_t chunk_siblings_created = 0;
static uint32_t chunk_siblings_trimmed = 0;
static boolean  chunk_siblings_stop_shuffling = FALSE;
static cprMutex_t chunk_mutex;







static INLINE chunk_t *
chunk_create_inline(uint32_t cfg_size, uint32_t cfg_max, uint32_t flags,
                    uint32_t alignment,
                    CHUNK_PRIVATE_MEMPOOL_ARG
                    const char *name, void *caller_pc);











static INLINE boolean
chunk_is_dynamic (chunk_t *chunk)
{
    return (boolean)(chunk->flags & CHUNK_FLAGS_DYNAMIC);
}

static INLINE boolean
chunk_is_sibling (chunk_t *chunk)
{
    return (boolean)(chunk->flags & CHUNK_FLAGS_SIBLING);
}

static INLINE boolean
chunk_is_independent (chunk_t *chunk)
{
    return (boolean)(chunk->flags & CHUNK_FLAGS_INDEPENDENT);
}

static INLINE boolean
chunk_is_resident (chunk_t *chunk)
{
    return (boolean)(chunk->flags & CHUNK_FLAGS_RESIDENT);
}

static INLINE boolean
chunk_is_unzeroed (chunk_t *chunk)
{
    return (boolean)(chunk->flags & CHUNK_FLAGS_UNZEROED);
}

boolean
chunk_did_malloc_fail (chunk_t *chunk)
{
    return (boolean)(chunk->flags & CHUNK_FLAGS_MALLOCFAILED);
}

















boolean
chunk_init (void)
{
    if (chunks_initialized) {
        return TRUE;
    }

    


    chunk_mutex = cprCreateMutex("CPR chunk mutex");
    if (!chunk_mutex) {
        CPR_ERROR("ERROR: Unable to create CPR chunk mutex\n");
        return FALSE;
    }

    


    chunks_created         = 0;
    chunks_destroyed       = 0;
    chunk_siblings_created = 0;
    chunk_siblings_trimmed = 0;

    


    chunks_initialized = TRUE;

    return TRUE;
}






void
chunk_exit (void)
{
    
    (void) cprDestroyMutex(chunk_mutex);

    chunks_initialized = FALSE;
}














static INLINE uint32_t
chunk_calculate_size (uint32_t size, uint32_t alignment, uint32_t flags)
{
    uint32_t overhead;

#ifdef CPR_CHUNK_DEBUG
    CHUNK_DEBUG("chunk_calculate_size(%u, %u, 0x%u)\n", size, alignment, flags);
#endif

    if (!alignment) {
        alignment = CHUNK_ALIGN_BYTES;
    }
    alignment = ALIGN_UP(alignment, CHUNK_ALIGN_BYTES);

    
    overhead = FIELDOFFSET(chunk_header_t, data_area.data);
    overhead = ALIGN_UP(overhead, alignment);

    



    if (size < sizeof(free_chunk_t)) {
        size = sizeof(free_chunk_t);
    }
    size = ALIGN_UP(size, alignment);

    CHUNK_DEBUG("chunk_calculate_size yields %u\n", (size + overhead));

    return (size + overhead);
}

#ifdef NEED_TO_ADJUST_MEMSIZE


















static INLINE uint32_t
chunk_get_req_memsize (chunk_t *chunk, uint32_t *overhead,
                       uint32_t *req_memsize)
{
    *overhead = 0;
    return *req_memsize;
}
#endif











static INLINE free_chunk_t * 
hdr_to_free_chunk (chunk_header_t *hdr)
{
    cpr_assert_debug(hdr != NULL);

    return &(hdr->data_area.free_info);
}










static INLINE chunk_header_t *
data_to_chunk_hdr (chunk_header_t *p)
{
    cpr_assert_debug(p != NULL);

    return (chunk_header_t *)
        ((unsigned char *)p - FIELDOFFSET(chunk_header_t, data_area.data));
}











static INLINE boolean
chunk_destroy_sibling_ok (chunk_t *chunk)
{
    return (boolean)((chunk->flags &
            (CHUNK_FLAGS_SIBLING | CHUNK_FLAGS_DYNAMIC | CHUNK_FLAGS_RESIDENT))
            == (CHUNK_FLAGS_SIBLING | CHUNK_FLAGS_DYNAMIC));
}













































static void
chunk_populate_freelist (chunk_t *chunk, uint32_t real_size)
{
    void          *data;
    uint32_t       count;
    free_chunk_t  *freehdr;
    uint32_t       alignment;
    uint32_t       data_alignment;
    chunk_header_t *hdr;

    if (!chunk) {
        return;
    }

#ifdef CPR_CHUNK_DEBUG
    CHUNK_DEBUG("chunk_populate_freelist(%x, %u)\n", chunk, real_size);
#endif

    alignment = chunk->alignment;
    data = chunk->data;

#ifdef CPR_CHUNK_DEBUG
    CHUNK_DEBUG("    alignment = %u, data = %x\n", alignment, data);
#endif

    



    for (count = 0; count < chunk->real_max; count++) {
        



        if (alignment) {
            hdr = (chunk_header_t *)data;
            freehdr = hdr_to_free_chunk(hdr);
            data_alignment = ((uintptr_t)freehdr % alignment);
            if (data_alignment) {
                data_alignment = alignment - data_alignment;
                



                data = (void *)((uintptr_t)data + data_alignment);
            }
        }
        hdr = (chunk_header_t *) data;
        freehdr = hdr_to_free_chunk(hdr);

        chunk->freelist[count] = (void *) freehdr;
        freehdr->magic = FREECHUNKMAGIC;
        freehdr->last_deallocator = 0;

        hdr->root_chunk = chunk;
        data = (void *) ((uintptr_t)data + real_size);
    }
}























static chunk_t *
chunk_create_inline (uint32_t cfg_size, uint32_t cfg_max, uint32_t flags,
                     uint32_t alignment,
                     CHUNK_PRIVATE_MEMPOOL_ARG
                     const char *name, void *caller_pc)
{
    chunk_t *chunk;
    uint8_t *chunk_data;
    uint32_t total_datasize, overhead;
#ifdef NEED_TO_ADJUST_MEMSIZE
    uint32_t temp;
#endif
    uint32_t real_size, real_max, req_memsize;
    uint32_t max_chunk_size;

    max_chunk_size = 0;

#ifdef CPR_CHUNK_DEBUG
    CHUNK_DEBUG("chunk_create(%u, %u, 0x%x, %u, %s, %p)\n",
                cfg_size, cfg_max, flags, alignment, name, caller_pc);
#endif

    


    if (alignment && (alignment & (alignment-1))) {
        CHUNK_DEBUG("chunk_create: alignment not a power of 2: %d\n",
                    alignment);
        return NULL;
    }

    chunk_data = NULL;

    



    if (alignment) {
        flags |= CHUNK_FLAGS_INDEPENDENT;
    }

    






    real_size = chunk_calculate_size(cfg_size, alignment, flags);
    overhead = 0;

    if (flags & CHUNK_FLAGS_DYNAMIC) {
        max_chunk_size = CHUNK_MEMBLOCK_MAXSIZE - sizeof(chunk_t)
                         - (sizeof(uint8_t *) * cfg_max);

        


        if (real_size > max_chunk_size) {
            CPR_ERROR("CPR Chunk pool size too large: %u", real_size);
            return NULL;
        }
    }

    






    if (flags & CHUNK_FLAGS_INDEPENDENT) {
        total_datasize = real_size * cfg_max;

        


        chunk_data = MEMALIGN(alignment, total_datasize);
        if (!chunk_data) {
            CPR_ERROR("Out-of-memory for chunk\n");
            return NULL;
        }

        


#ifdef NEED_TO_ADJUST_MEMSIZE
        total_datasize = chunk_get_req_memsize((chunk_t *)chunk_data,
                                               &temp, &total_datasize);
        total_datasize -= temp;
#endif

        if (flags & CHUNK_FLAGS_DYNAMIC) {
            real_max = total_datasize / real_size;
        } else {
            real_max = cfg_max;
        }
        overhead = total_datasize - real_max*real_size;

        



        req_memsize = sizeof(chunk_t) + (sizeof(uint8_t *) * real_max);

        


        chunk =(chunk_t *)MALLOC(req_memsize);
        if (!chunk) {
            CPR_ERROR("Out-of-memory for chunk\n");
            free(chunk_data);
            return NULL;
        }
#ifdef NEED_TO_ADJUST_MEMSIZE
        total_datasize = chunk_get_req_memsize(chunk, &temp, &req_memsize);
        overhead += total_datasize - req_memsize - temp;
#else
        total_datasize = req_memsize;
        overhead = 0;
#endif
    } else {
        





        if (cfg_max & 1) {
            cfg_max++;
        }

        


        req_memsize = sizeof(chunk_t) +
            (sizeof(uint8_t *) + real_size) * cfg_max;

        


        chunk =(chunk_t *)MALLOC(req_memsize);
        if (!chunk) {
            CHUNK_DEBUG("Out-of-memory for chunk: %d\n", req_memsize);
            return NULL;
        }

CHUNK_DEBUG("create_chunk yields chunk_header = 0x%x\n", chunk);
CHUNK_DEBUG("of size %u\n", req_memsize);

        



#ifdef NEED_TO_ADJUST_MEMSIZE
        total_datasize = chunk_get_req_memsize(chunk, &temp, &req_memsize);
        total_datasize -= (sizeof(chunk_t) + temp);
#else
        total_datasize = req_memsize - sizeof(chunk_t);
#endif
CHUNK_DEBUG("total_datasize = %u\n", total_datasize);

        


        if (flags & CHUNK_FLAGS_DYNAMIC) {
            real_max = total_datasize / (sizeof(uint8_t *) + real_size);
        } else {
            real_max = cfg_max;
        }

        


        overhead = total_datasize - real_max * (real_size + sizeof(uint8_t *));
CHUNK_DEBUG("overhead = %u\n", overhead);
    }

    


    chunk->cfg_size     = cfg_size;
    chunk->real_size    = real_size;
    chunk->cfg_max      = cfg_max;
    chunk->real_max     = real_max;
    chunk->index        = real_max;
    chunk->total_inuse  = 0;
    chunk->inuse_hwm    = 0;
    chunk->total_free   = 0;
    chunk->flags        = flags;
    CHUNK_PRIVATE_MEMPOOL_ASSIGN(chunk);
    chunk->next         = NULL;
    chunk->alignment    = (uint16_t) alignment;  
    chunk->next_sibling = NULL;
    chunk->prev_sibling = NULL;
    chunk->p.tail       = NULL;
    chunk->overhead     = (uint16_t) overhead; 
    strncpy(chunk->name, name, sizeof(chunk->name) - 1);
    chunk->name[sizeof(chunk->name) - 1] = '\0';

    


    if (chunk_is_independent(chunk)) {
        chunk->data = chunk_data;
        
    } else {
        chunk->data = (uint8_t *) &chunk->freelist[real_max];
        MEMASSERT(chunk->data);
    }

    


    chunk->end = chunk->data + (real_size * real_max);

    



    chunk_populate_freelist(chunk, real_size);

    


    chunks_created++;

    



    if (!chunk_is_sibling(chunk)) {
        chunk->total_free = real_max;
    } else {
        chunk->total_free = 0;
    }

CHUNK_DEBUG("return chunk 0x%x\n", chunk);
    return chunk;
}



















chunk_t *
chunk_create (uint32_t cfg_size,
              uint32_t cfg_max,
              uint32_t flags,
              uint32_t alignment,
              CHUNK_PRIVATE_MEMPOOL_ARG
              const char *name)
{

    void *caller_pc = CHUNK_CALLER_PC;

    if ((flags & CHUNK_FLAGS_DYNAMIC) &&
        (cfg_size > CHUNK_MEMBLOCK_MAXSIZE)) {
        CPR_ERROR("Invalid chunk size = %u\n", cfg_size);
        return NULL;
    }

    





    (void) chunk_init();

    return chunk_create_inline(cfg_size, cfg_max, flags, alignment,
                               CHUNK_PRIVATE_MEMPOOL_VAR
                               name, caller_pc);
}

















chunk_t *
chunk_create_caller (uint32_t cfg_size,
                     uint32_t cfg_max,
                     uint32_t flags,
                     uint32_t alignment,
                     CHUNK_PRIVATE_MEMPOOL_ARG
                     const char *name,
                     void *caller_pc)
{
    if ((flags & CHUNK_FLAGS_DYNAMIC) &&
        (cfg_size > CHUNK_MEMBLOCK_MAXSIZE)) {
        CPR_ERROR("Invalid chunk size = %u\n", cfg_size);
        return NULL;
    }

    





    (void) chunk_init();

    return chunk_create_inline(cfg_size, cfg_max, flags, alignment,
                               CHUNK_PRIVATE_MEMPOOL_VAR
                               name, caller_pc);
}









boolean
chunk_is_destroyable (chunk_t *chunk)
{
    if (!chunk) {
        return FALSE;
    }

    
    if (chunk->index != chunk->real_max) {
        return FALSE;
    }

    
    if (chunk->next_sibling != NULL) {
        return FALSE;
    }

    return TRUE;
}

















static void
chunk_remove_sibling (chunk_t *head_chunk, chunk_t *chunk)
{
    if (chunk == head_chunk->p.tail) {
        head_chunk->p.tail = chunk->prev_sibling;
    }

    chunk->prev_sibling->next_sibling = chunk->next_sibling;
    if (chunk->next_sibling) {
        chunk->next_sibling->prev_sibling = chunk->prev_sibling;
    }
    chunk->next_sibling = NULL;
    chunk->prev_sibling = NULL;
}




















static void
chunk_insert_sibling (chunk_t *head_chunk, chunk_t *append,
                      chunk_t *chunk)
{
    if (append->next_sibling) {
        chunk->next_sibling = append->next_sibling;
        chunk->prev_sibling = append;
        append->next_sibling->prev_sibling = chunk;
        append->next_sibling = chunk;
    } else {
        if (append == head_chunk) {
            head_chunk->p.tail = chunk;
        }
        chunk->next_sibling = NULL;
        chunk->prev_sibling = append;
        append->next_sibling = chunk;
    }

    if (append == head_chunk->p.tail) {
        head_chunk->p.tail = chunk;
    }
}



















static INLINE void
chunk_turn_empty (chunk_t *head_chunk, chunk_t *chunk)
{
    if (chunk == head_chunk) {
        return;
    }

    



    if (head_chunk->next_sibling == head_chunk->p.tail ||
        chunk_siblings_stop_shuffling) {
        return;
    }

    (void) cprGetMutex(chunk_mutex);
    chunk_remove_sibling(head_chunk, chunk);
    chunk_insert_sibling(head_chunk, head_chunk->p.tail, chunk);
    (void) cprReleaseMutex(chunk_mutex);
    return;
}

























static INLINE void
chunk_turn_not_empty (chunk_t *head_chunk, chunk_t *chunk)
{
    chunk_t *append;

    if (chunk == head_chunk) {
        return;
    }

    



    if (head_chunk->next_sibling == head_chunk->p.tail ||
        chunk_siblings_stop_shuffling) {
        return;
    }

    (void) cprGetMutex(chunk_mutex);
    chunk_remove_sibling(head_chunk, chunk);
    append = head_chunk->next_sibling;
    if (!append->index) {
        append = head_chunk;
    }
    chunk_insert_sibling(head_chunk, append, chunk);
    (void) cprReleaseMutex(chunk_mutex);
    return;
}











boolean
chunk_destroy_internal (chunk_t *chunk, void *caller_pc, boolean force)
{
    chunk_t *head_chunk = NULL;

    


    if (!chunk) {
        return FALSE;
    }

    


    (void) cprGetMutex(chunk_mutex);
    if ((chunk->index != chunk->real_max) && (force == FALSE)) {
        


        (void) cprReleaseMutex(chunk_mutex);
        return FALSE;
    }

    


    if (chunk_is_sibling(chunk)) {
        


        head_chunk = chunk->p.head_chunk;

        chunk_remove_sibling(head_chunk, chunk);
        head_chunk->total_free -= chunk->index;
        (void) cprReleaseMutex(chunk_mutex);

        chunk_siblings_trimmed++;
    } else {
        



        head_chunk = chunk;
        if (chunk->next_sibling != NULL) {
            (void) cprReleaseMutex(chunk_mutex);
            CPR_ERROR("Can not destroy chunk due to siblings, 0x%x", chunk);
            return FALSE;
        }

        


        (void) cprReleaseMutex(chunk_mutex);
    }
    chunks_destroyed++;

    


    if (chunk_is_independent(chunk)) {
        FREE(chunk->data);
    }

    CHUNK_DEBUG("Free chunk 0x%x, ", chunk);
    CHUNK_DEBUG("caller_pc %p\n", caller_pc);
    FREE(chunk);

    return TRUE;
}








boolean
chunk_destroy (chunk_t *chunk)
{
    void *caller_pc = CHUNK_CALLER_PC;

    if (chunk && chunk_is_resident(chunk) && !chunk_is_sibling(chunk) &&
        chunk->total_inuse == 0) {
        while (chunk->next_sibling) {
            if (!chunk_destroy_internal(chunk->next_sibling, caller_pc,
                                        FALSE)) {
                return FALSE;
            }
        }
    }
    return chunk_destroy_internal(chunk, caller_pc, FALSE);
}
















boolean
chunk_destroy_forced (chunk_t *chunk)
{
    void *caller_pc = CHUNK_CALLER_PC;

    if (!chunk) {
        return FALSE;
    }

    if (chunk_is_sibling(chunk)) {
        CPR_ERROR("Chunk is a sibling, 0x%x, name = %s\n", chunk, chunk->name);
        return FALSE;
    }

    while (chunk->next_sibling) {
        if (!chunk_destroy_internal(chunk->next_sibling, caller_pc,
                                    TRUE)) {
            return FALSE;
        }
    }
    return chunk_destroy_internal(chunk, caller_pc, TRUE);
}

















static void
chunk_prepare_data (void *data, chunk_t *chunk, chunk_t *head_chunk)
{
    chunk_header_t *hdr;

    cpr_assert_debug(data != NULL);
    cpr_assert_debug(chunk != NULL);
    cpr_assert_debug(head_chunk != NULL);

    


    if (head_chunk->total_inuse > head_chunk->inuse_hwm) {
        head_chunk->inuse_hwm = head_chunk->total_inuse;
    }

    


    hdr = data_to_chunk_hdr(data);
    hdr->root_chunk = chunk; 

    if (!chunk_is_unzeroed(chunk)) {
        memset(data, 0, chunk->cfg_size);
    } else {
        ((free_chunk_t *)data)->last_deallocator = NULL;
    }
}



















static void *
chunk_malloc_inline (chunk_t *chunk, void *caller_pc)
{
    int index;
    void *data;
    chunk_t *head_chunk;

    


    if (!chunk) {
        CPR_ERROR("chunk_malloc: chunk is (NULL)\n");
        return NULL;
    }

    head_chunk = chunk;

    

    if (!head_chunk->total_free && !chunk_is_dynamic(head_chunk)){
        


        chunk->flags |= CHUNK_FLAGS_MALLOCFAILED;
        return NULL;
    }
    
    


    (void) cprGetMutex(chunk_mutex);

    


    if (head_chunk->total_free) {

        
        if (head_chunk->total_free & (1 << 31)) {
            
            (void) cprReleaseMutex(chunk_mutex);
            cpr_crashdump();
        }

        while (chunk) {
            


            if (chunk->index > 0) {
                


                index = --chunk->index;
                head_chunk->total_free--;

                


                if ((index == 0) && head_chunk->total_free) {
                    chunk_turn_empty(head_chunk, chunk);
                }

                


                head_chunk->total_inuse++;

                


                data = chunk->freelist[index];

                


                (void) cprReleaseMutex(chunk_mutex);

                


                chunk_prepare_data(data, chunk, head_chunk);

                CHUNK_DEBUG("Allocate chunk node: data = 0x%x\n", data);
                CHUNK_DEBUG("Allocate chunk node: pc = %p\n", caller_pc);
                return data;
            }

            chunk = chunk->next_sibling;
        }
        


        CPR_ERROR("No more chunk siblings available for %s: free = %u,"
                  " inuse = %u\n", head_chunk->name, head_chunk->total_free,
                  head_chunk->total_inuse);
    }

    


    (void) cprReleaseMutex(chunk_mutex);

    


    if (chunk_is_dynamic(head_chunk)) {
        chunk_t *new_chunk = NULL;

        


        new_chunk = chunk_create_inline(head_chunk->cfg_size,
                                        head_chunk->cfg_max,
                                        (head_chunk->flags |
                                         CHUNK_FLAGS_SIBLING),
                                        head_chunk->alignment,
                                        CHUNK_PRIVATE_MEMPOOL_ARG_REF(head_chunk)
                                        head_chunk->name, caller_pc);
        



        if (new_chunk) {

            new_chunk->p.head_chunk = head_chunk;

            


            (void) cprGetMutex(chunk_mutex);

            chunk_insert_sibling(head_chunk, head_chunk, new_chunk);
            head_chunk->total_free += new_chunk->real_max;

            


            index = --new_chunk->index;
            head_chunk->total_free--;

            


            head_chunk->total_inuse++;

            


            data = new_chunk->freelist[index];

            


            (void) cprReleaseMutex(chunk_mutex);

            


            MEMASSERT(data);

            


            chunk_prepare_data(data, new_chunk, head_chunk);

            CHUNK_DEBUG("Allocate chunk node: data = 0x%x\n", data);
            CHUNK_DEBUG("Allocate chunk node: pc = %p\n", caller_pc);
            chunk_siblings_created++;
            return data;
        }
    }

    return NULL;
}












void *
chunk_malloc (chunk_t *chunk)
{
    void *caller_pc = CHUNK_CALLER_PC;

    return chunk_malloc_inline(chunk, caller_pc);
}











void *
chunk_malloc_caller (chunk_t *chunk, void *caller_pc)
{
    return chunk_malloc_inline(chunk, caller_pc);
}












uint32_t
get_chunk_size (void *data)
{
    chunk_header_t *hdr;

    if (!data) {
        return 0;
    }

    


    hdr = data_to_chunk_hdr(data);

    return hdr->root_chunk->cfg_size;
}
















static void
poison_chunk (chunk_t *chunk, void *data, uint32_t size)
{
    uint32_t i;
    uint32_t *data2;

    data2 = (uint32_t *)((uintptr_t)data + sizeof(free_chunk_t));
    size -= sizeof(free_chunk_t);
    if (size > MAX_CHUNK_POISON) {
        size = MAX_CHUNK_POISON;
    }
    size /= 4;
    for (i = 0; i < size; i++) {
        *data2++ = CHUNK_POISON_PATTERN;
    }
}



















static boolean
chunk_free_body (chunk_t *chunk, chunk_t *head_chunk, void *data,
                 void *caller_pc)
{
    cpr_assert_debug(chunk != NULL);
    cpr_assert_debug(head_chunk != NULL);
    cpr_assert_debug(data != NULL);

    


    if (chunk->index >= chunk->real_max) {
        


        CPR_ERROR("Attempt to free non-empty chunk (0x%x), %s: free = %u,"
                  " max_free = %u\n", chunk, head_chunk->name, chunk->index,
                  chunk->real_max);
        return FALSE;
    }

    ((free_chunk_t *)data)->magic = FREECHUNKMAGIC;
    ((free_chunk_t *)data)->last_deallocator = (void *)caller_pc;

    


    (void) cprGetMutex(chunk_mutex);

    



    if (!chunk->index) {
        chunk_turn_not_empty(head_chunk, chunk);
    }
    chunk->freelist[chunk->index++] = data;
    head_chunk->total_free++;
    head_chunk->total_inuse--;

    (void) cprReleaseMutex(chunk_mutex);

    CHUNK_DEBUG("Free chunk node: data = 0x%x,", data);
    CHUNK_DEBUG("Free chunk node: pc = 0x%x\n", caller_pc);

    



    if (chunk->index == chunk->real_max) {
        if (chunk_destroy_sibling_ok(chunk)) {
            chunk_destroy_internal(chunk, caller_pc, FALSE);
        }
    }
    return TRUE;
}


















static boolean
chunk_free_inline (chunk_t *chunk, void *data, void *caller_pc)
{
    chunk_t *head_chunk;
    chunk_header_t *hdr;

    


    if (!data) {
        return FALSE;
    }

    


    if (!chunk) {
        hdr = data_to_chunk_hdr(data);
        chunk = hdr->root_chunk;
    }

    


    if (!chunk) {
        CPR_ERROR("chunk_free: no root chunk\n");
        cpr_crashdump();
    }

    head_chunk = NULL;
    hdr = data_to_chunk_hdr(data);
    chunk = hdr->root_chunk;

    


    if (chunk_is_sibling(chunk)) {
        head_chunk = chunk->p.head_chunk;
    } else {
        head_chunk = chunk;
    }

    if (((uint8_t *)data < chunk->data) ||
        ((uint8_t *)data >= chunk->end)) {
        


        CPR_ERROR("chunk_free: data node not in chunk (0x%x, %s) data range,"
                  " free=%d, max_free=%d\n", chunk, head_chunk->name,
                  chunk->index, chunk->real_max);
        return FALSE;
    }

    poison_chunk(chunk, data, chunk->cfg_size);
    return chunk_free_body(chunk, head_chunk, data, caller_pc);
}












boolean
chunk_free (chunk_t *chunk, void *data)
{
    void *caller_pc = CHUNK_CALLER_PC;

    return chunk_free_inline(chunk, data, caller_pc);
}












boolean
chunk_free_caller (chunk_t *chunk, void *data, void *caller_pc)
{
    return chunk_free_inline(chunk, data, caller_pc);
}









long
chunk_totalfree_count (chunk_t *chunk)
{
    if (!chunk) {
        return 0;
    }

    return chunk->total_free;
}

 

