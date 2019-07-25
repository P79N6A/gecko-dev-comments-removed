















#if !defined(_state_H)
# define _state_H (1)
# include "internal.h"
# include "huffman.h"
# include "quant.h"




typedef ptrdiff_t       oc_sb_map_quad[4];

typedef oc_sb_map_quad  oc_sb_map[4];

typedef ptrdiff_t       oc_mb_map_plane[4];

typedef oc_mb_map_plane oc_mb_map[3];

typedef ogg_int16_t     oc_mv;

typedef struct oc_sb_flags              oc_sb_flags;
typedef struct oc_border_info           oc_border_info;
typedef struct oc_fragment              oc_fragment;
typedef struct oc_fragment_plane        oc_fragment_plane;
typedef struct oc_base_opt_vtable       oc_base_opt_vtable;
typedef struct oc_base_opt_data         oc_base_opt_data;
typedef struct oc_state_dispatch_vtable oc_state_dispatch_vtable;
typedef struct oc_theora_state          oc_theora_state;




# if defined(OC_X86_ASM)
#  if defined(_MSC_VER)
#   include "x86_vc/x86int.h"
#  else
#   include "x86/x86int.h"
#  endif
# endif
# if defined(OC_ARM_ASM)
#  include "arm/armint.h"
# endif
# if defined(OC_C64X_ASM)
#  include "c64x/c64xint.h"
# endif

# if !defined(oc_state_accel_init)
#  define oc_state_accel_init oc_state_accel_init_c
# endif
# if defined(OC_STATE_USE_VTABLE)
#  if !defined(oc_frag_copy)
#   define oc_frag_copy(_state,_dst,_src,_ystride) \
  ((*(_state)->opt_vtable.frag_copy)(_dst,_src,_ystride))
#  endif
#  if !defined(oc_frag_copy_list)
#   define oc_frag_copy_list(_state,_dst_frame,_src_frame,_ystride, \
 _fragis,_nfragis,_frag_buf_offs) \
 ((*(_state)->opt_vtable.frag_copy_list)(_dst_frame,_src_frame,_ystride, \
  _fragis,_nfragis,_frag_buf_offs))
#  endif
#  if !defined(oc_frag_recon_intra)
#   define oc_frag_recon_intra(_state,_dst,_dst_ystride,_residue) \
  ((*(_state)->opt_vtable.frag_recon_intra)(_dst,_dst_ystride,_residue))
#  endif
#  if !defined(oc_frag_recon_inter)
#   define oc_frag_recon_inter(_state,_dst,_src,_ystride,_residue) \
  ((*(_state)->opt_vtable.frag_recon_inter)(_dst,_src,_ystride,_residue))
#  endif
#  if !defined(oc_frag_recon_inter2)
#   define oc_frag_recon_inter2(_state,_dst,_src1,_src2,_ystride,_residue) \
  ((*(_state)->opt_vtable.frag_recon_inter2)(_dst, \
   _src1,_src2,_ystride,_residue))
#  endif
# if !defined(oc_idct8x8)
#   define oc_idct8x8(_state,_y,_x,_last_zzi) \
  ((*(_state)->opt_vtable.idct8x8)(_y,_x,_last_zzi))
#  endif
#  if !defined(oc_state_frag_recon)
#   define oc_state_frag_recon(_state,_fragi, \
 _pli,_dct_coeffs,_last_zzi,_dc_quant) \
  ((*(_state)->opt_vtable.state_frag_recon)(_state,_fragi, \
   _pli,_dct_coeffs,_last_zzi,_dc_quant))
#  endif
#  if !defined(oc_loop_filter_init)
#   define oc_loop_filter_init(_state,_bv,_flimit) \
  ((*(_state)->opt_vtable.loop_filter_init)(_bv,_flimit))
#  endif
#  if !defined(oc_state_loop_filter_frag_rows)
#   define oc_state_loop_filter_frag_rows(_state, \
 _bv,_refi,_pli,_fragy0,_fragy_end) \
  ((*(_state)->opt_vtable.state_loop_filter_frag_rows)(_state, \
   _bv,_refi,_pli,_fragy0,_fragy_end))
