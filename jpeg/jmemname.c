













#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		

#ifndef HAVE_STDLIB_H		
extern void * malloc JPP((size_t size));
extern void free JPP((void *ptr));
#endif

#ifndef SEEK_SET		
#define SEEK_SET  0		/* if not, assume 0 is correct */
#endif

#ifdef DONT_USE_B_MODE		
#define READ_BINARY	"r"
#define RW_BINARY	"w+"
#else
#ifdef VMS			
#define READ_BINARY	"rb", "ctx=stm"
#define RW_BINARY	"w+b", "ctx=stm"
#else				
#define READ_BINARY	"rb"
#define RW_BINARY	"w+b"
#endif
#endif




























#ifndef TEMP_DIRECTORY		
#define TEMP_DIRECTORY  "/usr/tmp/" /* recommended setting for Unix */
#endif

static int next_file_num;	

#ifdef NO_MKTEMP

#ifndef TEMP_FILE_NAME		
#define TEMP_FILE_NAME  "%sJPG%03d.TMP"
#endif

#ifndef NO_ERRNO_H
#include <errno.h>		
#endif





#ifndef errno
extern int errno;
#endif


LOCAL(void)
select_file_name (char * fname)
{
  FILE * tfile;

  
  for (;;) {
    next_file_num++;		
    sprintf(fname, TEMP_FILE_NAME, TEMP_DIRECTORY, next_file_num);
    if ((tfile = fopen(fname, READ_BINARY)) == NULL) {
      



#ifdef ENOENT
      if (errno != ENOENT)
	continue;
#endif
      break;
    }
    fclose(tfile);		
  }
}

#else 


#ifndef TEMP_FILE_NAME		
#define TEMP_FILE_NAME  "%sJPG%dXXXXXX"
#endif

LOCAL(void)
select_file_name (char * fname)
{
  next_file_num++;		
  sprintf(fname, TEMP_FILE_NAME, TEMP_DIRECTORY, next_file_num);
  mktemp(fname);		
  
}

#endif 







GLOBAL(void *)
jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_small (j_common_ptr cinfo, void * object, size_t sizeofobject)
{
  free(object);
}









GLOBAL(void FAR *)
jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject)
{
  return (void FAR *) malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  free(object);
}










#ifndef DEFAULT_MAX_MEM		
#define DEFAULT_MAX_MEM		1000000L 
#endif

GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
		    long max_bytes_needed, long already_allocated)
{
  return cinfo->mem->max_memory_to_use - already_allocated;
}










METHODDEF(void)
read_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		    void FAR * buffer_address,
		    long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFREAD(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_READ);
}


METHODDEF(void)
write_backing_store (j_common_ptr cinfo, backing_store_ptr info,
		     void FAR * buffer_address,
		     long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  if (JFWRITE(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(cinfo, JERR_TFILE_WRITE);
}


METHODDEF(void)
close_backing_store (j_common_ptr cinfo, backing_store_ptr info)
{
  fclose(info->temp_file);	
  unlink(info->temp_name);	




  TRACEMSS(cinfo, 1, JTRC_TFILE_CLOSE, info->temp_name);
}






GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
  select_file_name(info->temp_name);
  if ((info->temp_file = fopen(info->temp_name, RW_BINARY)) == NULL)
    ERREXITS(cinfo, JERR_TFILE_CREATE, info->temp_name);
  info->read_backing_store = read_backing_store;
  info->write_backing_store = write_backing_store;
  info->close_backing_store = close_backing_store;
  TRACEMSS(cinfo, 1, JTRC_TFILE_OPEN, info->temp_name);
}







GLOBAL(long)
jpeg_mem_init (j_common_ptr cinfo)
{
  next_file_num = 0;		
  return DEFAULT_MAX_MEM;	
}

GLOBAL(void)
jpeg_mem_term (j_common_ptr cinfo)
{
  
}
