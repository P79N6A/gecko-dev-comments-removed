






































#ifndef __CPR_DARWIN_CHUNK_H__
#define __CPR_DARWIN_CHUNK_H__


#define CHUNK_ALIGN_BYTES 4





typedef struct chunk_s {
    struct chunk_s *next;          
    struct chunk_s *next_sibling;  
    struct chunk_s *prev_sibling;  
    union {
        struct chunk_s *head_chunk;
        struct chunk_s *tail;      
    } p;

    uint32_t    cfg_max;           
    uint32_t    real_max;          
    uint32_t    cfg_size;          
    uint32_t    real_size;         
    uint32_t    index;             
    uint32_t    total_inuse;       
    uint32_t    inuse_hwm;         
    uint32_t    total_free;        
    uint16_t    alignment;         
    uint16_t    overhead;          
    uint32_t    flags;             
    char        name[15];          
    uint8_t    *data;              
    uint8_t    *end;               
    void       *freelist[0];       
} chunk_t;




typedef struct free_chunk_s {
    uint32_t magic;
    void *last_deallocator;    
} free_chunk_t;




typedef struct chunk_header_s {
    chunk_t *root_chunk;
    union {
        uint8_t *data;          
        free_chunk_t free_info; 
    } data_area;
} chunk_header_t;

































#define FREECHUNKMAGIC          0xEF4321CD      /* Magic value for freechunkmagic */
#define CHUNK_POISON_PATTERN    0xb0d0b0d




#define CHUNK_MEMBLOCK_MAXSIZE  65536











#define CHUNK_FLAGS_NONE         0x00000000
#define CHUNK_FLAGS_DYNAMIC      0x00000001
#define CHUNK_FLAGS_SIBLING      0x00000002
#define CHUNK_FLAGS_INDEPENDENT  0x00000004
#define CHUNK_FLAGS_RESIDENT     0x00000008
#define CHUNK_FLAGS_UNZEROED     0x00000010
#define CHUNK_FLAGS_MALLOCFAILED 0x00000080










#define CHUNK_FLAGS_PRIVATE     0x00000200








#define CHUNK_FLAGS_DATA_HEADER (CHUNK_FLAGS_SMALLHEADER)








extern boolean chunk_init(void);
extern void chunk_exit(void);
extern chunk_t *chunk_create(uint32_t cfg_size, uint32_t cfg_max,
                             uint32_t flags, uint32_t alignment,
                             const char *name);
extern boolean chunk_destroy(chunk_t *chunk);
extern boolean chunk_destroy_forced(chunk_t *chunk);
extern void *chunk_malloc(chunk_t *chunk);
extern void *chunk_malloc_caller(chunk_t *chunk, uint32_t caller_pc);
extern boolean chunk_free(chunk_t *chunk, void *data);
extern boolean chunk_free_caller(chunk_t *chunk, void *data,
                                 uint32_t caller_pc);
extern boolean chunk_is_destroyable(chunk_t *chunk);
extern boolean chunk_did_malloc_fail(chunk_t *chunk);
extern uint32_t get_chunk_size(void *data);
extern long chunk_totalfree_count(chunk_t *chunk);

#endif
