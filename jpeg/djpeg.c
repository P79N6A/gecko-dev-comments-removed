
























#include "cdjpeg.h"		
#include "jversion.h"		

#include <ctype.h>		

#ifdef USE_CCOMMAND		
#ifdef __MWERKS__
#include <SIOUX.h>              
#include <console.h>		
#endif
#ifdef THINK_C
#include <console.h>		
#endif
#endif




#define JMESSAGE(code,string)	string ,

static const char * const cdjpeg_message_table[] = {
#include "cderror.h"
  NULL
};









typedef enum {
	FMT_BMP,		
	FMT_GIF,		
	FMT_OS2,		
	FMT_PPM,		
	FMT_RLE,		
	FMT_TARGA,		
	FMT_TIFF		
} IMAGE_FORMATS;

#ifndef DEFAULT_FMT		
#define DEFAULT_FMT	FMT_PPM
#endif

static IMAGE_FORMATS requested_fmt;











static const char * progname;	
static char * outfilename;	


LOCAL(void)
usage (void)

{
  fprintf(stderr, "usage: %s [switches] ", progname);
#ifdef TWO_FILE_COMMANDLINE
  fprintf(stderr, "inputfile outputfile\n");
#else
  fprintf(stderr, "[inputfile]\n");
#endif

  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -colors N      Reduce image to no more than N colors\n");
  fprintf(stderr, "  -fast          Fast, low-quality processing\n");
  fprintf(stderr, "  -grayscale     Force grayscale output\n");
#ifdef IDCT_SCALING_SUPPORTED
  fprintf(stderr, "  -scale M/N     Scale output image by fraction M/N, eg, 1/8\n");
#endif
#ifdef BMP_SUPPORTED
  fprintf(stderr, "  -bmp           Select BMP output format (Windows style)%s\n",
	  (DEFAULT_FMT == FMT_BMP ? " (default)" : ""));
#endif
#ifdef GIF_SUPPORTED
  fprintf(stderr, "  -gif           Select GIF output format%s\n",
	  (DEFAULT_FMT == FMT_GIF ? " (default)" : ""));
#endif
#ifdef BMP_SUPPORTED
  fprintf(stderr, "  -os2           Select BMP output format (OS/2 style)%s\n",
	  (DEFAULT_FMT == FMT_OS2 ? " (default)" : ""));
#endif
#ifdef PPM_SUPPORTED
  fprintf(stderr, "  -pnm           Select PBMPLUS (PPM/PGM) output format%s\n",
	  (DEFAULT_FMT == FMT_PPM ? " (default)" : ""));
#endif
#ifdef RLE_SUPPORTED
  fprintf(stderr, "  -rle           Select Utah RLE output format%s\n",
	  (DEFAULT_FMT == FMT_RLE ? " (default)" : ""));
#endif
#ifdef TARGA_SUPPORTED
  fprintf(stderr, "  -targa         Select Targa output format%s\n",
	  (DEFAULT_FMT == FMT_TARGA ? " (default)" : ""));
#endif
  fprintf(stderr, "Switches for advanced users:\n");
#ifdef DCT_ISLOW_SUPPORTED
  fprintf(stderr, "  -dct int       Use integer DCT method%s\n",
	  (JDCT_DEFAULT == JDCT_ISLOW ? " (default)" : ""));
#endif
#ifdef DCT_IFAST_SUPPORTED
  fprintf(stderr, "  -dct fast      Use fast integer DCT (less accurate)%s\n",
	  (JDCT_DEFAULT == JDCT_IFAST ? " (default)" : ""));
#endif
#ifdef DCT_FLOAT_SUPPORTED
  fprintf(stderr, "  -dct float     Use floating-point DCT method%s\n",
	  (JDCT_DEFAULT == JDCT_FLOAT ? " (default)" : ""));
#endif
  fprintf(stderr, "  -dither fs     Use F-S dithering (default)\n");
  fprintf(stderr, "  -dither none   Don't use dithering in quantization\n");
  fprintf(stderr, "  -dither ordered  Use ordered dither (medium speed, quality)\n");
#ifdef QUANT_2PASS_SUPPORTED
  fprintf(stderr, "  -map FILE      Map to colors used in named image file\n");
#endif
  fprintf(stderr, "  -nosmooth      Don't use high-quality upsampling\n");
#ifdef QUANT_1PASS_SUPPORTED
  fprintf(stderr, "  -onepass       Use 1-pass quantization (fast, low quality)\n");
#endif
  fprintf(stderr, "  -maxmemory N   Maximum memory to use (in kbytes)\n");
  fprintf(stderr, "  -outfile name  Specify name for output file\n");
  fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
  exit(EXIT_FAILURE);
}


