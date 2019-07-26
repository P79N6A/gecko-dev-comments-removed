


















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_LANGSPAN_H_
#define I18N_ENCODINGS_CLD2_INTERNAL_LANGSPAN_H_

#include "generated_language.h"
#include "generated_ulscript.h"

namespace CLD2 {

typedef struct {
  char* text;             
  int text_bytes;         
  int offset;             
  ULScript ulscript;      
  Language lang;          
  bool truncated;         
                          
} LangSpan;

}  
#endif  

