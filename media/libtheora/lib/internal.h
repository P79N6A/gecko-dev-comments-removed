















#if !defined(_internal_H)
# define _internal_H (1)
# include <stdlib.h>
# include <limits.h>
# if defined(HAVE_CONFIG_H)
#  include <config.h>
# endif
# include "theora/codec.h"
# include "theora/theora.h"

# if defined(_MSC_VER)

#  pragma warning(disable:4799)

#  pragma warning(disable:4554)
# endif

# if defined(__GNUC_PREREQ)
#  if __GNUC_PREREQ(4,2)
#   pragma GCC diagnostic ignored "-Wparentheses"
#  endif
# endif

# include "ocintrin.h"
# include "huffman.h"
# include "quant.h"


# if defined(OC_X86_ASM)
#  if defined(__GNUC__)
#   define OC_ALIGN8(expr) expr __attribute__((aligned(8)))
#   define OC_ALIGN16(expr) expr __attribute__((aligned(16)))
#  elif defined(_MSC_VER)
#   define OC_ALIGN8(expr) __declspec (align(8)) expr
#   define OC_ALIGN16(expr) __declspec (align(16)) expr
#  endif
# endif
# if !defined(OC_ALIGN8)
#  define OC_ALIGN8(expr) expr
# endif
# if !defined(OC_ALIGN16)
#  define OC_ALIGN16(expr) expr
# endif



typedef struct oc_sb_flags              oc_sb_flags;
typedef struct oc_border_info           oc_border_info;
typedef struct oc_fragment              oc_fragment;
typedef struct oc_fragment_plane        oc_fragment_plane;
typedef struct oc_base_opt_vtable       oc_base_opt_vtable;
typedef struct oc_base_opt_data         oc_base_opt_data;
typedef struct oc_state_dispatch_vtable oc_state_dispatch_vtable;
typedef struct oc_theora_state          oc_theora_state;




# define OC_VENDOR_STRING "Xiph.Org libtheora 1.1 20090822 (Thusnelda)"


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


#define OC_FRAME_FOR_MODE(_x) \
 OC_UNIBBLE_TABLE32(OC_FRAME_PREV,OC_FRAME_SELF,OC_FRAME_PREV,OC_FRAME_PREV, \
  OC_FRAME_PREV,OC_FRAME_GOLD,OC_FRAME_GOLD,OC_FRAME_PREV,(_x))




#define OC_PACKET_INFO_HDR    (-3)

#define OC_PACKET_COMMENT_HDR (-2)

#define OC_PACKET_SETUP_HDR   (-1)

#define OC_PACKET_DONE        (INT_MAX)

































typedef ptrdiff_t       oc_sb_map_quad[4];

typedef oc_sb_map_quad  oc_sb_map[4];

typedef ptrdiff_t       oc_mb_map_plane[4];

typedef oc_mb_map_plane oc_mb_map[3];

typedef signed char     oc_mv[2];




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
  
  unsigned   qii:6;
  
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




struct oc_base_opt_vtable{
  void (*frag_copy)(unsigned char *_dst,
   const unsigned char *_src,int _ystride);
  void (*frag_recon_intra)(unsigned char *_dst,int _ystride,
   const ogg_int16_t _residue[64]);
  void (*frag_recon_inter)(unsigned char *_dst,
   const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
  void (*frag_recon_inter2)(unsigned char *_dst,const unsigned char *_src1,
   const unsigned char *_src2,int _ystride,const ogg_int16_t _residue[64]);
  void (*idct8x8)(ogg_int16_t _y[64],int _last_zzi);
  void (*state_frag_recon)(const oc_theora_state *_state,ptrdiff_t _fragi,
   int _pli,ogg_int16_t _dct_coeffs[64],int _last_zzi,ogg_uint16_t _dc_quant);
  void (*state_frag_copy_list)(const oc_theora_state *_state,
   const ptrdiff_t *_fragis,ptrdiff_t _nfragis,
   int _dst_frame,int _src_frame,int _pli);
  void (*state_loop_filter_frag_rows)(const oc_theora_state *_state,
   int _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end);  
  void (*restore_fpu)(void);
};



struct oc_base_opt_data{
  const unsigned char *dct_fzig_zag;
};



struct oc_theora_state{
  
  th_info             info;
  
  oc_base_opt_vtable  opt_vtable;
  
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
  
  int                 ref_frame_idx[4];
  
  th_ycbcr_buffer     ref_frame_bufs[4];
  
  unsigned char      *ref_frame_data[4];
  
  int                 ref_ystride[3];
  
  int                 nborders;
  


  oc_border_info      borders[16];
  
  ogg_int64_t         keyframe_num;
  
  ogg_int64_t         curframe_num;
  
  ogg_int64_t         granpos;
  
  unsigned char       frame_type;
  
  unsigned char       granpos_bias;
  
