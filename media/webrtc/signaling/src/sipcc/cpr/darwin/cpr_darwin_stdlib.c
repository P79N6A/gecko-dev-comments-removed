






































#include "cpr.h"
#include "cpr_types.h"
#include "cpr_debug.h"
#include "cpr_assert.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_time.h"
#include "cpr_memory.h"
#include "cpr_locks.h"
#include "cpr_darwin_align.h"
#include "cpr_darwin_chunk.h"
#include "cpr_darwin_memory_api.h"
#include "plat_api.h"
#include <errno.h>
#include <sys/syslog.h>

















#define SIZE_OF_MEMORY_HEADER (offsetof(cpr_mem_t, mem))


#define CPR_TRACKING_MAGIC 0xabcdc0de


#define CPR_POISON 0xdeadc0de


#define CPR_REDZONE 0xbeef00ee


#define SIZE_OF_REDZONE 4


#define MIN_MEMORY_SIZE 4


#define REDZONE_RECORD_SIZE 16


#define DEFAULT_POISON_SIZE 16


#define MAX_POISON_SIZE 128


#define DEFAULT_TRACKING_SIZE 1024


#define MAX_TRACKING_SIZE (12*1024)


#define PRIVATE_SYS_MEM_GUARD_SIZE 64


#define BEGIN_GUARD_VALUE 0xBBAA


#define END_GUARD_VALUE 0x0055


#define MEM_TRACKING_FLAG_OOC 0x00000001


#define MEM_TRACKING_FLAG_FREE_FAILED 0x00000002


#define MEM_HIGH_WATER_MARK ((PRIVATE_SYS_MEM_SIZE >> 3) * 7)


#define MEM_LOW_WATER_MARK ((PRIVATE_SYS_MEM_SIZE >> 2) * 3)

#define NUM_OF_CPR_SHOW_ARGUMENTS 3
 


















typedef struct cpr_mem_statistics_s
{
    uint32_t total_failed_allocs;
    uint32_t total_allocs;       
    uint32_t mallocs;            
    uint32_t callocs;            
    uint32_t reallocs;           
    uint32_t frees;              
    uint32_t total_alloc_size;   
    uint32_t in_use_count;       
    uint32_t in_use_size;        
    uint32_t max_use_size;       
    uint32_t allocs_1_to_63;     
    uint32_t allocs_64_to_255;   
    uint32_t allocs_256_to_1k;   
    uint32_t allocs_1k_to_4k;    
    uint32_t allocs_4k_to_32k;   
    uint32_t allocs_32k_plus;    
    uint32_t redzone_violations; 
    uint32_t redzone_size;       
    void *redzone_pc;            
    uint8_t redzone_memory[REDZONE_RECORD_SIZE];  
} cpr_mem_statistics_t;




typedef struct cpr_mem_tracking_node_s
{
    struct cpr_mem_tracking_node_s *next; 
    struct cpr_mem_tracking_node_s *prev; 
    void *mem;                   
    void *pc;                    
    size_t size;                 
    uint32_t magic;              
    cpr_time_t timestamp;        
} cpr_mem_tracking_node_t;




typedef struct cpr_mem_tracking_list_s
{
    cpr_mem_tracking_node_t *head; 
    cpr_mem_tracking_node_t *tail; 
    uint16_t max_size;             
    uint16_t real_size;            
    uint32_t missing_allocs;       
    boolean enabled;               
    uint32_t flags;                
    cpr_time_t timestamp;          
} cpr_mem_tracking_t;









typedef struct cpr_mem_s
{
    union {
        size_t size;             
        cpr_mem_tracking_node_t *node; 
    } u;
    void *pc;                    
    void *mem;                   
} cpr_mem_t;

 












#define GET_UINT32_PTR(mem, adj) ((uint32_t *)((uint8_t *)(mem) + (adj)))




#define WRITE_4BYTES_MEM_ALIGNED WRITE_4BYTES_UNALIGNED
#define  READ_4BYTES_MEM_ALIGNED  READ_4BYTES_UNALIGNED




#define GET_REDZONE_PTR(cpr_mem, mem_size) \
    ((uint8_t *)(cpr_mem) + ((mem_size) - SIZE_OF_REDZONE))




#define GET_USER_MEM_PTR(cpr_mem) \
    ((void *)((uint8_t *)(cpr_mem) + offsetof(cpr_mem_t, mem)))


#define CPR_CALLER_PC __builtin_return_address(0)









static chunk_t *tracking_chunk = NULL;


static cpr_mem_statistics_t memory_stats;
static cpr_mem_statistics_t saved_memory_stats;


static uint32_t poison_size = DEFAULT_POISON_SIZE;


static cpr_mem_tracking_t memory_tracking;


static cprMutex_t mem_tracking_mutex;


static const char *cmd_prefix = "CPR memory";


static boolean high_mem_use_condition = FALSE;


static size_t private_size;








volatile cpr_mem_tracking_node_t *next_display_node;







extern pthread_mutex_t morecore_mutex;
extern pthread_mutex_t magic_init_mutex;







