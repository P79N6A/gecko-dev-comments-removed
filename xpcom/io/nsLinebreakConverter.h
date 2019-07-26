




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


  











  static char16_t* ConvertUnicharLineBreaks(const char16_t* aSrc,
                                            ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                                            int32_t aSrcLen = kIgnoreLen, int32_t* aOutLen = nullptr);


  








  static nsresult ConvertStringLineBreaks(nsString& aIoString,
                                          ELinebreakType aSrcBreaks,
                                          ELinebreakType aDestBreaks);


  














  static nsresult ConvertLineBreaksInSitu(char** aIoBuffer,
                                          ELinebreakType aSrcBreaks,
                                          ELinebreakType aDestBreaks,
                                          int32_t aSrcLen = kIgnoreLen,
                                          int32_t* aOutLen = nullptr);


  













  static nsresult ConvertUnicharLineBreaksInSitu(char16_t** aIoBuffer,
                                                 ELinebreakType aSrcBreaks,
                                                 ELinebreakType aDestBreaks,
                                                 int32_t aSrcLen = kIgnoreLen,
                                                 int32_t* aOutLen = nullptr);

};

#endif 
