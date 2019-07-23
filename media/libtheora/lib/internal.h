
















#if !defined(_internal_H)
# define _internal_H (1)
# include <stdlib.h>
# if defined(HAVE_CONFIG_H)
#  include <config.h>
# endif
# include "theora/codec.h"
# include "theora/theora.h"
# include "dec/ocintrin.h"
# include "dec/huffman.h"
# include "dec/quant.h"


# if defined(_MSC_VER)
#  pragma warning(disable:4554) /* order of operations */
#  pragma warning(disable:4799) /* disable missing EMMS warnings */
# endif


# define OC_VENDOR_STRING "Xiph.Org libTheora I 20081020 3 2 1"


# define TH_VERSION_MAJOR (3)
# define TH_VERSION_MINOR (2)
# define TH_VERSION_SUB   (1)
# define TH_VERSION_CHECK(_info,_maj,_min,_sub) \
 ((_info)->version_major>(_maj)||(_info)->version_major==(_maj)&& \
 ((_info)->version_minor>(_min)||(_info)->version_minor==(_min)&& \
 (_info)->version_subminor>=(_sub)))


#define OC_INTRA_FRAME (0)

#define OC_INTER_FRAME (1)

#define OC_UNKWN_FRAME (-1)





#define OC_UMV_PADDING (16)



#define OC_FRAME_GOLD (0)

#define OC_FRAME_PREV (1)

#define OC_FRAME_SELF (2)


#define OC_FRAME_IO   (3)



#define OC_MODE_INVALID        (-1)

#define OC_MODE_INTER_NOMV     (0)

#define OC_MODE_INTRA          (1)


#define OC_MODE_INTER_MV       (2)


#define OC_MODE_INTER_MV_LAST  (3)


#define OC_MODE_INTER_MV_LAST2 (4)


#define OC_MODE_GOLDEN_NOMV    (5)


#define OC_MODE_GOLDEN_MV      (6)


#define OC_MODE_INTER_MV_FOUR  (7)

#define OC_NMODES              (8)


#define OC_MODE_NOT_CODED      (8)



#define OC_PL  (1)

#define OC_PUL (2)

#define OC_PU  (4)

#define OC_PUR (8)




#define OC_PACKET_INFO_HDR    (-3)

#define OC_PACKET_COMMENT_HDR (-2)

#define OC_PACKET_SETUP_HDR   (-1)

#define OC_PACKET_DONE        (INT_MAX)



typedef struct oc_theora_state oc_theora_state;




typedef int         oc_sb_map[4][4];

typedef int         oc_mb_map[3][4];

typedef signed char oc_mv[2];
















typedef struct{
  unsigned  coded_fully:1;
  unsigned  coded_partially:1;
  unsigned  quad_valid:4;
  oc_sb_map map;
}oc_sb;










typedef struct{
  


  int           mode;
  
  int           x;
  
  int           y;
  



  oc_mb_map     map;
}oc_mb;









typedef struct{
  

  ogg_int64_t mask;
  

  int         npixels;
}oc_border_info;




typedef struct{
  
  unsigned        coded:1;
  



  unsigned        invalid:1;
  
  unsigned        qi:6;
  


  signed int      mbmode:8;
  


  signed int      dc:16;
  





  unsigned char  *buffer[4];
  


  oc_border_info *border;
  
  oc_mv           mv;
}oc_fragment;




typedef struct{
  
  int nhfrags;
  
  int nvfrags;
  
  int froffset;
  
  int nfrags;
  
  int nhsbs;
  
  int nvsbs;
  
  int sboffset;
  
  int nsbs;
}oc_fragment_plane;




typedef struct{
  void (*frag_recon_intra)(unsigned char *_dst,int _dst_ystride,
   const ogg_int16_t *_residue);
  void (*frag_recon_inter)(unsigned char *_dst,int _dst_ystride,
   const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue);
  void (*frag_recon_inter2)(unsigned char *_dst,int _dst_ystride,
   const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
   int _src2_ystride,const ogg_int16_t *_residue);
  void (*state_frag_copy)(const oc_theora_state *_state,
   const int *_fragis,int _nfragis,int _dst_frame,int _src_frame,int _pli);
  void (*state_frag_recon)(oc_theora_state *_state,oc_fragment *_frag,
   int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,int _ncoefs,
   ogg_uint16_t _dc_iquant,const ogg_uint16_t _ac_iquant[64]);
  void (*restore_fpu)(void);
  void (*state_loop_filter_frag_rows)(oc_theora_state *_state,int *_bv,
   int _refi,int _pli,int _fragy0,int _fragy_end);  
}oc_base_opt_vtable;




struct oc_theora_state{
  
  th_info             info;
  
  oc_base_opt_vtable  opt_vtable;
  
  ogg_uint32_t        cpu_flags;
  
  oc_fragment_plane   fplanes[3];
  
  int                 nfrags;
  
  oc_fragment        *frags;
  
  int                 nsbs;
  
  oc_sb              *sbs;
  
  int                 nhmbs;
  
  int                 nvmbs;
  
  int                 nmbs;
  


  oc_mb              *mbs;
  
  int                *coded_fragis;
  
  int                 ncoded_fragis[3];
  


  int                *uncoded_fragis;
  
  int                 nuncoded_fragis[3];
  
  int                *coded_mbis;
  
  int                 ncoded_mbis;
  


  th_ycbcr_buffer     input;
  