static INLINE void
cpr_peg_alloc_size(size_t size);
static INLINE void
cpr_record_memory_size(cpr_mem_t *cpr_mem, size_t size);
static INLINE size_t
cpr_get_memory_size(cpr_mem_t *cpr_mem);
static INLINE void
cpr_poison_memory(cpr_mem_t *cpr_mem, size_t size);
static void
cpr_check_redzone(cpr_mem_t *cpr_mem, size_t size);
static void
cpr_assign_mem_tracking_node(cpr_mem_t *cpr_mem, size_t size);
static void
cpr_release_mem_tracking_node(cpr_mem_tracking_node_t *node);
static INLINE cpr_mem_tracking_node_t *
cpr_get_mem_tracking_node(cpr_mem_t *cpr_mem);
static int32_t
cpr_debug_memory_cli(int32_t argc, const char *argv[]);
static void
cpr_debug_memory_cli_usage(void);
static boolean
cpr_set_memory_tracking(boolean debug, int32_t argc, const char *argv[]);
static INLINE void
cpr_enable_memory_tracking(void);
static INLINE void
cpr_disable_memory_tracking(void);
static boolean
cpr_set_poison_size(boolean debug, int32_t argc, const char *argv[]);
static void
cpr_clear_memory_tracking(void);
int32_t
cpr_clear_memory(cc_int32_t argc, const char *argv[]);
int32_t
cpr_show_memory(cc_int32_t argc, const char *argv[]);
static void
cpr_show_memory_usage(void);
static void
cpr_show_memory_config(void);
static void
cpr_show_memory_statistics(void);
static void
cpr_show_memory_tracking(void);
static void
cpr_dump_memory(uint32_t *memory, size_t size); 









#define FAULT_INSERTION_CONFIG ""






#define MEMORY_FAULT_INSERTION

#define PARSE_FAULT_INSERTION_CLI(token, debug, argc, argv)





















































boolean
cpr_memory_mgmt_pre_init (size_t size)
{
    const char *fname = "cpr_memory_mgmt_pre_init";

    private_size = size + (2*PRIVATE_SYS_MEM_GUARD_SIZE);

    if (chunk_init() != TRUE) {
        return FALSE;
    }

    
    memset(&memory_stats, 0, sizeof(cpr_mem_statistics_t));
    memset(&saved_memory_stats, 0, sizeof(cpr_mem_statistics_t));

    
    memory_tracking.enabled = FALSE;

    
    memory_tracking.max_size = DEFAULT_TRACKING_SIZE;

    
    mem_tracking_mutex = cprCreateMutex("Memory tracking");
    if (mem_tracking_mutex == NULL) {
        CPR_ERROR("%s: init mem_tracking_mutex failure\n", fname);
        return FALSE;
    }

    return TRUE;
}











boolean
cpr_memory_mgmt_post_init (void)
{
    
    
    

    return TRUE;
}









void
cpr_memory_mgmt_destroy (void)
{
    chunk_exit();

    
    (void) cprDestroyMutex(mem_tracking_mutex);
}













void *
cpr_malloc(size_t size)
{
    cpr_mem_t *cpr_mem;
    uint8_t *p;
    size_t mem_size;
    void *caller_pc;

    
    caller_pc = CPR_CALLER_PC;

    if (size == 0) {
        errno = ENOMEM;
        CPR_ERROR("ERROR: malloc %d bytes are not supported, pc=0x%x\n",
                  size, caller_pc);
        return NULL;
    }

    MEMORY_FAULT_INSERTION;

    
    size = MAX((size_t)MIN_MEMORY_SIZE, size);
    mem_size = size + SIZE_OF_MEMORY_HEADER + SIZE_OF_REDZONE;
    cpr_mem = MALLOC(mem_size);
    if (!cpr_mem) {
        errno = ENOMEM;
        CPR_ERROR("ERROR: malloc %d bytes failed, pc=0x%x\n", size, caller_pc);
        return NULL;
    }

    






    p = GET_REDZONE_PTR(cpr_mem, mem_size);
    WRITE_4BYTES_UNALIGNED(p, CPR_REDZONE);

    
    cpr_mem->pc = caller_pc;

    
    if (memory_tracking.enabled == TRUE) {
        
        cpr_assign_mem_tracking_node(cpr_mem, size);
    } else {
        
        cpr_record_memory_size(cpr_mem, size);
    }

    
    memory_stats.total_allocs++;
    memory_stats.mallocs++;
    memory_stats.in_use_count++;
    memory_stats.in_use_size += size;
    memory_stats.total_alloc_size += size;
    if (memory_stats.max_use_size < memory_stats.in_use_size) {
        memory_stats.max_use_size = memory_stats.in_use_size;
    }
    cpr_peg_alloc_size(size);

    
    if (memory_stats.in_use_size > MEM_HIGH_WATER_MARK) {
        high_mem_use_condition = TRUE;
    }

    return GET_USER_MEM_PTR(cpr_mem);
}















