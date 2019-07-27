































EXTERN(void *) jpeg_get_small (j_common_ptr cinfo, size_t sizeofobject);
EXTERN(void) jpeg_free_small (j_common_ptr cinfo, void * object,
                              size_t sizeofobject);









EXTERN(void *) jpeg_get_large (j_common_ptr cinfo, size_t sizeofobject);
EXTERN(void) jpeg_free_large (j_common_ptr cinfo, void * object,
                              size_t sizeofobject);












#ifndef MAX_ALLOC_CHUNK         
#define MAX_ALLOC_CHUNK  1000000000L
#endif























EXTERN(size_t) jpeg_mem_available (j_common_ptr cinfo, size_t min_bytes_needed,
                                   size_t max_bytes_needed,
                                   size_t already_allocated);









#define TEMP_NAME_LENGTH   64   /* max length of a temporary file's name */


#ifdef USE_MSDOS_MEMMGR         

typedef unsigned short XMSH;    
typedef unsigned short EMSH;    

typedef union {
  short file_handle;            
  XMSH xms_handle;              
  EMSH ems_handle;              
} handle_union;

#endif 

#ifdef USE_MAC_MEMMGR           
#include <Files.h>
#endif 


typedef struct backing_store_struct * backing_store_ptr;

typedef struct backing_store_struct {
  
  void (*read_backing_store) (j_common_ptr cinfo, backing_store_ptr info,
                              void * buffer_address, long file_offset,
                              long byte_count);
  void (*write_backing_store) (j_common_ptr cinfo, backing_store_ptr info,
                               void * buffer_address, long file_offset,
                               long byte_count);
  void (*close_backing_store) (j_common_ptr cinfo, backing_store_ptr info);

  
#ifdef USE_MSDOS_MEMMGR
  
  handle_union handle;          
  char temp_name[TEMP_NAME_LENGTH]; 
#else
#ifdef USE_MAC_MEMMGR
  
  short temp_file;              
  FSSpec tempSpec;              
  char temp_name[TEMP_NAME_LENGTH]; 
#else
  
  FILE * temp_file;             
  char temp_name[TEMP_NAME_LENGTH]; 
#endif
#endif
} backing_store_info;










EXTERN(void) jpeg_open_backing_store (j_common_ptr cinfo,
                                      backing_store_ptr info,
                                      long total_bytes_needed);














EXTERN(long) jpeg_mem_init (j_common_ptr cinfo);
EXTERN(void) jpeg_mem_term (j_common_ptr cinfo);
