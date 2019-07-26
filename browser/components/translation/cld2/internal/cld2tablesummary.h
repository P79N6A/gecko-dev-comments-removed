


















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_CLD2TABLESUMMARY_H_
#define I18N_ENCODINGS_CLD2_INTERNAL_CLD2TABLESUMMARY_H_

#include "integral_types.h"

namespace CLD2 {



typedef struct {
  uint32 keyvalue[4];   
} IndirectProbBucket4;





typedef struct {
  const IndirectProbBucket4* kCLDTable;
                                      
                                      
  const uint32* kCLDTableInd;         
  uint32 kCLDTableSizeOne;            
  uint32 kCLDTableSize;               
  uint32 kCLDTableKeyMask;            
  uint32 kCLDTableBuildDate;          
  const char* kRecognizedLangScripts; 
                                      
                                      
} CLD2TableSummary;

}       

#endif  