void *
cpr_calloc (size_t num, size_t size)
{
    cpr_mem_t *cpr_mem;
    uint8_t *p;
    size_t mem_size;
    void *caller_pc;

    
    caller_pc = CPR_CALLER_PC;

    if (size == 0 || num == 0) {
        CPR_ERROR("ERROR: calloc num %d of size %d bytes are not supported,"
                  " pc=0x%x\n", num, size, caller_pc);
        return NULL;
    }

    MEMORY_FAULT_INSERTION;

    
    if (num != 1) {
        


        size = size * num;
    }
    size = MAX((size_t)MIN_MEMORY_SIZE, size);
    mem_size = size + SIZE_OF_MEMORY_HEADER + SIZE_OF_REDZONE;

    cpr_mem = CALLOC(1, mem_size);
    if (!cpr_mem) {
        CPR_ERROR("ERROR: calloc num %d of size %d bytes failed, pc=0x%x\n", 
                  num, size, caller_pc);
        return NULL;
    }

    






    p = GET_REDZONE_PTR(cpr_mem, mem_size);
    WRITE_4BYTES_UNALIGNED(p, CPR_REDZONE);

    
    cpr_mem->pc = caller_pc;

    
    if (memory_tracking.enabled == TRUE) {
        
        cpr_assign_mem_tracking_node(cpr_mem, size);
    } else {
        
        cpr_record_memory_size(cpr_mem, size);
    }

    
    memory_stats.total_allocs++;
    memory_stats.callocs++;
    memory_stats.in_use_count++;
    memory_stats.in_use_size += size;
    memory_stats.total_alloc_size += size;
    if (memory_stats.max_use_size < memory_stats.in_use_size) {
        memory_stats.max_use_size = memory_stats.in_use_size;
    }
    cpr_peg_alloc_size(size);

    
    if (memory_stats.in_use_size > MEM_HIGH_WATER_MARK) {
        high_mem_use_condition = TRUE;
    }

    return GET_USER_MEM_PTR(cpr_mem);
}












void *
cpr_realloc (void *mem, size_t size)
{
    cpr_mem_t *cpr_mem;
    size_t mem_size;
    size_t prev_size;
    cpr_mem_t *realloc_mem;
    uint32_t *p;
    void *caller_pc;

    
    caller_pc = CPR_CALLER_PC;

    
    if (mem == NULL) {
        return NULL;
    }
    if (size == 0) {
        
        memory_stats.total_allocs++;
        memory_stats.reallocs++;
        memory_stats.frees--;
        cpr_free(mem);
        return NULL;
    }

    MEMORY_FAULT_INSERTION;

    
    cpr_mem = (cpr_mem_t *)((uint8_t *)mem - SIZE_OF_MEMORY_HEADER);

    
    if (cpr_mem->pc == (void *) CPR_POISON) {
        CPR_ERROR("ERROR: attempt to realloc free memory, 0x%x\n", mem);
        return NULL;
    }

    
    prev_size = cpr_get_memory_size(cpr_mem);

    
    size = MAX((size_t)MIN_MEMORY_SIZE, size);
    mem_size = size + SIZE_OF_MEMORY_HEADER + SIZE_OF_REDZONE;

    
    realloc_mem = REALLOC(cpr_mem, mem_size);
    if (realloc_mem == NULL) {
        CPR_ERROR("ERROR: realloc %d bytes failed, pc=0x%x\n", size, caller_pc);
        return NULL;
    }

    
    p = GET_UINT32_PTR(realloc_mem, mem_size - SIZE_OF_REDZONE);
    WRITE_4BYTES_UNALIGNED(p, CPR_REDZONE);

    
    realloc_mem->pc = caller_pc;

    
    if (memory_tracking.enabled == TRUE) {
        cpr_mem_tracking_node_t *node;

        
        node = (cpr_mem_tracking_node_t *)cpr_get_mem_tracking_node(realloc_mem);

        if (node && (node->magic == CPR_TRACKING_MAGIC)) {
            
            node->pc = realloc_mem->pc;
            node->size = size;
            node->timestamp = time(NULL);
        } else {
            
            cpr_assign_mem_tracking_node(realloc_mem, size);
        }
    } else {
        
        cpr_record_memory_size(realloc_mem, size);
    }

    
    memory_stats.total_allocs++;
    memory_stats.reallocs++;
    memory_stats.in_use_size += size - prev_size;
    memory_stats.total_alloc_size += size;
    cpr_peg_alloc_size(size);
    if (memory_stats.max_use_size < memory_stats.in_use_size) {
        memory_stats.max_use_size = memory_stats.in_use_size;
    }

    
    if (memory_stats.in_use_size > MEM_HIGH_WATER_MARK) {
        high_mem_use_condition = TRUE;
    }

    return GET_USER_MEM_PTR(realloc_mem);
}









