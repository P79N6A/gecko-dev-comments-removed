


























































typedef enum
  {
    UNW_DYN_STOP = 0,		
    UNW_DYN_SAVE_REG,		
    UNW_DYN_SPILL_FP_REL,	
    UNW_DYN_SPILL_SP_REL,	
    UNW_DYN_ADD,		
    UNW_DYN_POP_FRAMES,		
    UNW_DYN_LABEL_STATE,	
    UNW_DYN_COPY_STATE,		
    UNW_DYN_ALIAS		
  }
unw_dyn_operation_t;

typedef enum
  {
    UNW_INFO_FORMAT_DYNAMIC,		
    UNW_INFO_FORMAT_TABLE,		
    UNW_INFO_FORMAT_REMOTE_TABLE,	
    UNW_INFO_FORMAT_ARM_EXIDX		
  }
unw_dyn_info_format_t;

typedef struct unw_dyn_op
  {
    int8_t tag;				
    int8_t qp;				
    int16_t reg;			
    int32_t when;			
    unw_word_t val;			
  }
unw_dyn_op_t;

typedef struct unw_dyn_region_info
  {
    struct unw_dyn_region_info *next;	
    int32_t insn_count;			
    uint32_t op_count;			
    unw_dyn_op_t op[1];			
  }
unw_dyn_region_info_t;

typedef struct unw_dyn_proc_info
  {
    unw_word_t name_ptr;	
    unw_word_t handler;		
    uint32_t flags;
    int32_t pad0;
    unw_dyn_region_info_t *regions;
  }
unw_dyn_proc_info_t;

typedef struct unw_dyn_table_info
  {
    unw_word_t name_ptr;	
    unw_word_t segbase;		
    unw_word_t table_len;	
    unw_word_t *table_data;
  }
unw_dyn_table_info_t;

typedef struct unw_dyn_remote_table_info
  {
    unw_word_t name_ptr;	
    unw_word_t segbase;		
    unw_word_t table_len;	
    unw_word_t table_data;
  }
unw_dyn_remote_table_info_t;

typedef struct unw_dyn_info
  {
    
    struct unw_dyn_info *next;
    struct unw_dyn_info *prev;
    unw_word_t start_ip;	
    unw_word_t end_ip;		
    unw_word_t gp;		
    int32_t format;		
    int32_t pad;
    union
      {
	unw_dyn_proc_info_t pi;
	unw_dyn_table_info_t ti;
	unw_dyn_remote_table_info_t rti;
      }
    u;
  }
unw_dyn_info_t;

typedef struct unw_dyn_info_list
  {
    uint32_t version;
    uint32_t generation;
    unw_dyn_info_t *first;
  }
unw_dyn_info_list_t;



#define _U_dyn_region_info_size(op_count)				\
	((char *) (((unw_dyn_region_info_t *) NULL)->op + (op_count))	\
	 - (char *) NULL)



extern void _U_dyn_register (unw_dyn_info_t *);



extern void _U_dyn_cancel (unw_dyn_info_t *);




#define _U_dyn_op(_tag, _qp, _when, _reg, _val)				\
	((unw_dyn_op_t) { (_tag), (_qp), (_reg), (_when), (_val) })

#define _U_dyn_op_save_reg(op, qp, when, reg, dst)			\
	(*(op) = _U_dyn_op (UNW_DYN_SAVE_REG, (qp), (when), (reg), (dst)))

#define _U_dyn_op_spill_fp_rel(op, qp, when, reg, offset)		\
	(*(op) = _U_dyn_op (UNW_DYN_SPILL_FP_REL, (qp), (when), (reg),	\
			    (offset)))

#define _U_dyn_op_spill_sp_rel(op, qp, when, reg, offset)		\
	(*(op) = _U_dyn_op (UNW_DYN_SPILL_SP_REL, (qp), (when), (reg),	\
			    (offset)))

#define _U_dyn_op_add(op, qp, when, reg, value)				\
	(*(op) = _U_dyn_op (UNW_DYN_ADD, (qp), (when), (reg), (value)))

#define _U_dyn_op_pop_frames(op, qp, when, num_frames)			\
	(*(op) = _U_dyn_op (UNW_DYN_POP_FRAMES, (qp), (when), 0, (num_frames)))

#define _U_dyn_op_label_state(op, label)				\
	(*(op) = _U_dyn_op (UNW_DYN_LABEL_STATE, _U_QP_TRUE, -1, 0, (label)))

#define _U_dyn_op_copy_state(op, label)					\
	(*(op) = _U_dyn_op (UNW_DYN_COPY_STATE, _U_QP_TRUE, -1, 0, (label)))

#define _U_dyn_op_alias(op, qp, when, addr)				\
	(*(op) = _U_dyn_op (UNW_DYN_ALIAS, (qp), (when), 0, (addr)))

#define _U_dyn_op_stop(op)						\
	(*(op) = _U_dyn_op (UNW_DYN_STOP, _U_QP_TRUE, -1, 0, 0))




#define _U_QP_TRUE	_U_TDEP_QP_TRUE