LOCAL(int)
parse_switches (j_decompress_ptr cinfo, int argc, char **argv,
		int last_file_arg_seen, boolean for_real)








{
  int argn;
  char * arg;

  
  requested_fmt = DEFAULT_FMT;	
  outfilename = NULL;
  cinfo->err->trace_level = 0;

  

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') {
      
      if (argn <= last_file_arg_seen) {
	outfilename = NULL;	
	continue;		
      }
      break;			
    }
    arg++;			

    if (keymatch(arg, "bmp", 1)) {
      
      requested_fmt = FMT_BMP;

    } else if (keymatch(arg, "colors", 1) || keymatch(arg, "colours", 1) ||
	       keymatch(arg, "quantize", 1) || keymatch(arg, "quantise", 1)) {
      
      int val;

      if (++argn >= argc)	
	usage();
      if (sscanf(argv[argn], "%d", &val) != 1)
	usage();
      cinfo->desired_number_of_colors = val;
      cinfo->quantize_colors = TRUE;

    } else if (keymatch(arg, "dct", 2)) {
      
      if (++argn >= argc)	
	usage();
      if (keymatch(argv[argn], "int", 1)) {
	cinfo->dct_method = JDCT_ISLOW;
      } else if (keymatch(argv[argn], "fast", 2)) {
	cinfo->dct_method = JDCT_IFAST;
      } else if (keymatch(argv[argn], "float", 2)) {
	cinfo->dct_method = JDCT_FLOAT;
      } else
	usage();

    } else if (keymatch(arg, "dither", 2)) {
      
      if (++argn >= argc)	
	usage();
      if (keymatch(argv[argn], "fs", 2)) {
	cinfo->dither_mode = JDITHER_FS;
      } else if (keymatch(argv[argn], "none", 2)) {
	cinfo->dither_mode = JDITHER_NONE;
      } else if (keymatch(argv[argn], "ordered", 2)) {
	cinfo->dither_mode = JDITHER_ORDERED;
      } else
	usage();

    } else if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
      
      
      static boolean printed_version = FALSE;

      if (! printed_version) {
	fprintf(stderr, "Independent JPEG Group's DJPEG, version %s\n%s\n",
		JVERSION, JCOPYRIGHT);
	printed_version = TRUE;
      }
      cinfo->err->trace_level++;

    } else if (keymatch(arg, "fast", 1)) {
      
      cinfo->two_pass_quantize = FALSE;
      cinfo->dither_mode = JDITHER_ORDERED;
      if (! cinfo->quantize_colors) 
	cinfo->desired_number_of_colors = 216;
      cinfo->dct_method = JDCT_FASTEST;
      cinfo->do_fancy_upsampling = FALSE;

    } else if (keymatch(arg, "gif", 1)) {
      
      requested_fmt = FMT_GIF;

    } else if (keymatch(arg, "grayscale", 2) || keymatch(arg, "greyscale",2)) {
      
      cinfo->out_color_space = JCS_GRAYSCALE;

    } else if (keymatch(arg, "map", 3)) {
      
      if (++argn >= argc)	
	usage();
      if (for_real) {		
#ifdef QUANT_2PASS_SUPPORTED	
	FILE * mapfile;

	if ((mapfile = fopen(argv[argn], READ_BINARY)) == NULL) {
	  fprintf(stderr, "%s: can't open %s\n", progname, argv[argn]);
	  exit(EXIT_FAILURE);
	}
	read_color_map(cinfo, mapfile);
	fclose(mapfile);
	cinfo->quantize_colors = TRUE;
#else
	ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
      }

    } else if (keymatch(arg, "maxmemory", 3)) {
      
      long lval;
      char ch = 'x';

      if (++argn >= argc)	
	usage();
      if (sscanf(argv[argn], "%ld%c", &lval, &ch) < 1)
	usage();
      if (ch == 'm' || ch == 'M')
	lval *= 1000L;
      cinfo->mem->max_memory_to_use = lval * 1000L;

    } else if (keymatch(arg, "nosmooth", 3)) {
      
      cinfo->do_fancy_upsampling = FALSE;

    } else if (keymatch(arg, "onepass", 3)) {
      
      cinfo->two_pass_quantize = FALSE;

    } else if (keymatch(arg, "os2", 3)) {
      
      requested_fmt = FMT_OS2;

    } else if (keymatch(arg, "outfile", 4)) {
      
      if (++argn >= argc)	
	usage();
      outfilename = argv[argn];	

    } else if (keymatch(arg, "pnm", 1) || keymatch(arg, "ppm", 1)) {
      
      requested_fmt = FMT_PPM;

    } else if (keymatch(arg, "rle", 1)) {
      
      requested_fmt = FMT_RLE;

    } else if (keymatch(arg, "scale", 1)) {
      
      if (++argn >= argc)	
	usage();
      if (sscanf(argv[argn], "%d/%d",
		 &cinfo->scale_num, &cinfo->scale_denom) != 2)
	usage();

    } else if (keymatch(arg, "targa", 1)) {
      
      requested_fmt = FMT_TARGA;

    } else {
      usage();			
    }
  }

  return argn;			
}