void
cpr_free (void *mem)
{
    cpr_mem_tracking_node_t *node;
    cpr_mem_t *cpr_mem;
    size_t size = 0;

    if (!mem) {
        return;
    }

    
    cpr_mem = (cpr_mem_t *)((uint8_t *)mem - SIZE_OF_MEMORY_HEADER);

    
    if (cpr_mem->pc == (void *) CPR_POISON) {
        CPR_ERROR("ERROR: attempt to double free memory, 0x%x\n", mem);
        return;
    }

    
    node = (cpr_mem_tracking_node_t *)cpr_get_mem_tracking_node(cpr_mem);
    if (!node) {
        

        
        size = cpr_get_memory_size(cpr_mem);

        cpr_check_redzone(cpr_mem, size);
    } else if (memory_tracking.enabled == TRUE) {
        

        if (node->magic == CPR_TRACKING_MAGIC) {
            size = node->size;
            cpr_check_redzone(cpr_mem, size);

            
            cpr_release_mem_tracking_node(node);
        }
    } else if (node->magic == CPR_TRACKING_MAGIC) {
        
        CPR_ERROR("Unexpected tracking node found while off, 0x%x\n", node);
#ifdef HELP
        if (chunk_free(tracking_chunk, node) == FALSE) {
            CPR_ERROR("Memory tracking is broken\n");
        }
#endif
    }
    





    
    cpr_poison_memory(cpr_mem, size);

    
    FREE(cpr_mem);

    
    memory_stats.frees++;
    memory_stats.in_use_count--;
    memory_stats.in_use_size -= size;

    
    if (memory_stats.in_use_size < MEM_LOW_WATER_MARK) {
        high_mem_use_condition = FALSE;
    }

    return;
}














void *
cpr_linux_sbrk (int incr)
{
    return ((void *)-1);
}








void
cpr_crashdump (void)
{
    *(volatile int *) 0xdeadbeef = 0x12345678;
}









boolean
cpr_mem_high_water_mark (void)
{
    return high_mem_use_condition;
}









static INLINE void
cpr_peg_alloc_size (size_t size)
{
    if (size < 64) {
        memory_stats.allocs_1_to_63++;
    } else if (size < 256) {
        memory_stats.allocs_64_to_255++;
    } else if (size < 1024) {
        memory_stats.allocs_256_to_1k++;
    } else if (size < 4096) {
        memory_stats.allocs_1k_to_4k++;
    } else if (size < 32768) {
        memory_stats.allocs_4k_to_32k++;
    } else {
        memory_stats.allocs_32k_plus++;
    }
}

















static void
cpr_record_memory_size (cpr_mem_t *cpr_mem, size_t size)
{
    cpr_assert_debug(cpr_mem != NULL);
    cpr_assert_debug((size >> 31) == 0);

    cpr_mem->u.size = size << 1 | 0x1;
}













static INLINE size_t
cpr_get_memory_size (cpr_mem_t *cpr_mem)
{
    size_t size;

    cpr_assert_debug(cpr_mem != NULL);

    if (cpr_mem->u.size & 0x1) {
        size = cpr_mem->u.size >> 1;
    } else {
        size = cpr_mem->u.node->size;
    }
    return size;
}


















static void
cpr_assign_mem_tracking_node (cpr_mem_t *cpr_mem, size_t size)
{
    cpr_mem_tracking_node_t *node;

    cpr_assert_debug(memory_tracking.enabled == TRUE);
    cpr_assert_debug(cpr_mem != NULL);
    cpr_assert_debug(cpr_mem->pc != NULL);
    cpr_assert_debug(size != 0);

    node = chunk_malloc(tracking_chunk);
    if (!node) {
        
        if (!(memory_tracking.flags & MEM_TRACKING_FLAG_OOC)) {
            memory_tracking.flags |= MEM_TRACKING_FLAG_OOC;
            CPR_ERROR("WARNING: Out-of-chunks for memory tracking!\n");
        }
        memory_tracking.missing_allocs++;

        
        cpr_record_memory_size(cpr_mem, size);
        return;
    }

    
    node->mem       = cpr_mem;
    node->pc        = cpr_mem->pc;
    node->size      = size;
    node->timestamp = time(NULL);
    node->magic     = CPR_TRACKING_MAGIC;

    
    (void) cprGetMutex(mem_tracking_mutex);

    
    node->next = NULL;
    node->prev = memory_tracking.tail;
    if (memory_tracking.tail != NULL) {
        memory_tracking.tail->next = node;
    } else {
        memory_tracking.head = node;
    }
    memory_tracking.tail = node;

    
    (void) cprReleaseMutex(mem_tracking_mutex);

    
    cpr_mem->u.node = node;
}














static void
cpr_release_mem_tracking_node (cpr_mem_tracking_node_t *node)
{
    static const char *fname = "cpr_release_mem_tracking_node";

    cpr_assert_debug(node != NULL);

    
    (void) cprGetMutex(mem_tracking_mutex);

    
    if (node == next_display_node) {
        



        next_display_node = node->next;

        
        CPR_INFO("%s: Correcting node being displayed\n", fname);
    }

    
    if (node->next) {
        node->next->prev = node->prev;
    } else if (node == memory_tracking.tail) {
        memory_tracking.tail = node->prev;
    }

    if (node->prev) {
        node->prev->next = node->next;
    } else if (node == memory_tracking.head) {
        memory_tracking.head = node->next;
    }
    node->next = NULL;
    node->prev = NULL;

    
    (void) cprReleaseMutex(mem_tracking_mutex);

    
    if (chunk_free(tracking_chunk, node) == FALSE) {
        
        if (!(memory_tracking.flags & MEM_TRACKING_FLAG_FREE_FAILED)) {
            CPR_ERROR("Error: Memory tracking is broken\n");
            memory_tracking.flags |= MEM_TRACKING_FLAG_FREE_FAILED;
        }
    }
}










