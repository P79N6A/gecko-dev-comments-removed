
























#ifndef dwarf_h
#define dwarf_h

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_ATOMIC_OPS_H
# include <atomic_ops.h>
#endif

#include <libunwind.h>

struct dwarf_cursor;	

#include "dwarf-config.h"
#ifndef UNW_REMOTE_ONLY

#endif
#include <pthread.h>



typedef enum
  {
    DW_OP_addr			= 0x03,
    DW_OP_deref			= 0x06,
    DW_OP_const1u		= 0x08,
    DW_OP_const1s		= 0x09,
    DW_OP_const2u		= 0x0a,
    DW_OP_const2s		= 0x0b,
    DW_OP_const4u		= 0x0c,
    DW_OP_const4s		= 0x0d,
    DW_OP_const8u		= 0x0e,
    DW_OP_const8s		= 0x0f,
    DW_OP_constu		= 0x10,
    DW_OP_consts		= 0x11,
    DW_OP_dup			= 0x12,
    DW_OP_drop			= 0x13,
    DW_OP_over			= 0x14,
    DW_OP_pick			= 0x15,
    DW_OP_swap			= 0x16,
    DW_OP_rot			= 0x17,
    DW_OP_xderef		= 0x18,
    DW_OP_abs			= 0x19,
    DW_OP_and			= 0x1a,
    DW_OP_div			= 0x1b,
    DW_OP_minus			= 0x1c,
    DW_OP_mod			= 0x1d,
    DW_OP_mul			= 0x1e,
    DW_OP_neg			= 0x1f,
    DW_OP_not			= 0x20,
    DW_OP_or			= 0x21,
    DW_OP_plus			= 0x22,
    DW_OP_plus_uconst		= 0x23,
    DW_OP_shl			= 0x24,
    DW_OP_shr			= 0x25,
    DW_OP_shra			= 0x26,
    DW_OP_xor			= 0x27,
    DW_OP_skip			= 0x2f,
    DW_OP_bra			= 0x28,
    DW_OP_eq			= 0x29,
    DW_OP_ge			= 0x2a,
    DW_OP_gt			= 0x2b,
    DW_OP_le			= 0x2c,
    DW_OP_lt			= 0x2d,
    DW_OP_ne			= 0x2e,
    DW_OP_lit0			= 0x30,
    DW_OP_lit1,  DW_OP_lit2,  DW_OP_lit3,  DW_OP_lit4,  DW_OP_lit5,
    DW_OP_lit6,  DW_OP_lit7,  DW_OP_lit8,  DW_OP_lit9,  DW_OP_lit10,
    DW_OP_lit11, DW_OP_lit12, DW_OP_lit13, DW_OP_lit14, DW_OP_lit15,
    DW_OP_lit16, DW_OP_lit17, DW_OP_lit18, DW_OP_lit19, DW_OP_lit20,
    DW_OP_lit21, DW_OP_lit22, DW_OP_lit23, DW_OP_lit24, DW_OP_lit25,
    DW_OP_lit26, DW_OP_lit27, DW_OP_lit28, DW_OP_lit29, DW_OP_lit30,
    DW_OP_lit31,
    DW_OP_reg0			= 0x50,
    DW_OP_reg1,  DW_OP_reg2,  DW_OP_reg3,  DW_OP_reg4,  DW_OP_reg5,
    DW_OP_reg6,  DW_OP_reg7,  DW_OP_reg8,  DW_OP_reg9,  DW_OP_reg10,
    DW_OP_reg11, DW_OP_reg12, DW_OP_reg13, DW_OP_reg14, DW_OP_reg15,
    DW_OP_reg16, DW_OP_reg17, DW_OP_reg18, DW_OP_reg19, DW_OP_reg20,
    DW_OP_reg21, DW_OP_reg22, DW_OP_reg23, DW_OP_reg24, DW_OP_reg25,
    DW_OP_reg26, DW_OP_reg27, DW_OP_reg28, DW_OP_reg29, DW_OP_reg30,
    DW_OP_reg31,
    DW_OP_breg0			= 0x70,
    DW_OP_breg1,  DW_OP_breg2,  DW_OP_breg3,  DW_OP_breg4,  DW_OP_breg5,
    DW_OP_breg6,  DW_OP_breg7,  DW_OP_breg8,  DW_OP_breg9,  DW_OP_breg10,
    DW_OP_breg11, DW_OP_breg12, DW_OP_breg13, DW_OP_breg14, DW_OP_breg15,
    DW_OP_breg16, DW_OP_breg17, DW_OP_breg18, DW_OP_breg19, DW_OP_breg20,
    DW_OP_breg21, DW_OP_breg22, DW_OP_breg23, DW_OP_breg24, DW_OP_breg25,
    DW_OP_breg26, DW_OP_breg27, DW_OP_breg28, DW_OP_breg29, DW_OP_breg30,
    DW_OP_breg31,
    DW_OP_regx			= 0x90,
    DW_OP_fbreg			= 0x91,
    DW_OP_bregx			= 0x92,
    DW_OP_piece			= 0x93,
    DW_OP_deref_size		= 0x94,
    DW_OP_xderef_size		= 0x95,
    DW_OP_nop			= 0x96,
    DW_OP_push_object_address	= 0x97,
    DW_OP_call2			= 0x98,
    DW_OP_call4			= 0x99,
    DW_OP_call_ref		= 0x9a,
    DW_OP_lo_user		= 0xe0,
    DW_OP_hi_user		= 0xff
  }
