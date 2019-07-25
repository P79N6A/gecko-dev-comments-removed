
























#ifndef IA64_LIBUNWIND_I_H
#define IA64_LIBUNWIND_I_H




#include "elf64.h"
#include "mempool.h"

typedef struct
  {
    
  }
unw_tdep_frame_t;

enum ia64_pregnum
  {
    
    IA64_REG_PRI_UNAT_GR,
    IA64_REG_PRI_UNAT_MEM,

    
    IA64_REG_PSP,			
    
    IA64_REG_BSP,			
    IA64_REG_BSPSTORE,
    IA64_REG_PFS,			
    IA64_REG_RNAT,
    
    IA64_REG_IP,

    
    IA64_REG_R4, IA64_REG_R5, IA64_REG_R6, IA64_REG_R7,
    IA64_REG_NAT4, IA64_REG_NAT5, IA64_REG_NAT6, IA64_REG_NAT7,
    IA64_REG_UNAT, IA64_REG_PR, IA64_REG_LC, IA64_REG_FPSR,
    IA64_REG_B1, IA64_REG_B2, IA64_REG_B3, IA64_REG_B4, IA64_REG_B5,
    IA64_REG_F2, IA64_REG_F3, IA64_REG_F4, IA64_REG_F5,
    IA64_REG_F16, IA64_REG_F17, IA64_REG_F18, IA64_REG_F19,
    IA64_REG_F20, IA64_REG_F21, IA64_REG_F22, IA64_REG_F23,
    IA64_REG_F24, IA64_REG_F25, IA64_REG_F26, IA64_REG_F27,
    IA64_REG_F28, IA64_REG_F29, IA64_REG_F30, IA64_REG_F31,
    IA64_NUM_PREGS
  };

#ifdef UNW_LOCAL_ONLY

typedef unw_word_t ia64_loc_t;

#else 

typedef struct ia64_loc
  {
    unw_word_t w0, w1;
  }
ia64_loc_t;

#endif 

#include "script.h"

#define ABI_UNKNOWN			0
#define ABI_LINUX			1
#define ABI_HPUX			2
#define ABI_FREEBSD			3
#define ABI_OPENVMS			4
#define ABI_NSK				5	/* Tandem/HP Non-Stop Kernel */
#define ABI_WINDOWS			6

struct unw_addr_space
  {
    struct unw_accessors acc;
    int big_endian;
    int abi;	
    unw_caching_policy_t caching_policy;
#ifdef HAVE_ATOMIC_OPS_H
    AO_t cache_generation;
#else
    uint32_t cache_generation;
#endif
    unw_word_t dyn_generation;
    unw_word_t dyn_info_list_addr;	
#ifndef UNW_REMOTE_ONLY
    unsigned long long shared_object_removals;
#endif

    struct ia64_script_cache global_cache;
   };



#define ABI_MARKER_OLD_LINUX_SIGTRAMP	((0 << 8) | 's')
#define ABI_MARKER_OLD_LINUX_INTERRUPT	((0 << 8) | 'i')
#define ABI_MARKER_HP_UX_SIGTRAMP	((1 << 8) | 1)
#define ABI_MARKER_LINUX_SIGTRAMP	((3 << 8) | 's')
#define ABI_MARKER_LINUX_INTERRUPT	((3 << 8) | 'i')

struct cursor
  {
    void *as_arg;		
    unw_addr_space_t as;	

    


    unw_word_t ip;		
    unw_word_t cfm;		
    unw_word_t pr;		

    
    unw_word_t bsp;		
    unw_word_t sp;		
    unw_word_t psp;		
    ia64_loc_t cfm_loc;		
    ia64_loc_t ec_loc;		
    ia64_loc_t loc[IA64_NUM_PREGS];

    unw_word_t eh_args[4];	
    unw_word_t sigcontext_addr;	
    unw_word_t sigcontext_off;	

    short hint;
    short prev_script;

    uint8_t nat_bitnr[4];	
    uint16_t abi_marker;	
    uint16_t last_abi_marker;	
    uint8_t eh_valid_mask;

    unsigned int pi_valid :1;		
    unsigned int pi_is_dynamic :1; 
    unw_proc_info_t pi;		

    















    uint8_t rbs_curr;		
    uint8_t rbs_left_edge;	
    struct rbs_area
      {
	unw_word_t end;
	unw_word_t size;
	ia64_loc_t rnat_loc;
      }
    rbs_area[96 + 2];	
};

struct ia64_global_unwind_state
  {
    pthread_mutex_t lock;		

    volatile char needs_initialization;

    

    const unsigned char save_order[8];

    





    struct
      {
	unw_word_t  r0;			
	unw_fpreg_t f0;			
	unw_fpreg_t f1_le, f1_be;	
      }
    read_only;
    unw_fpreg_t nat_val_le, nat_val_be;
    unw_fpreg_t int_val_le, int_val_be;

    struct mempool reg_state_pool;
    struct mempool labeled_state_pool;

# if UNW_DEBUG
    const char *preg_name[IA64_NUM_PREGS];
# endif
  };

#define tdep_getcontext_trace           unw_getcontext
#define tdep_needs_initialization	unw.needs_initialization
#define tdep_init			UNW_OBJ(init)


#define tdep_search_unwind_table	unw_search_ia64_unwind_table
#define tdep_find_proc_info		UNW_OBJ(find_proc_info)
#define tdep_uc_addr			UNW_OBJ(uc_addr)
#define tdep_get_elf_image		UNW_ARCH_OBJ(get_elf_image)
#define tdep_access_reg			UNW_OBJ(access_reg)
#define tdep_access_fpreg		UNW_OBJ(access_fpreg)
#define tdep_fetch_frame(c,ip,n)	do {} while(0)
#define tdep_cache_frame(c,rs)		do {} while(0)
#define tdep_reuse_frame(c,rs)		do {} while(0)
#define tdep_stash_frame(c,rs)		do {} while(0)
#define tdep_trace(cur,addr,n)		(-UNW_ENOINFO)
#define tdep_get_as(c)			((c)->as)
#define tdep_get_as_arg(c)		((c)->as_arg)
#define tdep_get_ip(c)			((c)->ip)
#define tdep_big_endian(as)		((c)->as->big_endian != 0)

#ifndef UNW_LOCAL_ONLY
# define tdep_put_unwind_info		UNW_OBJ(put_unwind_info)
#endif





#define unw				UNW_OBJ(data)

extern void tdep_init (void);
extern int tdep_find_proc_info (unw_addr_space_t as, unw_word_t ip,
				unw_proc_info_t *pi, int need_unwind_info,
				void *arg);
extern void tdep_put_unwind_info (unw_addr_space_t as,
				  unw_proc_info_t *pi, void *arg);
extern void *tdep_uc_addr (ucontext_t *uc, unw_regnum_t regnum,
			   uint8_t *nat_bitnr);
extern int tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
			       unsigned long *segbase, unsigned long *mapoff,
			       char *path, size_t pathlen);
extern int tdep_access_reg (struct cursor *c, unw_regnum_t reg,
			    unw_word_t *valp, int write);
extern int tdep_access_fpreg (struct cursor *c, unw_regnum_t reg,
			      unw_fpreg_t *valp, int write);

extern struct ia64_global_unwind_state unw;




#define rbs_get_base(c,bspstore,rbs_basep)				\
	(*(rbs_basep) = (bspstore) - (((unw_word_t) 1) << 63), 0)

#endif 