static INLINE cpr_mem_tracking_node_t *
cpr_get_mem_tracking_node (cpr_mem_t *cpr_mem)
{
    cpr_assert_debug(cpr_mem != NULL);

    if ((cpr_mem->u.size & 0x1) == 0) {
        return cpr_mem->u.node;
    }
    return NULL;
}




















static void
cpr_check_redzone (cpr_mem_t *cpr_mem, size_t size)
{
    uint32_t redzone;
    uint32_t *rz;

    cpr_assert_debug(cpr_mem != NULL);
    cpr_assert_debug(size > 0);

    
    rz = (uint32_t *)((uint8_t *)cpr_mem + offsetof(cpr_mem_t, mem) + size);

    
    redzone = READ_4BYTES_UNALIGNED(rz, redzone);

    
    if (redzone != CPR_REDZONE) {
        
        memory_stats.redzone_pc = cpr_mem->pc;
        memory_stats.redzone_size = size;
        memcpy(memory_stats.redzone_memory, (uint8_t *)cpr_mem +
               offsetof(cpr_mem_t, mem) + size - REDZONE_RECORD_SIZE/2,
               REDZONE_RECORD_SIZE);

        CPR_ERROR("Red-zone failure: caller_pc = %x, size = %d, rz = %x\n",
                  cpr_mem->pc, size, redzone);
        memory_stats.redzone_violations++;
    }
}























static void
cpr_poison_memory (cpr_mem_t *cpr_mem, size_t size)
{
    uint32_t *p;
    uint32_t *end;
    const char *fname = "cpr_poison_memory";

    cpr_assert_debug(cpr_mem != NULL);
    cpr_assert_debug((size >> 31) == 0);
    


    if ((cpr_mem != NULL) && ((size >> 31) == 0)) {
        cpr_assert_debug(((cpr_mem->u.size bitand 0x1) == 0) or
                ((cpr_mem->u.size >> 1) >= size));

        
        WRITE_4BYTES_MEM_ALIGNED(&cpr_mem->u.node, 0);
        WRITE_4BYTES_MEM_ALIGNED(&cpr_mem->pc, CPR_POISON);

        
        p = (uint32_t *) GET_USER_MEM_PTR(cpr_mem);

        
        end = (uint32_t *)((uint8_t *)p + MIN(poison_size, size));

        
        while (p < end) {
            WRITE_4BYTES_MEM_ALIGNED(p, CPR_POISON);
            p++;
        }
    } else {
        CPR_INFO("%s: Parameters passed in are wrong. cpr_mem: 0x%x, size: %d.\n", fname, cpr_mem, size);
    }
}






static void
cpr_show_memory_statistics (void)
{
    debugif_printf("SIP memory statistics:\n");
    debugif_printf("\tTotal memory allocated  : %u\n",
            memory_stats.total_alloc_size);
    debugif_printf("\tMaximum memory allocated: %u\n",
            memory_stats.max_use_size);
    debugif_printf("\tTotal memory allocations: %u\n",
            memory_stats.total_allocs);
    debugif_printf("\t 1 to 63 bytes   : %6u\n",
            memory_stats.allocs_1_to_63);
    debugif_printf("\t 64 to 255 bytes : %6u\n",
            memory_stats.allocs_64_to_255);
    debugif_printf("\t 256 to 1k bytes : %6u\n",
            memory_stats.allocs_256_to_1k);
    debugif_printf("\t 1k to 4k bytes  : %6u\n",
            memory_stats.allocs_1k_to_4k);
    debugif_printf("\t 4k to 32k bytes : %6u\n",
            memory_stats.allocs_4k_to_32k);
    debugif_printf("\t 32k plus bytes  : %6u\n",
            memory_stats.allocs_32k_plus);
    debugif_printf("\t Malloc : %6u\n",
            memory_stats.mallocs);
    debugif_printf("\t Calloc : %6u\n",
            memory_stats.callocs);
    debugif_printf("\t Realloc: %6u\n",
            memory_stats.reallocs);
    debugif_printf("\t Free   : %6u\n",
            memory_stats.frees);
    debugif_printf("\tIn-use count: %5u\n",
            memory_stats.in_use_count);
    debugif_printf("\tIn-use size: %6u\n",
            memory_stats.in_use_size);
    debugif_printf("\tRedzone violations: %u\n\n",
            memory_stats.redzone_violations);
    if (memory_stats.redzone_violations) {
        debugif_printf("Last Redzone Violation:\n");
        debugif_printf("\tSize of memory = %u\n", memory_stats.redzone_size);
        debugif_printf("\tAllocator PC = 0x%x\n", memory_stats.redzone_pc);
        cpr_dump_memory((uint32_t *)memory_stats.redzone_memory,
                        REDZONE_RECORD_SIZE);
    }
}














