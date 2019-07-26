



#ifndef nsCharSetProber_h__
#define nsCharSetProber_h__

#include "nscore.h"



typedef enum {
  eDetecting = 0,   
  eFoundIt = 1,     
  eNotMe = 2        
} nsProbingState;

#define SHORTCUT_THRESHOLD      (float)0.95

class nsCharSetProber {
public:
  virtual ~nsCharSetProber() {}
  virtual const char* GetCharSetName() = 0;
  virtual nsProbingState HandleData(const char* aBuf, uint32_t aLen) = 0;
  virtual nsProbingState GetState(void) = 0;
  virtual void      Reset(void)  = 0;
  virtual float     GetConfidence(void) = 0;

#ifdef DEBUG_chardet
  virtual void  DumpStatus() {};
#endif

  
  
  
  
  static bool FilterWithoutEnglishLetters(const char* aBuf, uint32_t aLen, char** newBuf, uint32_t& newLen);
  static bool FilterWithEnglishLetters(const char* aBuf, uint32_t aLen, char** newBuf, uint32_t& newLen);

};

#endif 
