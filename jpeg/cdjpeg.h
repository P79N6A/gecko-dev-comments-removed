










#define JPEG_CJPEG_DJPEG
#define JPEG_INTERNAL_OPTIONS
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"		
#include "cderror.h"		






typedef struct cjpeg_source_struct * cjpeg_source_ptr;

struct cjpeg_source_struct {
  JMETHOD(void, start_input, (j_compress_ptr cinfo,
			      cjpeg_source_ptr sinfo));
  JMETHOD(JDIMENSION, get_pixel_rows, (j_compress_ptr cinfo,
				       cjpeg_source_ptr sinfo));
  JMETHOD(void, finish_input, (j_compress_ptr cinfo,
			       cjpeg_source_ptr sinfo));

  FILE *input_file;

  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
};






typedef struct djpeg_dest_struct * djpeg_dest_ptr;

struct djpeg_dest_struct {
  


  JMETHOD(void, start_output, (j_decompress_ptr cinfo,
			       djpeg_dest_ptr dinfo));
  
  JMETHOD(void, put_pixel_rows, (j_decompress_ptr cinfo,
				 djpeg_dest_ptr dinfo,
				 JDIMENSION rows_supplied));
  
  JMETHOD(void, finish_output, (j_decompress_ptr cinfo,
				djpeg_dest_ptr dinfo));

  
  FILE * output_file;

  



  JSAMPARRAY buffer;
  JDIMENSION buffer_height;
};










struct cdjpeg_progress_mgr {
  struct jpeg_progress_mgr pub;	
  int completed_extra_passes;	
  int total_extra_passes;	
  
  int percent_done;
};

typedef struct cdjpeg_progress_mgr * cd_progress_ptr;




#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jinit_read_bmp		jIRdBMP
#define jinit_write_bmp		jIWrBMP
#define jinit_read_gif		jIRdGIF
#define jinit_write_gif		jIWrGIF
#define jinit_read_ppm		jIRdPPM
#define jinit_write_ppm		jIWrPPM
#define jinit_read_rle		jIRdRLE
#define jinit_write_rle		jIWrRLE
#define jinit_read_targa	jIRdTarga
#define jinit_write_targa	jIWrTarga
#define read_quant_tables	RdQTables
#define read_scan_script	RdScnScript
#define set_quant_slots		SetQSlots
#define set_sample_factors	SetSFacts
#define read_color_map		RdCMap
#define enable_signal_catcher	EnSigCatcher
#define start_progress_monitor	StProgMon
#define end_progress_monitor	EnProgMon
#define read_stdin		RdStdin
#define write_stdout		WrStdout
#endif 



EXTERN(cjpeg_source_ptr) jinit_read_bmp JPP((j_compress_ptr cinfo));
EXTERN(djpeg_dest_ptr) jinit_write_bmp JPP((j_decompress_ptr cinfo,
					    boolean is_os2));
EXTERN(cjpeg_source_ptr) jinit_read_gif JPP((j_compress_ptr cinfo));
EXTERN(djpeg_dest_ptr) jinit_write_gif JPP((j_decompress_ptr cinfo));
EXTERN(cjpeg_source_ptr) jinit_read_ppm JPP((j_compress_ptr cinfo));
EXTERN(djpeg_dest_ptr) jinit_write_ppm JPP((j_decompress_ptr cinfo));
EXTERN(cjpeg_source_ptr) jinit_read_rle JPP((j_compress_ptr cinfo));
EXTERN(djpeg_dest_ptr) jinit_write_rle JPP((j_decompress_ptr cinfo));
EXTERN(cjpeg_source_ptr) jinit_read_targa JPP((j_compress_ptr cinfo));
EXTERN(djpeg_dest_ptr) jinit_write_targa JPP((j_decompress_ptr cinfo));



EXTERN(boolean) read_quant_tables JPP((j_compress_ptr cinfo, char * filename,
				    int scale_factor, boolean force_baseline));
EXTERN(boolean) read_scan_script JPP((j_compress_ptr cinfo, char * filename));
EXTERN(boolean) set_quant_slots JPP((j_compress_ptr cinfo, char *arg));
EXTERN(boolean) set_sample_factors JPP((j_compress_ptr cinfo, char *arg));



EXTERN(void) read_color_map JPP((j_decompress_ptr cinfo, FILE * infile));



EXTERN(void) enable_signal_catcher JPP((j_common_ptr cinfo));
EXTERN(void) start_progress_monitor JPP((j_common_ptr cinfo,
					 cd_progress_ptr progress));
EXTERN(void) end_progress_monitor JPP((j_common_ptr cinfo));
EXTERN(boolean) keymatch JPP((char * arg, const char * keyword, int minchars));
EXTERN(FILE *) read_stdin JPP((void));
EXTERN(FILE *) write_stdout JPP((void));



#ifdef DONT_USE_B_MODE		
#define READ_BINARY	"r"
#define WRITE_BINARY	"w"
#else
#ifdef VMS			
#define READ_BINARY	"rb", "ctx=stm"
#define WRITE_BINARY	"wb", "ctx=stm"
#else				
#define READ_BINARY	"rb"
#define WRITE_BINARY	"wb"
#endif
#endif

#ifndef EXIT_FAILURE		
#define EXIT_FAILURE  1
#endif
#ifndef EXIT_SUCCESS
#ifdef VMS
#define EXIT_SUCCESS  1		
#else
#define EXIT_SUCCESS  0
#endif
#endif
#ifndef EXIT_WARNING
#ifdef VMS
#define EXIT_WARNING  1		
#else
#define EXIT_WARNING  2
#endif
#endif
