




































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
  virtual nsProbingState HandleData(const char* aBuf, PRUint32 aLen) = 0;
  virtual nsProbingState GetState(void) = 0;
  virtual void      Reset(void)  = 0;
  virtual float     GetConfidence(void) = 0;
  virtual void      SetOpion() = 0;

#ifdef DEBUG_chardet
  virtual void  DumpStatus() {};
#endif

  
  
  
  
  static PRBool FilterWithoutEnglishLetters(const char* aBuf, PRUint32 aLen, char** newBuf, PRUint32& newLen);
  static PRBool FilterWithEnglishLetters(const char* aBuf, PRUint32 aLen, char** newBuf, PRUint32& newLen);

};

#endif 