dwarf_expr_op_t;

#define DWARF_CIE_VERSION	3	/* GCC emits version 1??? */

#define DWARF_CFA_OPCODE_MASK	0xc0
#define DWARF_CFA_OPERAND_MASK	0x3f

typedef enum
  {
    DW_CFA_advance_loc		= 0x40,
    DW_CFA_offset		= 0x80,
    DW_CFA_restore		= 0xc0,
    DW_CFA_nop			= 0x00,
    DW_CFA_set_loc		= 0x01,
    DW_CFA_advance_loc1		= 0x02,
    DW_CFA_advance_loc2		= 0x03,
    DW_CFA_advance_loc4		= 0x04,
    DW_CFA_offset_extended	= 0x05,
    DW_CFA_restore_extended	= 0x06,
    DW_CFA_undefined		= 0x07,
    DW_CFA_same_value		= 0x08,
    DW_CFA_register		= 0x09,
    DW_CFA_remember_state	= 0x0a,
    DW_CFA_restore_state	= 0x0b,
    DW_CFA_def_cfa		= 0x0c,
    DW_CFA_def_cfa_register	= 0x0d,
    DW_CFA_def_cfa_offset	= 0x0e,
    DW_CFA_def_cfa_expression	= 0x0f,
    DW_CFA_expression		= 0x10,
    DW_CFA_offset_extended_sf	= 0x11,
    DW_CFA_def_cfa_sf		= 0x12,
    DW_CFA_def_cfa_offset_sf	= 0x13,
    DW_CFA_lo_user		= 0x1c,
    DW_CFA_MIPS_advance_loc8	= 0x1d,
    DW_CFA_GNU_window_save	= 0x2d,
    DW_CFA_GNU_args_size	= 0x2e,
    DW_CFA_GNU_negative_offset_extended	= 0x2f,
    DW_CFA_hi_user		= 0x3c
  }
dwarf_cfa_t;












#define DW_EH_PE_FORMAT_MASK	0x0f	/* format of the encoded value */
#define DW_EH_PE_APPL_MASK	0x70	/* how the value is to be applied */


#define DW_EH_PE_indirect	0x80


#define DW_EH_PE_omit		0xff
#define DW_EH_PE_ptr		0x00	/* pointer-sized unsigned value */
#define DW_EH_PE_uleb128	0x01	/* unsigned LE base-128 value */
#define DW_EH_PE_udata2		0x02	/* unsigned 16-bit value */
#define DW_EH_PE_udata4		0x03	/* unsigned 32-bit value */
#define DW_EH_PE_udata8		0x04	/* unsigned 64-bit value */
#define DW_EH_PE_sleb128	0x09	/* signed LE base-128 value */
#define DW_EH_PE_sdata2		0x0a	/* signed 16-bit value */
#define DW_EH_PE_sdata4		0x0b	/* signed 32-bit value */
#define DW_EH_PE_sdata8		0x0c	/* signed 64-bit value */