#  endif
#  if !defined(oc_restore_fpu)
#   define oc_restore_fpu(_state) \
  ((*(_state)->opt_vtable.restore_fpu)())
#  endif
# else
#  if !defined(oc_frag_copy)
#   define oc_frag_copy(_state,_dst,_src,_ystride) \
  oc_frag_copy_c(_dst,_src,_ystride)
#  endif
#  if !defined(oc_frag_copy_list)
#   define oc_frag_copy_list(_state,_dst_frame,_src_frame,_ystride, \
 _fragis,_nfragis,_frag_buf_offs) \
  oc_frag_copy_list_c(_dst_frame,_src_frame,_ystride, \
  _fragis,_nfragis,_frag_buf_offs)
#  endif
#  if !defined(oc_frag_recon_intra)
#   define oc_frag_recon_intra(_state,_dst,_dst_ystride,_residue) \
  oc_frag_recon_intra_c(_dst,_dst_ystride,_residue)
#  endif
#  if !defined(oc_frag_recon_inter)
#   define oc_frag_recon_inter(_state,_dst,_src,_ystride,_residue) \
  oc_frag_recon_inter_c(_dst,_src,_ystride,_residue)
#  endif
#  if !defined(oc_frag_recon_inter2)
#   define oc_frag_recon_inter2(_state,_dst,_src1,_src2,_ystride,_residue) \
  oc_frag_recon_inter2_c(_dst,_src1,_src2,_ystride,_residue)
#  endif
#  if !defined(oc_idct8x8)
#   define oc_idct8x8(_state,_y,_x,_last_zzi) oc_idct8x8_c(_y,_x,_last_zzi)
#  endif
#  if !defined(oc_state_frag_recon)
#   define oc_state_frag_recon oc_state_frag_recon_c
#  endif
#  if !defined(oc_loop_filter_init)
#   define oc_loop_filter_init(_state,_bv,_flimit) \
  oc_loop_filter_init_c(_bv,_flimit)
#  endif
#  if !defined(oc_state_loop_filter_frag_rows)
#   define oc_state_loop_filter_frag_rows oc_state_loop_filter_frag_rows_c
#  endif
#  if !defined(oc_restore_fpu)
#   define oc_restore_fpu(_state) do{}while(0)
#  endif
# endif




# define OC_INTRA_FRAME (0)

# define OC_INTER_FRAME (1)

# define OC_UNKWN_FRAME (-1)





# define OC_UMV_PADDING (16)



# define OC_FRAME_GOLD      (0)

# define OC_FRAME_PREV      (1)

# define OC_FRAME_SELF      (2)

# define OC_FRAME_NONE      (3)


# define OC_FRAME_IO        (3)

# define OC_FRAME_GOLD_ORIG (4)

# define OC_FRAME_PREV_ORIG (5)



# define OC_MODE_INVALID        (-1)

# define OC_MODE_INTER_NOMV     (0)

# define OC_MODE_INTRA          (1)


# define OC_MODE_INTER_MV       (2)


# define OC_MODE_INTER_MV_LAST  (3)


# define OC_MODE_INTER_MV_LAST2 (4)


# define OC_MODE_GOLDEN_NOMV    (5)


# define OC_MODE_GOLDEN_MV      (6)


# define OC_MODE_INTER_MV_FOUR  (7)

# define OC_NMODES              (8)


# define OC_FRAME_FOR_MODE(_x) \
 OC_UNIBBLE_TABLE32(OC_FRAME_PREV,OC_FRAME_SELF,OC_FRAME_PREV,OC_FRAME_PREV, \
  OC_FRAME_PREV,OC_FRAME_GOLD,OC_FRAME_GOLD,OC_FRAME_PREV,(_x))




# define OC_PACKET_INFO_HDR    (-3)

