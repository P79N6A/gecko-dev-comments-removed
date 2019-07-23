
































#ifndef XMS_SUPPORTED
#define XMS_SUPPORTED  1
#endif
#ifndef EMS_SUPPORTED
#define EMS_SUPPORTED  1
#endif


#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"		

#ifndef HAVE_STDLIB_H		
extern void * malloc JPP((size_t size));
extern void free JPP((void *ptr));
extern char * getenv JPP((const char * name));
#endif

#ifdef NEED_FAR_POINTERS

#ifdef __TURBOC__

#include <alloc.h>		
#define far_malloc(x)	farmalloc(x)
#define far_free(x)	farfree(x)
#else

#include <malloc.h>		
#define far_malloc(x)	_fmalloc(x)
#define far_free(x)	_ffree(x)
#endif

#else 

#define far_malloc(x)	malloc(x)
#define far_free(x)	free(x)

#endif 

#ifdef DONT_USE_B_MODE		
#define READ_BINARY	"r"
#else
#define READ_BINARY	"rb"
#endif

#ifndef USE_MSDOS_MEMMGR	
  You forgot to define USE_MSDOS_MEMMGR in jconfig.h. 
#endif

#if MAX_ALLOC_CHUNK >= 65535L	
  MAX_ALLOC_CHUNK should be less than 64K. 
#endif










typedef void far * XMSDRIVER;	
typedef struct {		
	unsigned short ax, dx, bx;
	void far * ds_si;
      } XMScontext;
typedef struct {		
	unsigned short ax, dx, bx;
	void far * ds_si;
      } EMScontext;

extern short far jdos_open JPP((short far * handle, char far * filename));
extern short far jdos_close JPP((short handle));
extern short far jdos_seek JPP((short handle, long offset));
extern short far jdos_read JPP((short handle, void far * buffer,
				unsigned short count));
extern short far jdos_write JPP((short handle, void far * buffer,
				 unsigned short count));
extern void far jxms_getdriver JPP((XMSDRIVER far *));
extern void far jxms_calldriver JPP((XMSDRIVER, XMScontext far *));
extern short far jems_available JPP((void));
extern void far jems_calldriver JPP((EMScontext far *));







static int next_file_num;	

LOCAL(void)
select_file_name (char * fname)
{
  const char * env;
  char * ptr;
  FILE * tfile;

  
  for (;;) {
    


    if ((env = (const char *) getenv("TMP")) == NULL)
      if ((env = (const char *) getenv("TEMP")) == NULL)
	env = ".";
    if (*env == '\0')		
      env = ".";
    ptr = fname;		
    while (*env != '\0')
      *ptr++ = *env++;
    if (ptr[-1] != '\\' && ptr[-1] != '/')
      *ptr++ = '\\';		
    
    next_file_num++;		
    sprintf(ptr, "JPG%03d.TMP", next_file_num);
    
    if ((tfile = fopen(fname, READ_BINARY)) == NULL)
      break;
    fclose(tfile);		
  }
}







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
  return (void FAR *) far_malloc(sizeofobject);
}

GLOBAL(void)
jpeg_free_large (j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
  far_free(object);
}










#ifndef DEFAULT_MAX_MEM		
#define DEFAULT_MAX_MEM		300000L 
#endif

GLOBAL(long)
jpeg_mem_available (j_common_ptr cinfo, long min_bytes_needed,
		    long max_bytes_needed, long already_allocated)
{
  return cinfo->mem->max_memory_to_use - already_allocated;
}





























METHODDEF(void)
read_file_store (j_common_ptr cinfo, backing_store_ptr info,
		 void FAR * buffer_address,
		 long file_offset, long byte_count)
{
  if (jdos_seek(info->handle.file_handle, file_offset))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  
  if (byte_count > 65535L)	
    ERREXIT(cinfo, JERR_BAD_ALLOC_CHUNK);
  if (jdos_read(info->handle.file_handle, buffer_address,
		(unsigned short) byte_count))
    ERREXIT(cinfo, JERR_TFILE_READ);
}


METHODDEF(void)
write_file_store (j_common_ptr cinfo, backing_store_ptr info,
		  void FAR * buffer_address,
		  long file_offset, long byte_count)
{
  if (jdos_seek(info->handle.file_handle, file_offset))
    ERREXIT(cinfo, JERR_TFILE_SEEK);
  
  if (byte_count > 65535L)	
    ERREXIT(cinfo, JERR_BAD_ALLOC_CHUNK);
  if (jdos_write(info->handle.file_handle, buffer_address,
		 (unsigned short) byte_count))
    ERREXIT(cinfo, JERR_TFILE_WRITE);
}


METHODDEF(void)
close_file_store (j_common_ptr cinfo, backing_store_ptr info)
{
  jdos_close(info->handle.file_handle);	
  remove(info->temp_name);	




  TRACEMSS(cinfo, 1, JTRC_TFILE_CLOSE, info->temp_name);
}