#define DW_EH_PE_absptr		0x00	/* absolute value */
#define DW_EH_PE_pcrel		0x10	/* rel. to addr. of encoded value */
#define DW_EH_PE_textrel	0x20	/* text-relative (GCC-specific???) */
#define DW_EH_PE_datarel	0x30	/* data-relative */



#define DW_EH_PE_funcrel	0x40	/* start-of-procedure-relative */
#define DW_EH_PE_aligned	0x50	/* aligned pointer */

extern struct mempool dwarf_reg_state_pool;
extern struct mempool dwarf_cie_info_pool;

typedef enum
  {
    DWARF_WHERE_UNDEF,		
    DWARF_WHERE_SAME,		
    DWARF_WHERE_CFAREL,		
    DWARF_WHERE_REG,		
    DWARF_WHERE_EXPR,		
  }
dwarf_where_t;

typedef struct
  {
    dwarf_where_t where;	
    unw_word_t val;		
  }
dwarf_save_loc_t;











#define DWARF_CFA_REG_COLUMN	DWARF_NUM_PRESERVED_REGS
#define DWARF_CFA_OFF_COLUMN	(DWARF_NUM_PRESERVED_REGS + 1)

typedef struct dwarf_reg_state
  {
    struct dwarf_reg_state *next;	
    dwarf_save_loc_t reg[DWARF_NUM_PRESERVED_REGS + 2];
    unw_word_t ip;		          
    unw_word_t ret_addr_column;           
    unsigned short lru_chain;	  
    unsigned short coll_chain;	
    unsigned short hint;	      
    unsigned short signal_frame; 
  }
dwarf_reg_state_t;

typedef struct dwarf_cie_info
  {
    unw_word_t cie_instr_start;	
    unw_word_t cie_instr_end;	
    unw_word_t fde_instr_start;	
    unw_word_t fde_instr_end;	
    unw_word_t code_align;	
    unw_word_t data_align;	
    unw_word_t ret_addr_column;	
    unw_word_t handler;		
    uint16_t abi;
    uint16_t tag;
    uint8_t fde_encoding;
    uint8_t lsda_encoding;
    unsigned int sized_augmentation : 1;
    unsigned int have_abi_marker : 1;
    unsigned int signal_frame : 1;
  }
dwarf_cie_info_t;

typedef struct dwarf_state_record
  {
    unsigned char fde_encoding;
    unw_word_t args_size;

    dwarf_reg_state_t rs_initial;	
    dwarf_reg_state_t rs_current;	
  }
dwarf_state_record_t;

typedef struct dwarf_cursor
  {
    void *as_arg;		
    unw_addr_space_t as;	

    unw_word_t cfa;	
    unw_word_t ip;		
    unw_word_t args_size;	
    unw_word_t ret_addr_column;	
    unw_word_t eh_args[UNW_TDEP_NUM_EH_REGS];
    unsigned int eh_valid_mask;

    dwarf_loc_t loc[DWARF_NUM_PRESERVED_REGS];

    unsigned int stash_frames :1; 
    unsigned int use_prev_instr :1; 
    unsigned int pi_valid :1;	
    unsigned int pi_is_dynamic :1; 
    unw_proc_info_t pi;		

    short hint; 
    short prev_rs;
  }
dwarf_cursor_t;

#define DWARF_LOG_UNW_CACHE_SIZE	7
#define DWARF_UNW_CACHE_SIZE	(1 << DWARF_LOG_UNW_CACHE_SIZE)

#define DWARF_LOG_UNW_HASH_SIZE	(DWARF_LOG_UNW_CACHE_SIZE + 1)
#define DWARF_UNW_HASH_SIZE	(1 << DWARF_LOG_UNW_HASH_SIZE)

typedef unsigned char unw_hash_index_t;

