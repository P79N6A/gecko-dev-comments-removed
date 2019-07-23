




































#ifndef WOFF_H_
#define WOFF_H_



#ifdef _MSC_VER 

typedef char           int8_t;
typedef short          int16_t;
typedef int            int32_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
#else
#include <inttypes.h>
#endif

#include <stdio.h> 


enum {
  
  eWOFF_ok = 0,

  
  eWOFF_out_of_memory = 1,       
  eWOFF_invalid = 2,             
  eWOFF_compression_failure = 3, 
  eWOFF_bad_signature = 4,       
  eWOFF_buffer_too_small = 5,    
  eWOFF_bad_parameter = 6,       
  eWOFF_illegal_order = 7,       

  

  eWOFF_warn_unknown_version = 0x0100,   

  eWOFF_warn_checksum_mismatch = 0x0200, 

  eWOFF_warn_misaligned_table = 0x0400,  

  eWOFF_warn_trailing_data = 0x0800,     

  eWOFF_warn_unpadded_table = 0x1000,    

  eWOFF_warn_removed_DSIG = 0x2000       

};





#define WOFF_SUCCESS(status) (((uint32_t)(status) & 0xff) == eWOFF_ok)
#define WOFF_FAILURE(status) (!WOFF_SUCCESS(status))
#define WOFF_WARNING(status) ((uint32_t)(status) & ~0xff)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WOFF_DISABLE_ENCODING








const uint8_t * woffEncode(const uint8_t * sfntData, uint32_t sfntLen,
                           uint16_t majorVersion, uint16_t minorVersion,
                           uint32_t * woffLen, uint32_t * status);











const uint8_t * woffSetMetadata(const uint8_t * woffData, uint32_t * woffLen,
                                const uint8_t * metaData, uint32_t metaLen,
                                uint32_t * status);











const uint8_t * woffSetPrivateData(const uint8_t * woffData, uint32_t * woffLen,
                                   const uint8_t * privData, uint32_t privLen,
                                   uint32_t * status);

#endif 




uint32_t woffGetDecodedSize(const uint8_t * woffData, uint32_t woffLen,
                            uint32_t * pStatus);







void woffDecodeToBuffer(const uint8_t * woffData, uint32_t woffLen,
                        uint8_t * sfntData, uint32_t bufferLen,
                        uint32_t * pActualSfntLen, uint32_t * pStatus);







const uint8_t * woffDecode(const uint8_t * woffData, uint32_t woffLen,
                           uint32_t * sfntLen, uint32_t * status);








const uint8_t * woffGetMetadata(const uint8_t * woffData, uint32_t woffLen,
                                uint32_t * metaLen, uint32_t * status);







const uint8_t * woffGetPrivateData(const uint8_t * woffData, uint32_t woffLen,
                                   uint32_t * privLen, uint32_t * status);







void woffGetFontVersion(const uint8_t * woffData, uint32_t woffLen,
                        uint16_t * major, uint16_t * minor,
                        uint32_t * status);









void woffPrintStatus(FILE * f, uint32_t status, const char * prefix);


#ifdef __cplusplus
}
#endif

#endif
