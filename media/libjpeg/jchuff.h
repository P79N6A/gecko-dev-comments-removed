



















#if BITS_IN_JSAMPLE == 8
#define MAX_COEF_BITS 10
#else
#define MAX_COEF_BITS 14
#endif



typedef struct {
  unsigned int ehufco[256];     
  char ehufsi[256];             
  
} c_derived_tbl;


EXTERN(void) jpeg_make_c_derived_tbl
        (j_compress_ptr cinfo, boolean isDC, int tblno,
         c_derived_tbl ** pdtbl);


EXTERN(void) jpeg_gen_optimal_table
        (j_compress_ptr cinfo, JHUFF_TBL * htbl, long freq[]);
