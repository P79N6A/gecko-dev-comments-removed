
















#ifndef _V_CODEBOOK_H_
#define _V_CODEBOOK_H_

#include <ogg/ogg.h>














typedef struct static_codebook{
  long   dim;            
  long   entries;        
  long  *lengthlist;     

  
  int    maptype;        



  
  long     q_min;       
  long     q_delta;     
  int      q_quant;     
  int      q_sequencep; 

  long     *quantlist;  



  
  struct encode_aux_nearestmatch *nearest_tree;
  struct encode_aux_threshmatch  *thresh_tree;
  struct encode_aux_pigeonhole  *pigeon_tree;

  int allocedp;
} static_codebook;



typedef struct encode_aux_nearestmatch{
  
  long   *ptr0;
  long   *ptr1;

  long   *p;         
  long   *q;         
  long   aux;        
  long   alloc;
} encode_aux_nearestmatch;


typedef struct encode_aux_threshmatch{
  float *quantthresh;
  long   *quantmap;
  int     quantvals;
  int     threshvals;
} encode_aux_threshmatch;

typedef struct encode_aux_pigeonhole{
  float min;
  float del;

  int  mapentries;
  int  quantvals;
  long *pigeonmap;

  long fittotal;
  long *fitlist;
  long *fitmap;
  long *fitlength;
} encode_aux_pigeonhole;

typedef struct codebook{
  long dim;           
  long entries;       
  long used_entries;  
  const static_codebook *c;

  
  

  float        *valuelist;  
  ogg_uint32_t *codelist;   

  int          *dec_index;  
  char         *dec_codelengths;
  ogg_uint32_t *dec_firsttable;
  int           dec_firsttablen;
  int           dec_maxlength;

} codebook;

extern void vorbis_staticbook_clear(static_codebook *b);
extern void vorbis_staticbook_destroy(static_codebook *b);
extern int vorbis_book_init_encode(codebook *dest,const static_codebook *source);
extern int vorbis_book_init_decode(codebook *dest,const static_codebook *source);
extern void vorbis_book_clear(codebook *b);

extern float *_book_unquantize(const static_codebook *b,int n,int *map);
extern float *_book_logdist(const static_codebook *b,float *vals);
extern float _float32_unpack(long val);
extern long   _float32_pack(float val);
extern int  _best(codebook *book, float *a, int step);
extern int _ilog(unsigned int v);
extern long _book_maptype1_quantvals(const static_codebook *b);

extern int vorbis_book_besterror(codebook *book,float *a,int step,int addmul);
extern long vorbis_book_codeword(codebook *book,int entry);
extern long vorbis_book_codelen(codebook *book,int entry);



extern int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *b);
extern int vorbis_staticbook_unpack(oggpack_buffer *b,static_codebook *c);

extern int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);
extern int vorbis_book_errorv(codebook *book, float *a);
extern int vorbis_book_encodev(codebook *book, int best,float *a,
                               oggpack_buffer *b);

extern long vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern long vorbis_book_decodevs_add(codebook *book, float *a,
                                     oggpack_buffer *b,int n);
extern long vorbis_book_decodev_set(codebook *book, float *a,
                                    oggpack_buffer *b,int n);
extern long vorbis_book_decodev_add(codebook *book, float *a,
                                    oggpack_buffer *b,int n);
extern long vorbis_book_decodevv_add(codebook *book, float **a,
                                     long off,int ch,
                                    oggpack_buffer *b,int n);



#endif
