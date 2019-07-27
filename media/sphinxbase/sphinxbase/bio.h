

































































#ifndef _S3_BIO_H_
#define _S3_BIO_H_

#include <stdio.h>
#include <stdarg.h>


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/byteorder.h>







#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif

#define BYTE_ORDER_MAGIC	(0x11223344)





#if (__BIG_ENDIAN__)
#define REVERSE_SENSE_SWAP_INT16(x)  x = ( (((x)<<8)&0x0000ff00) | (((x)>>8)&0x00ff) )
#define REVERSE_SENSE_SWAP_INT32(x)  x = ( (((x)<<24)&0xff000000) | (((x)<<8)&0x00ff0000) | \
                         (((x)>>8)&0x0000ff00) | (((x)>>24)&0x000000ff) )
#else
#define REVERSE_SENSE_SWAP_INT16(x)
#define REVERSE_SENSE_SWAP_INT32(x)

#endif

















SPHINXBASE_EXPORT
int32 bio_readhdr (FILE *fp,		
		   char ***name,	
		   char ***val,		
		   int32 *swap	
		   );





SPHINXBASE_EXPORT
int32 bio_writehdr_version (FILE *fp,  
			    char *version 
	);






SPHINXBASE_EXPORT
int32 bio_writehdr(FILE *fp, ...);




SPHINXBASE_EXPORT
void bio_hdrarg_free (char **name,	
		      char **val	
		      );







SPHINXBASE_EXPORT
int32 bio_fread (void *buf,		
		 int32 el_sz,		
		 int32 n_el,		
		 FILE *fp,              
		 int32 swap,		
		 uint32 *chksum	
		 );






SPHINXBASE_EXPORT
int32 bio_fwrite(const void *buf,	
		 int32 el_sz,		
		 int32 n_el,		
		 FILE *fp,              
		 int32 swap,		
		 uint32 *chksum	
		 );











SPHINXBASE_EXPORT
int32 bio_fread_1d (void **buf,		

		    size_t el_sz,	
		    uint32 *n_el,	
		    FILE *fp,		
		    int32 sw,		
		    uint32 *ck	
		    );











SPHINXBASE_EXPORT
int32 bio_fread_2d(void ***arr,
                   size_t e_sz,
                   uint32 *d1,
                   uint32 *d2,
                   FILE *fp,
                   uint32 swap,
                   uint32 *chksum);











SPHINXBASE_EXPORT
int32 bio_fread_3d(void ****arr,
                   size_t e_sz,
                   uint32 *d1,
                   uint32 *d2,
                   uint32 *d3,
                   FILE *fp,
                   uint32 swap,
                   uint32 *chksum);





SPHINXBASE_EXPORT
void bio_verify_chksum (FILE *fp,	
			int32 byteswap,	
			uint32 chksum	
			);









SPHINXBASE_EXPORT
int bio_fwrite_1d(void *arr,       
                  size_t e_sz,     
                  uint32 d1,       
                  FILE *fp,        
                  uint32 *chksum   
                  );







SPHINXBASE_EXPORT
int bio_fwrite_3d(void ***arr,    
                  size_t e_sz,    
                  uint32 d1,      
                  uint32 d2,      
                  uint32 d3,      
                  FILE *fp,       
                  uint32 *chksum  
                  );






SPHINXBASE_EXPORT
int16* bio_read_wavfile(char const *directory, 
			char const *filename,  
			char const *extension, 
			int32 header,          
			int32 endian,          
			size_t *nsamps         
			);

#ifdef __cplusplus
}
#endif

#endif