LOCAL(boolean)
open_file_store (j_common_ptr cinfo, backing_store_ptr info,
		 long total_bytes_needed)
{
  short handle;

  select_file_name(info->temp_name);
  if (jdos_open((short far *) & handle, (char far *) info->temp_name)) {
    
    ERREXITS(cinfo, JERR_TFILE_CREATE, info->temp_name);
    return FALSE;
  }
  info->handle.file_handle = handle;
  info->read_backing_store = read_file_store;
  info->write_backing_store = write_file_store;
  info->close_backing_store = close_file_store;
  TRACEMSS(cinfo, 1, JTRC_TFILE_OPEN, info->temp_name);
  return TRUE;			
}






#if XMS_SUPPORTED

static XMSDRIVER xms_driver;	

typedef union {			
	long offset;
	void far * ptr;
      } XMSPTR;

typedef struct {		
	long length;
	XMSH src_handle;
	XMSPTR src;
	XMSH dst_handle;
	XMSPTR dst;
      } XMSspec;

#define ODD(X)	(((X) & 1L) != 0)


METHODDEF(void)
read_xms_store (j_common_ptr cinfo, backing_store_ptr info,
		void FAR * buffer_address,
		long file_offset, long byte_count)
{
  XMScontext ctx;
  XMSspec spec;
  char endbuffer[2];

  



  spec.length = byte_count & (~ 1L);
  spec.src_handle = info->handle.xms_handle;
  spec.src.offset = file_offset;
  spec.dst_handle = 0;
  spec.dst.ptr = buffer_address;
  
  ctx.ds_si = (void far *) & spec;
  ctx.ax = 0x0b00;		
  jxms_calldriver(xms_driver, (XMScontext far *) & ctx);
  if (ctx.ax != 1)
    ERREXIT(cinfo, JERR_XMS_READ);

  if (ODD(byte_count)) {
    read_xms_store(cinfo, info, (void FAR *) endbuffer,
		   file_offset + byte_count - 1L, 2L);
    ((char FAR *) buffer_address)[byte_count - 1L] = endbuffer[0];
  }
}


METHODDEF(void)
write_xms_store (j_common_ptr cinfo, backing_store_ptr info,
		 void FAR * buffer_address,
		 long file_offset, long byte_count)
{
  XMScontext ctx;
  XMSspec spec;
  char endbuffer[2];

  



  spec.length = byte_count & (~ 1L);
  spec.src_handle = 0;
  spec.src.ptr = buffer_address;
  spec.dst_handle = info->handle.xms_handle;
  spec.dst.offset = file_offset;

  ctx.ds_si = (void far *) & spec;
  ctx.ax = 0x0b00;		
  jxms_calldriver(xms_driver, (XMScontext far *) & ctx);
  if (ctx.ax != 1)
    ERREXIT(cinfo, JERR_XMS_WRITE);

  if (ODD(byte_count)) {
    read_xms_store(cinfo, info, (void FAR *) endbuffer,
		   file_offset + byte_count - 1L, 2L);
    endbuffer[0] = ((char FAR *) buffer_address)[byte_count - 1L];
    write_xms_store(cinfo, info, (void FAR *) endbuffer,
		    file_offset + byte_count - 1L, 2L);
  }
}


METHODDEF(void)
close_xms_store (j_common_ptr cinfo, backing_store_ptr info)
{
  XMScontext ctx;

  ctx.dx = info->handle.xms_handle;
  ctx.ax = 0x0a00;
  jxms_calldriver(xms_driver, (XMScontext far *) & ctx);
  TRACEMS1(cinfo, 1, JTRC_XMS_CLOSE, info->handle.xms_handle);
  
}


LOCAL(boolean)
open_xms_store (j_common_ptr cinfo, backing_store_ptr info,
		long total_bytes_needed)
{
  XMScontext ctx;

  
  jxms_getdriver((XMSDRIVER far *) & xms_driver);
  if (xms_driver == NULL)
    return FALSE;		

  
  ctx.ax = 0x0000;
  jxms_calldriver(xms_driver, (XMScontext far *) & ctx);
  if (ctx.ax < (unsigned short) 0x0200)
    return FALSE;

  
  ctx.dx = (unsigned short) ((total_bytes_needed + 1023L) >> 10);
  ctx.ax = 0x0900;
  jxms_calldriver(xms_driver, (XMScontext far *) & ctx);
  if (ctx.ax != 1)
    return FALSE;

  
  info->handle.xms_handle = ctx.dx;
  info->read_backing_store = read_xms_store;
  info->write_backing_store = write_xms_store;
  info->close_backing_store = close_xms_store;
  TRACEMS1(cinfo, 1, JTRC_XMS_OPEN, ctx.dx);
  return TRUE;			
}

#endif 






#if EMS_SUPPORTED










typedef void far * EMSPTR;

typedef union {			
	long length;		
	char bytes[18];		
      } EMSspec;


