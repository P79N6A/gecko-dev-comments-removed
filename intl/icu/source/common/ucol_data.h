






















#ifndef __UCOL_DATA_H__
#define __UCOL_DATA_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION


#define UCOL_HEADER_MAGIC 0x20030618

typedef struct {
      int32_t size;
      
      
      uint32_t options; 
      uint32_t UCAConsts; 
      uint32_t contractionUCACombos;        
      uint32_t magic;            
      uint32_t mappingPosition;  
      uint32_t expansion;        
      uint32_t contractionIndex; 
      uint32_t contractionCEs;   
      uint32_t contractionSize;  
       

      uint32_t endExpansionCE;      

      uint32_t expansionCESize;     



      int32_t  endExpansionCECount; 
      uint32_t unsafeCP;            
      uint32_t contrEndCP;          
                                    

      int32_t contractionUCACombosSize;     
                                            
      UBool jamoSpecial;                    
      UBool isBigEndian;                    
      uint8_t charSetFamily;                
      uint8_t contractionUCACombosWidth;    
      UVersionInfo version;
      UVersionInfo UCAVersion;              
      UVersionInfo UCDVersion;              
      UVersionInfo formatVersion;           
      uint32_t scriptToLeadByte;            
      uint32_t leadByteToScript;            
      uint8_t reserved[76];                 
} UCATableHeader;

typedef struct {
  uint32_t byteSize;
  uint32_t tableSize;
  uint32_t contsSize;
  uint32_t table;
  uint32_t conts;
  UVersionInfo UCAVersion;              
  uint8_t padding[8];
} InverseUCATableHeader;

#endif  

#endif  
