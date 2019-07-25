

















#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"




typedef struct {
  struct jpeg_source_mgr pub;	

  FILE * infile;		
  JOCTET * buffer;		
  boolean start_of_file;	
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */







METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  



  src->start_of_file = TRUE;
}

#if JPEG_LIB_VERSION >= 80
METHODDEF(void)
init_mem_source (j_decompress_ptr cinfo)
{
  
}
#endif



































METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;
  size_t nbytes;

  nbytes = JFREAD(src->infile, src->buffer, INPUT_BUF_SIZE);

  if (nbytes <= 0) {
    if (src->start_of_file)	
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }

  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;

  return TRUE;
}

#if JPEG_LIB_VERSION >= 80
METHODDEF(boolean)
fill_mem_input_buffer (j_decompress_ptr cinfo)
{
  static JOCTET mybuffer[4];

  



  WARNMS(cinfo, JWRN_JPEG_EOF);
  
  mybuffer[0] = (JOCTET) 0xFF;
  mybuffer[1] = (JOCTET) JPEG_EOI;

  cinfo->src->next_input_byte = mybuffer;
  cinfo->src->bytes_in_buffer = 2;

  return TRUE;
}
#endif














METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr * src = cinfo->src;

  



  if (num_bytes > 0) {
    while (num_bytes > (long) src->bytes_in_buffer) {
      num_bytes -= (long) src->bytes_in_buffer;
      (void) (*src->fill_input_buffer) (cinfo);
      


    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}




















METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
  
}








GLOBAL(void)
jpeg_stdio_src (j_decompress_ptr cinfo, FILE * infile)
{
  my_src_ptr src;

  






  if (cinfo->src == NULL) {	
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  SIZEOF(my_source_mgr));
    src = (my_src_ptr) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  INPUT_BUF_SIZE * SIZEOF(JOCTET));
  }

  src = (my_src_ptr) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; 
  src->pub.term_source = term_source;
  src->infile = infile;
  src->pub.bytes_in_buffer = 0; 
  src->pub.next_input_byte = NULL; 
}


#if JPEG_LIB_VERSION >= 80





GLOBAL(void)
jpeg_mem_src (j_decompress_ptr cinfo,
	      unsigned char * inbuffer, unsigned long insize)
{
  struct jpeg_source_mgr * src;

  if (inbuffer == NULL || insize == 0)	
    ERREXIT(cinfo, JERR_INPUT_EMPTY);

  



  if (cinfo->src == NULL) {	
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  SIZEOF(struct jpeg_source_mgr));
  }

  src = cinfo->src;
  src->init_source = init_mem_source;
  src->fill_input_buffer = fill_mem_input_buffer;
  src->skip_input_data = skip_input_data;
  src->resync_to_restart = jpeg_resync_to_restart; 
  src->term_source = term_source;
  src->bytes_in_buffer = (size_t) insize;
  src->next_input_byte = (JOCTET *) inbuffer;
}
#endif