  unsigned char       nqis;
  
  unsigned char       qis[3];
  

  ogg_uint16_t       *dequant_tables[64][3][2];
  OC_ALIGN16(oc_quant_table      dequant_table_data[64][3][2]);
  
  unsigned char       loop_filter_limits[64];
};










typedef void (*oc_set_chroma_mvs_func)(oc_mv _cbmvs[4],const oc_mv _lbmvs[4]);





extern const unsigned char OC_FZIG_ZAG[128];


extern const unsigned char OC_IZIG_ZAG[64];


extern const unsigned char OC_MB_MAP[2][2];


extern const unsigned char OC_MB_MAP_IDXS[TH_PF_NFORMATS][12];


extern const unsigned char OC_MB_MAP_NIDXS[TH_PF_NFORMATS];



extern const oc_set_chroma_mvs_func OC_SET_CHROMA_MVS_TABLE[TH_PF_NFORMATS];



int oc_ilog(unsigned _v);
void **oc_malloc_2d(size_t _height,size_t _width,size_t _sz);
void **oc_calloc_2d(size_t _height,size_t _width,size_t _sz);
void oc_free_2d(void *_ptr);

void oc_ycbcr_buffer_flip(th_ycbcr_buffer _dst,
 const th_ycbcr_buffer _src);

int oc_state_init(oc_theora_state *_state,const th_info *_info,int _nrefs);
void oc_state_clear(oc_theora_state *_state);
void oc_state_vtable_init_c(oc_theora_state *_state);
void oc_state_borders_fill_rows(oc_theora_state *_state,int _refi,int _pli,
 int _y0,int _yend);
void oc_state_borders_fill_caps(oc_theora_state *_state,int _refi,int _pli);
void oc_state_borders_fill(oc_theora_state *_state,int _refi);
void oc_state_fill_buffer_ptrs(oc_theora_state *_state,int _buf_idx,
 th_ycbcr_buffer _img);
int oc_state_mbi_for_pos(oc_theora_state *_state,int _mbx,int _mby);
int oc_state_get_mv_offsets(const oc_theora_state *_state,int _offsets[2],
 int _pli,int _dx,int _dy);

int oc_state_loop_filter_init(oc_theora_state *_state,int *_bv);
void oc_state_loop_filter(oc_theora_state *_state,int _frame);
#if defined(OC_DUMP_IMAGES)
int oc_state_dump_frame(const oc_theora_state *_state,int _frame,
 const char *_suf);
#endif


void oc_frag_copy(const oc_theora_state *_state,unsigned char *_dst,
 const unsigned char *_src,int _ystride);
void oc_frag_recon_intra(const oc_theora_state *_state,
 unsigned char *_dst,int _dst_ystride,const ogg_int16_t _residue[64]);
void oc_frag_recon_inter(const oc_theora_state *_state,unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
void oc_frag_recon_inter2(const oc_theora_state *_state,
 unsigned char *_dst,const unsigned char *_src1,const unsigned char *_src2,
 int _ystride,const ogg_int16_t _residue[64]);
void oc_idct8x8(const oc_theora_state *_state,ogg_int16_t _y[64],int _last_zzi);
void oc_state_frag_recon(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[64],int _last_zzi,ogg_uint16_t _dc_quant);
void oc_state_frag_copy_list(const oc_theora_state *_state,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,
 int _dst_frame,int _src_frame,int _pli);
void oc_state_loop_filter_frag_rows(const oc_theora_state *_state,
 int _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end);
void oc_restore_fpu(const oc_theora_state *_state);


void oc_frag_copy_c(unsigned char *_dst,
 const unsigned char *_src,int _src_ystride);
void oc_frag_recon_intra_c(unsigned char *_dst,int _dst_ystride,
 const ogg_int16_t _residue[64]);
void oc_frag_recon_inter_c(unsigned char *_dst,
 const unsigned char *_src,int _ystride,const ogg_int16_t _residue[64]);
void oc_frag_recon_inter2_c(unsigned char *_dst,const unsigned char *_src1,
 const unsigned char *_src2,int _ystride,const ogg_int16_t _residue[64]);
void oc_idct8x8_c(ogg_int16_t _y[64],int _last_zzi);
void oc_state_frag_recon_c(const oc_theora_state *_state,ptrdiff_t _fragi,
 int _pli,ogg_int16_t _dct_coeffs[64],int _last_zzi,ogg_uint16_t _dc_quant);
void oc_state_frag_copy_list_c(const oc_theora_state *_state,
 const ptrdiff_t *_fragis,ptrdiff_t _nfragis,
 int _dst_frame,int _src_frame,int _pli);
void oc_state_loop_filter_frag_rows_c(const oc_theora_state *_state,
 int _bv[256],int _refi,int _pli,int _fragy0,int _fragy_end);
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