static void
cpr_dump_memory (uint32_t *memory, size_t size)
{
    uint16_t i;
    uint16_t idx;

    






    size = (size >> 2) + ((size & 3) ? 1 : 0);

    for (i = 0; i < size; i++) {
        idx = i & 3;
        if (!idx) {
            debugif_printf("\n\t%x: ", memory);
            if (i != 0) {
                memory += 4;
            }
        }
        debugif_printf("%08x  ", memory[idx]);
    }
    debugif_printf("\n");
}





















static void
cpr_show_memory_tracking (void)
{
    cpr_mem_tracking_node_t node;
    cpr_mem_tracking_node_t *display_node;
    cpr_time_t timestamp;

    if (memory_tracking.enabled == FALSE) {
        debugif_printf("CPR memory tracking not enabled\n");
        return;
    }

    
    timestamp = time(NULL);

    debugif_printf("CPR memory tracking size = %u, missed allocations = %u\n",
                   memory_tracking.max_size, memory_tracking.missing_allocs);
    if (memory_tracking.flags & MEM_TRACKING_FLAG_FREE_FAILED) {
        debugif_printf("   problems occurred with freeing tracking nodes\n\n");
    }
    debugif_printf("__Memory__ TimeDelta  __Size__ ____PC____\n");

    if (memory_tracking.head) {
        
        (void) cprGetMutex(mem_tracking_mutex);

        
        next_display_node = memory_tracking.head;

        
        (void) cprReleaseMutex(mem_tracking_mutex);
    } else {
        debugif_printf("Currently no memory allocation being tracked\n");
        return;
    }

    while ((memory_tracking.enabled == TRUE) && 
           (next_display_node != NULL)) {
        
        (void) cprGetMutex(mem_tracking_mutex);

        
        display_node = (cpr_mem_tracking_node_t *) next_display_node;

        
        memcpy(&node, display_node, sizeof(cpr_mem_tracking_node_t));
        

        
        next_display_node = display_node->next;

        
        (void) cprReleaseMutex(mem_tracking_mutex);

        
        debugif_printf("0x%08x %10ld %8u 0x%08x: %s:!!!\n", node.mem,
                       timestamp - node.timestamp,
                       node.size, node.pc, (node.mem + SIZE_OF_MEMORY_HEADER));
    }

    
    next_display_node = NULL;
}










int32_t
cpr_show_memory (cc_int32_t argc, const char *argv[])
{
#ifdef DISABLE_MEMORY_ENHANCEMENTS
    debugif_printf("show cpr-memory commands are disabled\n");
#else
    uint8_t i = 0;

    
    if (argc && strcasecmp("show", argv[i]) == 0) {
        argc--;
        i++;
    }
    if (argc && strcasecmp("cpr-memory", argv[i]) == 0) {
        argc--;
        i++;
    }

    
    if (argc == 0) {
        cpr_show_memory_usage();
    }
    else if ((strcasecmp(argv[i], "statistics") == 0) ||
             (strcasecmp(argv[i], "stats") == 0)) {
        cpr_show_memory_statistics();
    }
    else if ((strcasecmp(argv[i], "config") == 0) ||
             (strcasecmp(argv[i], "configuration") == 0)) {
        cpr_show_memory_config();
    }
    else if (strcasecmp(argv[i], "tracking") == 0) {
        cpr_show_memory_tracking();
    }
    else {
        cpr_show_memory_usage();
        




        return 0;
    }
#endif
    return 0;
}






static void
cpr_show_memory_usage (void)
{
    debugif_printf("\nshow cpr-memory [options]:\n");
    debugif_printf("\tconfig    \t- CPR memory configuration settings\n");
    debugif_printf("\tstatistics\t- CPR memory statistics\n");
    debugif_printf("\ttracking  \t- CPR memory tracked allocations\n\n");
}










static void
cpr_clear_memory_tracking (void)
{
    if (memory_tracking.enabled == TRUE) {
        cpr_disable_memory_tracking();
        cpr_enable_memory_tracking();
        debugif_printf("CPR memory tracking cleared/reset\n");
    } else {
        debugif_printf("CPR memory tracking is currently disabled\n");
    }
}








int32_t
cpr_clear_memory (cc_int32_t argc, const char *argv[])
{
#ifdef DISABLE_MEMORY_ENHANCEMENTS
    debugif_printf("clear cpr-memory commands are disabled\n");
#else
    uint8_t i = 0;

    
    if (argc && strcasecmp("clear", argv[i]) == 0) {
        argc--;
        i++;
    }
    if (argc && strcasecmp("cpr-memory", argv[i]) == 0) {
        argc--;
        i++;
    }

    
    if (argc == 0) {
        debugif_printf("\nclear cpr-memory [options]:\n");
        debugif_printf("\tstatistics\t- clear cpr-memory statistics\n");
        debugif_printf("\ttracking  \t- clear cpr-memory tracking\n");
    }
    else if ((strcasecmp(argv[i], "statistics") == 0) ||
             (strcasecmp(argv[i], "stats") == 0)) {
        
        memcpy(&saved_memory_stats, &memory_stats,
               sizeof(cpr_mem_statistics_t));
        memset(&memory_stats, 0, sizeof(cpr_mem_statistics_t));
        debugif_printf("\ncleared cpr-memory statistics\n");
    }
    else if (strcasecmp(argv[i], "tracking") == 0) {
        cpr_clear_memory_tracking();
    }
    else {
        




        debugif_printf("\nNo match - no operation performed\n");
        debugif_printf("\nclear cpr-memory [options]:\n");
        debugif_printf("\tstatistics\t- clear cpr-memory statistics\n");
        debugif_printf("\ttracking  \t- clear cpr-memory tracking\n");
        return 0;
    }
#endif
    return 0;
}
















