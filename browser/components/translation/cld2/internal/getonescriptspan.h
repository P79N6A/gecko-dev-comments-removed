


















#ifndef I18N_ENCODINGS_CLD2_INTERNAL_GETONESCRIPTSPAN_H_
#define I18N_ENCODINGS_CLD2_INTERNAL_GETONESCRIPTSPAN_H_

#include "integral_types.h"
#include "langspan.h"
#include "offsetmap.h"

namespace CLD2 {

static const int kMaxScriptBuffer = 40960;
static const int kMaxScriptLowerBuffer = (kMaxScriptBuffer * 3) / 2;
static const int kMaxScriptBytes = kMaxScriptBuffer - 32;   
static const int kWithinScriptTail = 32;    
                                            


static inline bool IsContinuationByte(char c) {
  return static_cast<signed char>(c) < -64;
}



int GetUTF8LetterScriptNum(const char* src);



const char* AdvanceQuad(const char* src);


class ScriptScanner {
 public:
  ScriptScanner(const char* buffer, int buffer_length, bool is_plain_text);
  ScriptScanner(const char* buffer, int buffer_length, bool is_plain_text,
                bool any_text, bool any_script);
  ~ScriptScanner();

  
  bool GetOneScriptSpan(LangSpan* span);

  
  void LowerScriptSpan(LangSpan* span);

  
  
  bool GetOneScriptSpanLower(LangSpan* span);

  
  
  
  bool GetOneTextSpan(LangSpan* span);

  
  
  
  
  
  
  
  
  int MapBack(int text_offset);

  const char* GetBufferStart() {return start_byte_;};

 private:
  
  int SkipToFrontOfSpan(const char* src, int len, int* script);

  const char* start_byte_;        
  const char* next_byte_;         
  const char* next_byte_limit_;   
  int byte_length_;               

  bool is_plain_text_;            
  char* script_buffer_;           
  char* script_buffer_lower_;     
  bool letters_marks_only_;       
                                  
  bool one_script_only_;          
                                  
  int exit_state_;                
                                  
 public :
  
  OffsetMap map2original_;    
  OffsetMap map2uplow_;       
};

}  

#endif  

