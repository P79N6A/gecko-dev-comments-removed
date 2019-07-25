
























#define IA64_LOG_UNW_CACHE_SIZE	7
#define IA64_UNW_CACHE_SIZE	(1 << IA64_LOG_UNW_CACHE_SIZE)

#define IA64_LOG_UNW_HASH_SIZE	(IA64_LOG_UNW_CACHE_SIZE + 1)
#define IA64_UNW_HASH_SIZE	(1 << IA64_LOG_UNW_HASH_SIZE)

typedef unsigned char unw_hash_index_t;

struct ia64_script_insn
  {
    unsigned int opc;		
    unsigned int dst;
    unw_word_t val;
  };




#define IA64_MAX_SCRIPT_LEN	(IA64_NUM_PREGS + 1)

struct ia64_script
  {
    unw_word_t ip;		
    unw_word_t pr_mask;		
    unw_word_t pr_val;		
    unw_proc_info_t pi;		
    unsigned short lru_chain;	
    unsigned short coll_chain;	
    unsigned short hint;	
    unsigned short count;	
    unsigned short abi_marker;
    struct ia64_script_insn insn[IA64_MAX_SCRIPT_LEN];
  };

struct ia64_script_cache
  {
#ifdef HAVE_ATOMIC_OPS_H
    AO_TS_t busy;		
#else
    pthread_mutex_t lock;
#endif
    unsigned short lru_head;	
    unsigned short lru_tail;	

    
    unsigned short hash[IA64_UNW_HASH_SIZE];

    uint32_t generation;	

    
    struct ia64_script buckets[IA64_UNW_CACHE_SIZE];
  };

#define ia64_cache_proc_info		UNW_OBJ(cache_proc_info)
#define ia64_get_cached_proc_info	UNW_OBJ(get_cached_proc_info)

struct cursor;			

extern int ia64_cache_proc_info (struct cursor *c);
extern int ia64_get_cached_proc_info (struct cursor *c);