static int32_t
cpr_debug_memory_cli (int32_t argc, const char *argv[])
{
    boolean debug;
    int32_t rc = 0;

    


    if (argc <= 2) {
        debugif_printf("\nMissing required args\n");
        debugif_printf("debug cpr-memory [options]:\n");
        debugif_printf("\ttracking - configuration of CPR memory tracking\n");
        debugif_printf("\tpoison-size - the number of bytes poisoned when"
                       " memory freed\n");
        




        return 0;
    }

    


    if (strcasecmp(argv[0], "debug") == 0)
    {
        debug = TRUE;
    }
    else if (strcasecmp(argv[0], "undebug") == 0)
    {
        debug = FALSE;
    }
    else
    {
        debugif_printf("Unknown prefix (%s) not 'debug' or 'undebug'\n");
        return 1;
    }

    


    if (strcasecmp(argv[2], "tracking") == 0) {
        rc = cpr_set_memory_tracking(debug, argc - 3, &argv[3]);
    }
    else if (strcasecmp(argv[2], "poison-size") == 0) {
        rc = cpr_set_poison_size(debug, argc - 3, &argv[3]);
    }
    
    PARSE_FAULT_INSERTION_CLI(argv[2], debug, argc - 3, &argv[3])
    else {
        debugif_printf("Error: Unable to parse command at token: %s\n", argv[2]);
        cpr_debug_memory_cli_usage();
        rc = 1;
    }

    return (int32_t) rc;
}






static void
cpr_debug_memory_cli_usage (void)
{
    debugif_printf("debug cpr-memory [options]:\n");
    debugif_printf("\ttracking [options] - configuration of"
                   " CPR memory tracking\n");
    debugif_printf("\tpoison-size        - the number of bytes"
                   " poisoned when memory freed\n");
}


















static boolean
cpr_set_memory_tracking (boolean debug, int32_t argc, const char *argv[])
{
    const char *cmd = "tracking";
    uint32_t size;
    boolean rc = 0;

    if (argc == 0) {
        if (debug == TRUE) {
            cpr_enable_memory_tracking();
        } else {
            cpr_disable_memory_tracking();
        }
        debugif_printf("%s %s %s\n", cmd_prefix, cmd,
                       (memory_tracking.enabled == TRUE) ?
                       "enabled" : "disabled");
    } else if (strcasecmp(argv[0], "?") == 0) {
        debugif_printf("debug cpr-memory tracking [size [0-%d]]\n",
                       MAX_TRACKING_SIZE);
    } else if (strcasecmp(argv[0], "size") == 0) {
        if ((argc == 1) || (debug == FALSE)) {
            memory_tracking.max_size = DEFAULT_TRACKING_SIZE;
            debugif_printf("%s %s size restored to default of %d\n",
                           cmd_prefix, cmd, memory_tracking.max_size);
        } else {
            size = atoi(argv[1]);

            if (size > MAX_TRACKING_SIZE) {
                debugif_printf("ERROR: %d is out-of-range, 0 - %d\n",
                               size, MAX_TRACKING_SIZE);
                rc = 1;
            } else {
                memory_tracking.max_size = (uint16_t) size;
            }
            debugif_printf("%s %s size set to %d\n", cmd_prefix,
                           cmd, memory_tracking.max_size);
        }
    } else {
        debugif_printf("Error: unabled to parse command at token, %s\n", argv[3]);
        debugif_printf("debug cpr-memory tracking [size [0-%d]]\n",
                       MAX_TRACKING_SIZE);
        rc = 1;
    }

    return rc;
}






static INLINE void
cpr_enable_memory_tracking (void)
{
    if (!tracking_chunk) {
        tracking_chunk = chunk_create(sizeof(cpr_mem_tracking_node_t),
                                             memory_tracking.max_size,
                                             CHUNK_FLAGS_NONE, 0,
                                             "CPR Memory Tracking");
        if (!tracking_chunk) {
            debugif_printf("Error: unable to initiate tracking of memory\n");
        } else {
            memory_tracking.enabled = TRUE;
            memory_tracking.timestamp = time(NULL);
        }
    } else if (memory_tracking.enabled == FALSE) {
        debugif_printf("Warning: memory_tracking not enabled,"
                       " but tracking chunk present\n");
        memory_tracking.enabled = TRUE;
        memory_tracking.timestamp = time(NULL);
    }
}