# define OC_PACKET_COMMENT_HDR (-2)

# define OC_PACKET_SETUP_HDR   (-1)

# define OC_PACKET_DONE        (INT_MAX)



#define OC_MV(_x,_y)         ((oc_mv)((_x)&0xFF|(_y)<<8))
#define OC_MV_X(_mv)         ((signed char)(_mv))
#define OC_MV_Y(_mv)         ((_mv)>>8)
#define OC_MV_ADD(_mv1,_mv2) \
  OC_MV(OC_MV_X(_mv1)+OC_MV_X(_mv2), \
   OC_MV_Y(_mv1)+OC_MV_Y(_mv2))
#define OC_MV_SUB(_mv1,_mv2) \
  OC_MV(OC_MV_X(_mv1)-OC_MV_X(_mv2), \
   OC_MV_Y(_mv1)-OC_MV_Y(_mv2))

































struct oc_sb_flags{
  unsigned char coded_fully:1;
  unsigned char coded_partially:1;
  unsigned char quad_valid:4;
};






struct oc_border_info{
  

  ogg_int64_t mask;
  

  int         npixels;
};




struct oc_fragment{
  
  unsigned   coded:1;
  




  unsigned   invalid:1;
  
  unsigned   qii:4;
  
  unsigned   refi:2;
  
  unsigned   mb_mode:3;
  




  signed int borderi:5;
  


  signed int dc:16;
};




struct oc_fragment_plane{
  
  int       nhfrags;
  
  int       nvfrags;
  
  ptrdiff_t froffset;
  
  ptrdiff_t nfrags;
  
  unsigned  nhsbs;
  
  unsigned  nvsbs;
  
  unsigned  sboffset;
  
  unsigned  nsbs;
};


typedef void (*oc_state_loop_filter_frag_rows_func)(
 const oc_theora_state *_state,signed char _bv[256],int _refi,int _pli,
 int _fragy0,int _fragy_end);


struct oc_base_opt_vtable{
  void (*frag_copy)(unsigned char *_dst,
   const unsigned char *_src,int _ystride);
  void (*frag_copy_list)(unsigned char *_dst_frame,
   const unsigned char *_src_frame,int _ystride,
   const ptrdiff_t *_fragis,ptrdiff_t _nfragis,const ptrdiff_t *_frag_buf_offs);
  void (*frag_recon_intra)(unsigned char *_dst,int _ystride,
   const ogg_int16_t _residue[64]);
  void (*frag_recon_inter)(unsigned char *_dst,
   const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
  void (*frag_recon_inter2)(unsigned char *_dst,const unsigned char *_src1,
   const unsigned char *_src2,int _ystride,const ogg_int16_t _residue[64]);
  void (*idct8x8)(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi);
  void (*state_frag_recon)(const oc_theora_state *_state,ptrdiff_t _fragi,
   int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,ogg_uint16_t _dc_quant);
  void (*loop_filter_init)(signed char _bv[256],int _flimit);
  oc_state_loop_filter_frag_rows_func state_loop_filter_frag_rows;
  void (*restore_fpu)(void);
};



struct oc_base_opt_data{
  const unsigned char *dct_fzig_zag;
};



struct oc_theora_state{
  
  th_info             info;
# if defined(OC_STATE_USE_VTABLE)
  
  oc_base_opt_vtable  opt_vtable;
# endif
  
  oc_base_opt_data    opt_data;
  
  ogg_uint32_t        cpu_flags;
  
  oc_fragment_plane   fplanes[3];
  
  oc_fragment        *frags;
  

  ptrdiff_t          *frag_buf_offs;
  
  oc_mv              *frag_mvs;
  
  ptrdiff_t           nfrags;
  
  oc_sb_map          *sb_maps;
  
  oc_sb_flags        *sb_flags;
  
  unsigned            nsbs;
  



  oc_mb_map          *mb_maps;
  


  signed char        *mb_modes;
  
  unsigned            nhmbs;
  