  int                 nborders;
  

  oc_border_info      borders[16];
  
  int                 ref_frame_idx[3];
  
  th_ycbcr_buffer     ref_frame_bufs[3];
  
  unsigned char      *ref_frame_data;
  
  ogg_int64_t         keyframe_num;
  
  ogg_int64_t         curframe_num;
  
  ogg_int64_t         granpos;
  
  int                 frame_type;
  
  int                 qis[3];
  
  int                 nqis;
  
  oc_quant_table     *dequant_tables[2][3];
  oc_quant_tables     dequant_table_data[2][3];
  
  unsigned char       loop_filter_limits[64];
};










typedef void (*oc_set_chroma_mvs_func)(oc_mv _cbmvs[4],const oc_mv _lbmvs[4]);








extern const int OC_FZIG_ZAG[128];


extern const int OC_IZIG_ZAG[64];

extern const int OC_FRAME_FOR_MODE[OC_NMODES];


extern const int OC_MB_MAP[2][2];


extern const int OC_MB_MAP_IDXS[TH_PF_NFORMATS][12];


extern const int OC_MB_MAP_NIDXS[TH_PF_NFORMATS];



extern const oc_set_chroma_mvs_func OC_SET_CHROMA_MVS_TABLE[TH_PF_NFORMATS];



int oc_ilog(unsigned _v);
void **oc_malloc_2d(size_t _height,size_t _width,size_t _sz);
void **oc_calloc_2d(size_t _height,size_t _width,size_t _sz);
void oc_free_2d(void *_ptr);

void oc_ycbcr_buffer_flip(th_ycbcr_buffer _dst,
 const th_ycbcr_buffer _src);

int oc_dct_token_skip(int _token,int _extra_bits);

int oc_frag_pred_dc(const oc_fragment *_frag,
 const oc_fragment_plane *_fplane,int _x,int _y,int _pred_last[3]);

int oc_state_init(oc_theora_state *_state,const th_info *_info);
void oc_state_clear(oc_theora_state *_state);
void oc_state_vtable_init_c(oc_theora_state *_state);
void oc_state_borders_fill_rows(oc_theora_state *_state,int _refi,int _pli,
 int _y0,int _yend);
void oc_state_borders_fill_caps(oc_theora_state *_state,int _refi,int _pli);
void oc_state_borders_fill(oc_theora_state *_state,int _refi);
void oc_state_fill_buffer_ptrs(oc_theora_state *_state,int _buf_idx,
 th_ycbcr_buffer _img);
int oc_state_mbi_for_pos(oc_theora_state *_state,int _mbx,int _mby);
int oc_state_get_mv_offsets(oc_theora_state *_state,int *_offsets,
 int _dx,int _dy,int _ystride,int _pli);

int oc_state_loop_filter_init(oc_theora_state *_state,int *_bv);
void oc_state_loop_filter(oc_theora_state *_state,int _frame);
#if defined(OC_DUMP_IMAGES)
int oc_state_dump_frame(const oc_theora_state *_state,int _frame,
 const char *_suf);
#endif


void oc_frag_recon_intra(const oc_theora_state *_state,
 unsigned char *_dst,int _dst_ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter(const oc_theora_state *_state,
 unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter2(const oc_theora_state *_state,
 unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue);
void oc_state_frag_copy(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli);
void oc_state_frag_recon(oc_theora_state *_state,oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,int _ncoefs,
 ogg_uint16_t _dc_iquant,const ogg_uint16_t _ac_iquant[64]);
void oc_state_loop_filter_frag_rows(oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end);
void oc_restore_fpu(const oc_theora_state *_state);


void oc_frag_recon_intra_c(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t *_residue);
void oc_frag_recon_inter_c(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src,int _src_ystride,const ogg_int16_t *_residue);
void oc_frag_recon_inter2_c(unsigned char *_dst,int _dst_ystride,
 const unsigned char *_src1,int _src1_ystride,const unsigned char *_src2,
 int _src2_ystride,const ogg_int16_t *_residue);
void oc_state_frag_copy_c(const oc_theora_state *_state,const int *_fragis,
 int _nfragis,int _dst_frame,int _src_frame,int _pli);
void oc_state_frag_recon_c(oc_theora_state *_state,oc_fragment *_frag,
 int _pli,ogg_int16_t _dct_coeffs[128],int _last_zzi,int _ncoefs,
 ogg_uint16_t _dc_iquant,const ogg_uint16_t _ac_iquant[64]);
void oc_state_loop_filter_frag_rows_c(oc_theora_state *_state,int *_bv,
 int _refi,int _pli,int _fragy0,int _fragy_end);
void oc_restore_fpu_c(void);








typedef void (*oc_state_clear_func)(theora_state *_th);
typedef int (*oc_state_control_func)(theora_state *th,int req,
 void *buf,size_t buf_sz);
typedef ogg_int64_t (*oc_state_granule_frame_func)(theora_state *_th,
 ogg_int64_t _granulepos);
typedef double (*oc_state_granule_time_func)(theora_state *_th,
 ogg_int64_t _granulepos);

typedef struct oc_state_dispatch_vtbl oc_state_dispatch_vtbl;

struct oc_state_dispatch_vtbl{
  oc_state_clear_func         clear;
  oc_state_control_func       control;
  oc_state_granule_frame_func granule_frame;
  oc_state_granule_time_func  granule_time;
};

#endif