static INLINE void
cpr_disable_memory_tracking (void)
{
    memory_tracking.enabled = FALSE;
    memory_tracking.head = NULL;
    memory_tracking.tail = NULL;
    memory_tracking.real_size = 0;
    memory_tracking.flags &= ~(MEM_TRACKING_FLAG_OOC |
                               MEM_TRACKING_FLAG_FREE_FAILED);
    memory_tracking.missing_allocs = 0;

    if (tracking_chunk) {
         if (chunk_destroy_forced(tracking_chunk) == FALSE) {
             debugif_printf("Error: unable to release tracking chunk\n");
         } else {
             tracking_chunk = NULL;
         }
    }
}


















static boolean
cpr_set_poison_size (boolean debug, int32_t argc, const char **argv)
{
    const char *cmd = "poison size";
    boolean rc = 0;

    if (debug == FALSE) {
        
        poison_size = DEFAULT_POISON_SIZE;
        debugif_printf("%s %s reset to default of %d\n", cmd_prefix, cmd,
                       poison_size);
    } else if (argc) {
        
        uint32_t size = atoi(argv[0]);

        if (size > MAX_POISON_SIZE) {
            debugif_printf("ERROR: %d is out-of-range, 0 - %d\n",
                               size, MAX_POISON_SIZE);
            rc = 1;
        } else {
            poison_size = size;
        }
        debugif_printf("%s %s set to %d\n", cmd_prefix, cmd, poison_size);
    } else {
        
        debugif_printf("%s %s set to %d\n", cmd_prefix, cmd, poison_size);
    }

    return rc;
}
 



void debugCprMem(cc_debug_cpr_mem_options_e category, cc_debug_flag_e flag)
{
    


    static const char *debugArgs[NUM_OF_CPR_SHOW_ARGUMENTS];

    if (flag == CC_DEBUG_DISABLE) {
        debugArgs[0] = "undebug";
    } else if (flag == CC_DEBUG_ENABLE) {
        debugArgs[0] = "debug";
    } else {
        debugif_printf("Incorrect Arguments. The \"flag\" can be CC_DEBUG_DISABLE/ENABLE\n");
    }
    debugArgs[1] = "cpr-memory";

    if (category == CC_DEBUG_CPR_MEM_TRACKING) {
        debugArgs[2] = "tracking";
    } else if (category == CC_DEBUG_CPR_MEM_POISON) {
        debugArgs[2] = "poison-size";
    } else {
        debugif_printf("Incorrect Arguments. The \"category\" can be CC_DEBUG_CPR_MEM_TRACKING/POISON\n");
    }
    cpr_debug_memory_cli(NUM_OF_CPR_SHOW_ARGUMENTS, debugArgs);
}


void debugClearCprMem(cc_debug_clear_cpr_options_e category)
{
    static const char *debugArgs[NUM_OF_CPR_SHOW_ARGUMENTS];

    debugArgs[0] = "clear";
    debugArgs[1] = "cpr-memory";

    if (category == CC_DEBUG_CLEAR_CPR_TRACKING) {
        debugArgs[2] = "tracking";
    } else if (category == CC_DEBUG_CLEAR_CPR_STATISTICS) {
        debugArgs[2] = "statistics";
    } else {
        debugif_printf("Incorrect Arguments. The \"category\" can be CC_DEBUG_CLEAR_CPR_TRACKING/STATISTICS\n");
    }
    cpr_clear_memory(NUM_OF_CPR_SHOW_ARGUMENTS, debugArgs);
}


void debugShowCprMem(cc_debug_show_cpr_options_e category)
{
    static const char *debugArgs[NUM_OF_CPR_SHOW_ARGUMENTS];

    debugArgs[0] = "show";
    debugArgs[1] = "cpr-memory";

    if (category == CC_DEBUG_SHOW_CPR_CONFIG) {
        debugArgs[2] = "config";
    } else if (category == CC_DEBUG_SHOW_CPR_HEAP_GUARD) {
        debugArgs[2] = "heap-guard";
    } else if (category == CC_DEBUG_SHOW_CPR_STATISTICS) {
        debugArgs[2] = "statistics";
    } else if (category == CC_DEBUG_SHOW_CPR_TRACKING) {
        debugArgs[2] = "tracking";
    } else {
        debugif_printf("Incorrect Arguments. The \"options\" can be one of show-cpr-options\n");
    }

    cpr_show_memory(NUM_OF_CPR_SHOW_ARGUMENTS, debugArgs);
}







static void
cpr_show_memory_config (void)
{
    debugif_printf("SIP memory management configuration:\n");
    debugif_printf("\tMemory tracking %s\n",
            (memory_tracking.enabled == TRUE) ? "on" : "off");
    debugif_printf("\tMemory tracking size: %d\n",
            memory_tracking.max_size);
    debugif_printf("\tPoison memory size: %d\n",
            poison_size);
    if (strlen(FAULT_INSERTION_CONFIG)) {
        debugif_printf("\t");
        debugif_printf(FAULT_INSERTION_CONFIG);
        debugif_printf("\n");
    }

    debugif_printf("\n");
}