struct dwarf_rs_cache
  {
    pthread_mutex_t lock;
    unsigned short lru_head;	
    unsigned short lru_tail;	

    
    unsigned short hash[DWARF_UNW_HASH_SIZE];

    uint32_t generation;	

    
    dwarf_reg_state_t buckets[DWARF_UNW_CACHE_SIZE];
  };



struct unw_debug_frame_list
  {
    
    unw_word_t start;
    unw_word_t end;
    
    char *debug_frame;
    size_t debug_frame_size;
    
    struct table_entry *index;
    size_t index_size;
    
    struct unw_debug_frame_list *next;
  };

struct dwarf_callback_data
  {
    
    unw_word_t ip;		
    unw_proc_info_t *pi;	
    int need_unwind_info;
    
    int single_fde;		
    unw_dyn_info_t di;		
    unw_dyn_info_t di_debug;	
  };


#define dwarf_init			UNW_ARCH_OBJ (dwarf_init)
#define dwarf_callback			UNW_OBJ (dwarf_callback)
#define dwarf_find_proc_info		UNW_OBJ (dwarf_find_proc_info)
#define dwarf_find_debug_frame		UNW_OBJ (dwarf_find_debug_frame)
#define dwarf_search_unwind_table	UNW_OBJ (dwarf_search_unwind_table)
#define dwarf_put_unwind_info		UNW_OBJ (dwarf_put_unwind_info)
#define dwarf_put_unwind_info		UNW_OBJ (dwarf_put_unwind_info)
#define dwarf_eval_expr			UNW_OBJ (dwarf_eval_expr)
#define dwarf_extract_proc_info_from_fde \
		UNW_OBJ (dwarf_extract_proc_info_from_fde)
#define dwarf_find_save_locs		UNW_OBJ (dwarf_find_save_locs)
#define dwarf_create_state_record	UNW_OBJ (dwarf_create_state_record)
#define dwarf_make_proc_info		UNW_OBJ (dwarf_make_proc_info)
#define dwarf_read_encoded_pointer	UNW_OBJ (dwarf_read_encoded_pointer)
#define dwarf_step			UNW_OBJ (dwarf_step)

extern int dwarf_init (void);
#ifndef UNW_REMOTE_ONLY
struct dl_phdr_info;
extern int dwarf_callback (struct dl_phdr_info *info, size_t size, void *ptr);
extern int dwarf_find_proc_info (unw_addr_space_t as, unw_word_t ip,
				 unw_proc_info_t *pi,
				 int need_unwind_info, void *arg);
#endif 
extern int dwarf_find_debug_frame (int found, unw_dyn_info_t *di_debug,
				   unw_word_t ip, unw_word_t segbase,
				   const char* obj_name, unw_word_t start,
				   unw_word_t end);
extern int dwarf_search_unwind_table (unw_addr_space_t as,
				      unw_word_t ip,
				      unw_dyn_info_t *di,
				      unw_proc_info_t *pi,
				      int need_unwind_info, void *arg);
extern void dwarf_put_unwind_info (unw_addr_space_t as,
				   unw_proc_info_t *pi, void *arg);
extern int dwarf_eval_expr (struct dwarf_cursor *c, unw_word_t *addr,
			    unw_word_t len, unw_word_t *valp,
			    int *is_register);
extern int dwarf_extract_proc_info_from_fde (unw_addr_space_t as,
					     unw_accessors_t *a,
					     unw_word_t *fde_addr,
					     unw_proc_info_t *pi,
					     int need_unwind_info,
					     unw_word_t base,
					     void *arg);
extern int dwarf_find_save_locs (struct dwarf_cursor *c);
extern int dwarf_create_state_record (struct dwarf_cursor *c,
				      dwarf_state_record_t *sr);
extern int dwarf_make_proc_info (struct dwarf_cursor *c);
extern int dwarf_read_encoded_pointer (unw_addr_space_t as,
				       unw_accessors_t *a,
				       unw_word_t *addr,
				       unsigned char encoding,
				       const unw_proc_info_t *pi,
				       unw_word_t *valp, void *arg);
extern int dwarf_step (struct dwarf_cursor *c);

#endif 
