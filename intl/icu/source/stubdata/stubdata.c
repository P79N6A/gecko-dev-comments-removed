


















#include "unicode/utypes.h"
#include "unicode/udata.h"
#include "unicode/uversion.h"


typedef struct {
    uint16_t headerSize;
    uint8_t magic1, magic2;
    UDataInfo info;
    char padding[8];
    uint32_t count, reserved;
    





   int   fakeNameAndData[4];       
                                   
} ICU_Data_Header;

U_EXPORT const ICU_Data_Header U_ICUDATA_ENTRY_POINT = {
    32,          
    0xda,        
    0x27,        
    {            
        sizeof(UDataInfo),      
        0,                      

#if U_IS_BIG_ENDIAN
        1,
#else
        0,
#endif

        U_CHARSET_FAMILY,
        sizeof(UChar),   
        0,               
        {                
           0x54, 0x6f, 0x43, 0x50}, 
           {1, 0, 0, 0},   
           {0, 0, 0, 0}    
    },
    {0,0,0,0,0,0,0,0},   
    0,                  
    0,                  
    {                   

          0 , 0 , 0, 0  
                        

    }
};