#define FIELD_AT(spec,offset,type)  (*((type *) &(spec.bytes[offset])))
#define SRC_TYPE(spec)		FIELD_AT(spec,4,char)
#define SRC_HANDLE(spec)	FIELD_AT(spec,5,EMSH)
#define SRC_OFFSET(spec)	FIELD_AT(spec,7,unsigned short)
#define SRC_PAGE(spec)		FIELD_AT(spec,9,unsigned short)
#define SRC_PTR(spec)		FIELD_AT(spec,7,EMSPTR)
#define DST_TYPE(spec)		FIELD_AT(spec,11,char)
#define DST_HANDLE(spec)	FIELD_AT(spec,12,EMSH)
#define DST_OFFSET(spec)	FIELD_AT(spec,14,unsigned short)
#define DST_PAGE(spec)		FIELD_AT(spec,16,unsigned short)
#define DST_PTR(spec)		FIELD_AT(spec,14,EMSPTR)

#define EMSPAGESIZE	16384L	/* gospel, see the EMS specs */

#define HIBYTE(W)  (((W) >> 8) & 0xFF)
#define LOBYTE(W)  ((W) & 0xFF)


METHODDEF(void)
read_ems_store (j_common_ptr cinfo, backing_store_ptr info,
		void FAR * buffer_address,
		long file_offset, long byte_count)
{
  EMScontext ctx;
  EMSspec spec;

  spec.length = byte_count;
  SRC_TYPE(spec) = 1;
  SRC_HANDLE(spec) = info->handle.ems_handle;
  SRC_PAGE(spec)   = (unsigned short) (file_offset / EMSPAGESIZE);
  SRC_OFFSET(spec) = (unsigned short) (file_offset % EMSPAGESIZE);
  DST_TYPE(spec) = 0;
  DST_HANDLE(spec) = 0;
  DST_PTR(spec)    = buffer_address;
  
  ctx.ds_si = (void far *) & spec;
  ctx.ax = 0x5700;		
  jems_calldriver((EMScontext far *) & ctx);
  if (HIBYTE(ctx.ax) != 0)
    ERREXIT(cinfo, JERR_EMS_READ);
}


METHODDEF(void)
write_ems_store (j_common_ptr cinfo, backing_store_ptr info,
		 void FAR * buffer_address,
		 long file_offset, long byte_count)
{
  EMScontext ctx;
  EMSspec spec;

  spec.length = byte_count;
  SRC_TYPE(spec) = 0;
  SRC_HANDLE(spec) = 0;
  SRC_PTR(spec)    = buffer_address;
  DST_TYPE(spec) = 1;
  DST_HANDLE(spec) = info->handle.ems_handle;
  DST_PAGE(spec)   = (unsigned short) (file_offset / EMSPAGESIZE);
  DST_OFFSET(spec) = (unsigned short) (file_offset % EMSPAGESIZE);
  
  ctx.ds_si = (void far *) & spec;
  ctx.ax = 0x5700;		
  jems_calldriver((EMScontext far *) & ctx);
  if (HIBYTE(ctx.ax) != 0)
    ERREXIT(cinfo, JERR_EMS_WRITE);
}


METHODDEF(void)
close_ems_store (j_common_ptr cinfo, backing_store_ptr info)
{
  EMScontext ctx;

  ctx.ax = 0x4500;
  ctx.dx = info->handle.ems_handle;
  jems_calldriver((EMScontext far *) & ctx);
  TRACEMS1(cinfo, 1, JTRC_EMS_CLOSE, info->handle.ems_handle);
  
}


LOCAL(boolean)
open_ems_store (j_common_ptr cinfo, backing_store_ptr info,
		long total_bytes_needed)
{
  EMScontext ctx;

  
  if (! jems_available())
    return FALSE;

  
  ctx.ax = 0x4000;
  jems_calldriver((EMScontext far *) & ctx);
  if (HIBYTE(ctx.ax) != 0)
    return FALSE;

  
  ctx.ax = 0x4600;
  jems_calldriver((EMScontext far *) & ctx);
  if (HIBYTE(ctx.ax) != 0 || LOBYTE(ctx.ax) < 0x40)
    return FALSE;

  
  ctx.ax = 0x4300;
  ctx.bx = (unsigned short) ((total_bytes_needed + EMSPAGESIZE-1L) / EMSPAGESIZE);
  jems_calldriver((EMScontext far *) & ctx);
  if (HIBYTE(ctx.ax) != 0)
    return FALSE;

  
  info->handle.ems_handle = ctx.dx;
  info->read_backing_store = read_ems_store;
  info->write_backing_store = write_ems_store;
  info->close_backing_store = close_ems_store;
  TRACEMS1(cinfo, 1, JTRC_EMS_OPEN, ctx.dx);
  return TRUE;			
}

#endif 






GLOBAL(void)
jpeg_open_backing_store (j_common_ptr cinfo, backing_store_ptr info,
			 long total_bytes_needed)
{
  
#if XMS_SUPPORTED
  if (open_xms_store(cinfo, info, total_bytes_needed))
    return;
#endif
#if EMS_SUPPORTED
  if (open_ems_store(cinfo, info, total_bytes_needed))
    return;
#endif
  if (open_file_store(cinfo, info, total_bytes_needed))
    return;
  ERREXITS(cinfo, JERR_TFILE_CREATE, "");
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
  


#ifdef NEED_FHEAPMIN
  _fheapmin();
#endif
}
