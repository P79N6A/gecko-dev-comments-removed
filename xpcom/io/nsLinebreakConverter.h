




































#ifndef nsLinebreakConverter_h_
#define nsLinebreakConverter_h_


#include "nscore.h"
#include "nsString.h"



class NS_COM nsLinebreakConverter
{
public:

  
  typedef enum {
    eLinebreakAny,          
    
    eLinebreakPlatform,     
    eLinebreakContent,      
    eLinebreakNet,          
  
    eLinebreakMac,          
    eLinebreakUnix,         
    eLinebreakWindows       
  
  } ELinebreakType;

  enum {
    kIgnoreLen = -1
  };
  
  











  static char* ConvertLineBreaks(const char* aSrc,
                ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                PRInt32 aSrcLen = kIgnoreLen, PRInt32* aOutLen = nsnull);
  

  











  static PRUnichar* ConvertUnicharLineBreaks(const PRUnichar* aSrc,
                ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                PRInt32 aSrcLen = kIgnoreLen, PRInt32* aOutLen = nsnull);
  

  








  static nsresult ConvertStringLineBreaks(nsString& ioString, ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks);


  














  static nsresult ConvertLineBreaksInSitu(char **ioBuffer, ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                        PRInt32 aSrcLen = kIgnoreLen, PRInt32* aOutLen = nsnull);


  













  static nsresult ConvertUnicharLineBreaksInSitu(PRUnichar **ioBuffer, ELinebreakType aSrcBreaks, ELinebreakType aDestBreaks,
                        PRInt32 aSrcLen = kIgnoreLen, PRInt32* aOutLen = nsnull);
    
};




#endif 
