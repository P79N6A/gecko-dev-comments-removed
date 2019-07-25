




#ifndef nsLinebreakConverter_h_
#define nsLinebreakConverter_h_

#include "nscore.h"
#include "nsString.h"



class nsLinebreakConverter
{
public:

  
  typedef enum {
    eLinebreakAny,          
    
    eLinebreakPlatform,     
    eLinebreakContent,      
    eLinebreakNet,          
  
    eLinebreakMac,          
    eLinebreakUnix,         
    eLinebreakWindows,      

    eLinebreakSpace         
  
  } ELinebreakType;

  enum {
    kIgnoreLen = -1
  };
  
  











  static char* ConvertLineBreaks(const char* aSrc,
                ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                int32_t aSrcLen = kIgnoreLen, int32_t* aOutLen = nullptr);
  

  











  static PRUnichar* ConvertUnicharLineBreaks(const PRUnichar* aSrc,
                ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                int32_t aSrcLen = kIgnoreLen, int32_t* aOutLen = nullptr);
  

  








  static nsresult ConvertStringLineBreaks(nsString& ioString, ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks);


  














  static nsresult ConvertLineBreaksInSitu(char **ioBuffer, ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                        int32_t aSrcLen = kIgnoreLen, int32_t* aOutLen = nullptr);


  













  static nsresult ConvertUnicharLineBreaksInSitu(PRUnichar **ioBuffer, ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                        int32_t aSrcLen = kIgnoreLen, int32_t* aOutLen = nullptr);
    
};

#endif 