LOCAL(unsigned int)
jpeg_getc (j_decompress_ptr cinfo)

{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0) {
    if (! (*datasrc->fill_input_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  }
  datasrc->bytes_in_buffer--;
  return GETJOCTET(*datasrc->next_input_byte++);
}


METHODDEF(boolean)
print_text_marker (j_decompress_ptr cinfo)
{
  boolean traceit = (cinfo->err->trace_level >= 1);
  INT32 length;
  unsigned int ch;
  unsigned int lastch = 0;

  length = jpeg_getc(cinfo) << 8;
  length += jpeg_getc(cinfo);
  length -= 2;			

  if (traceit) {
    if (cinfo->unread_marker == JPEG_COM)
      fprintf(stderr, "Comment, length %ld:\n", (long) length);
    else			
      fprintf(stderr, "APP%d, length %ld:\n",
	      cinfo->unread_marker - JPEG_APP0, (long) length);
  }

  while (--length >= 0) {
    ch = jpeg_getc(cinfo);
    if (traceit) {
      




      if (ch == '\r') {
	fprintf(stderr, "\n");
      } else if (ch == '\n') {
	if (lastch != '\r')
	  fprintf(stderr, "\n");
      } else if (ch == '\\') {
	fprintf(stderr, "\\\\");
      } else if (isprint(ch)) {
	putc(ch, stderr);
      } else {
	fprintf(stderr, "\\%03o", ch);
      }
      lastch = ch;
    }
  }

  if (traceit)
    fprintf(stderr, "\n");

  return TRUE;
}






int
main (int argc, char **argv)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif
  int file_index;
  djpeg_dest_ptr dest_mgr = NULL;
  FILE * input_file;
  FILE * output_file;
  JDIMENSION num_scanlines;

  
#ifdef USE_CCOMMAND
  argc = ccommand(&argv);
#endif

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "djpeg";		

  
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  
  jerr.addon_message_table = cdjpeg_message_table;
  jerr.first_addon_message = JMSG_FIRSTADDONCODE;
  jerr.last_addon_message = JMSG_LASTADDONCODE;

  





  jpeg_set_marker_processor(&cinfo, JPEG_COM, print_text_marker);
  jpeg_set_marker_processor(&cinfo, JPEG_APP0+12, print_text_marker);

  
#ifdef NEED_SIGNAL_CATCHER
  enable_signal_catcher((j_common_ptr) &cinfo);
#endif

  
  






  file_index = parse_switches(&cinfo, argc, argv, 0, FALSE);

#ifdef TWO_FILE_COMMANDLINE
  
  if (outfilename == NULL) {
    if (file_index != argc-2) {
      fprintf(stderr, "%s: must name one input and one output file\n",
	      progname);
      usage();
    }
    outfilename = argv[file_index+1];
  } else {
    if (file_index != argc-1) {
      fprintf(stderr, "%s: must name one input and one output file\n",
	      progname);
      usage();
    }
  }
#else
  
  if (file_index < argc-1) {
    fprintf(stderr, "%s: only one input file\n", progname);
    usage();
  }
#endif 

  
  if (file_index < argc) {
    if ((input_file = fopen(argv[file_index], READ_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, argv[file_index]);
      exit(EXIT_FAILURE);
    }
  } else {
    
    input_file = read_stdin();
  }

  
  if (outfilename != NULL) {
    if ((output_file = fopen(outfilename, WRITE_BINARY)) == NULL) {
      fprintf(stderr, "%s: can't open %s\n", progname, outfilename);
      exit(EXIT_FAILURE);
    }
  } else {
    
    output_file = write_stdout();
  }

#ifdef PROGRESS_REPORT
  start_progress_monitor((j_common_ptr) &cinfo, &progress);
#endif

  
  jpeg_stdio_src(&cinfo, input_file);

  
  (void) jpeg_read_header(&cinfo, TRUE);

  
  file_index = parse_switches(&cinfo, argc, argv, 0, TRUE);

  


  switch (requested_fmt) {
#ifdef BMP_SUPPORTED
  case FMT_BMP:
    dest_mgr = jinit_write_bmp(&cinfo, FALSE);
    break;
  case FMT_OS2:
    dest_mgr = jinit_write_bmp(&cinfo, TRUE);
    break;
#endif
#ifdef GIF_SUPPORTED
  case FMT_GIF:
    dest_mgr = jinit_write_gif(&cinfo);
    break;
#endif
#ifdef PPM_SUPPORTED
  case FMT_PPM:
    dest_mgr = jinit_write_ppm(&cinfo);
    break;
#endif
#ifdef RLE_SUPPORTED
  case FMT_RLE:
    dest_mgr = jinit_write_rle(&cinfo);
    break;
#endif
#ifdef TARGA_SUPPORTED
  case FMT_TARGA:
    dest_mgr = jinit_write_targa(&cinfo);
    break;
#endif
  default:
    ERREXIT(&cinfo, JERR_UNSUPPORTED_FORMAT);
    break;
  }
  dest_mgr->output_file = output_file;

  
  (void) jpeg_start_decompress(&cinfo);

  
  (*dest_mgr->start_output) (&cinfo, dest_mgr);

  
  while (cinfo.output_scanline < cinfo.output_height) {
    num_scanlines = jpeg_read_scanlines(&cinfo, dest_mgr->buffer,
					dest_mgr->buffer_height);
    (*dest_mgr->put_pixel_rows) (&cinfo, dest_mgr, num_scanlines);
  }

#ifdef PROGRESS_REPORT
  


  progress.pub.completed_passes = progress.pub.total_passes;
#endif

  



  (*dest_mgr->finish_output) (&cinfo, dest_mgr);
  (void) jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  
  if (input_file != stdin)
    fclose(input_file);
  if (output_file != stdout)
    fclose(output_file);

#ifdef PROGRESS_REPORT
  end_progress_monitor((j_common_ptr) &cinfo);
#endif

  
  exit(jerr.num_warnings ? EXIT_WARNING : EXIT_SUCCESS);
  return 0;			
}