  unsigned            nvmbs;
  
  size_t              nmbs;
  

  ptrdiff_t          *coded_fragis;
  
  ptrdiff_t           ncoded_fragis[3];
  
  ptrdiff_t           ntotal_coded_fragis;
  
  th_ycbcr_buffer     ref_frame_bufs[6];
  
  int                 ref_frame_idx[6];
  


  unsigned char      *ref_frame_data[6];
  
  unsigned char      *ref_frame_handle;
  
  int                 ref_ystride[3];
  
  int                 nborders;
  


  oc_border_info      borders[16];
  
  ogg_int64_t         keyframe_num;
  
  ogg_int64_t         curframe_num;
  
  ogg_int64_t         granpos;
  
  signed char         frame_type;
  
  unsigned char       granpos_bias;
  
  unsigned char       nqis;
  
  unsigned char       qis[3];
  

  ogg_uint16_t       *dequant_tables[64][3][2];
  OC_ALIGN16(oc_quant_table      dequant_table_data[64][3][2]);
  
  unsigned char       loop_filter_limits[64];
};










typedef void (*oc_set_chroma_mvs_func)(oc_mv _cbmvs[4],const oc_mv _lbmvs[4]);






extern const oc_set_chroma_mvs_func OC_SET_CHROMA_MVS_TABLE[TH_PF_NFORMATS];



int oc_state_init(oc_theora_state *_state,const th_info *_info,int _nrefs);
void oc_state_clear(oc_theora_state *_state);
void oc_state_accel_init_c(oc_theora_state *_state);
void oc_state_borders_fill_rows(oc_theora_state *_state,int _refi,int _pli,
 int _y0,int _yend);
void oc_state_borders_fill_caps(oc_theora_state *_state,int _refi,int _pli);
void oc_state_borders_fill(oc_theora_state *_state,int _refi);
void oc_state_fill_buffer_ptrs(oc_theora_state *_state,int _buf_idx,
 th_ycbcr_buffer _img);
int oc_state_mbi_for_pos(oc_theora_state *_state,int _mbx,int _mby);
int oc_state_get_mv_offsets(const oc_theora_state *_state,int _offsets[2],
 int _pli,oc_mv _mv);

void oc_loop_filter_init_c(signed char _bv[256],int _flimit);
void oc_state_loop_filter(oc_theora_state *_state,int _frame);
# if defined(OC_DUMP_IMAGES)
int oc_state_dump_frame(const oc_theora_state *_state,int _frame,
 const char *_suf);
# endif


void oc_frag_copy_c(unsigned char *_dst,
 const unsigned char *_src,int _src_ystride);
void oc_frag_copy_list_c(unsigned char *_dst_frame,
 const unsigned char *_src_frame,int _ystride,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,const ptrdiff_t *_frag_buf_offs);
void oc_frag_recon_intra_c(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t _residue[64]);
void oc_frag_recon_inter_c(unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
void oc_frag_recon_inter2_c(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t _residue[64]);
void oc_idct8x8_c(ogg_int16_t _y[64],ogg_int16_t _x[64],int _last_zzi);
void oc_state_frag_recon_c(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,ogg_uint16_t _dc_quant);
void oc_state_loop_filter_frag_rows_c(const oc_theora_state *_state,
 signed char _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end);
void oc_restore_fpu_c(void);








typedef void (*oc_state_clear_func)(theora_state *_th);
typedef int (*oc_state_control_func)(theora_state *th,int _req,
 void *_buf,size_t _buf_sz);
typedef ogg_int64_t (*oc_state_granule_frame_func)(theora_state *_th,
 ogg_int64_t _granulepos);
typedef double (*oc_state_granule_time_func)(theora_state *_th,
 ogg_int64_t _granulepos);


struct oc_state_dispatch_vtable{
  oc_state_clear_func         clear;
  oc_state_control_func       control;
  oc_state_granule_frame_func granule_frame;
  oc_state_granule_time_func  granule_time;
};

#endif
