





#ifndef nsPrimitiveHelpers_h___
#define nsPrimitiveHelpers_h___

#include "prtypes.h"
#include "nsError.h"
#include "nscore.h"

class nsISupports;


class nsPrimitiveHelpers
{
public:

    
    
    
  static void CreatePrimitiveForData ( const char* aFlavor, void* aDataBuff, 
                                         uint32_t aDataLen, nsISupports** aPrimitive ) ;

    
    
    
  static void CreateDataFromPrimitive ( const char* aFlavor, nsISupports* aPrimitive, 
                                         void** aDataBuff, uint32_t aDataLen ) ;

    
    
    
    
  static nsresult ConvertUnicodeToPlatformPlainText ( PRUnichar* inUnicode, int32_t inUnicodeLen, 
                                                    char** outPlainTextData, int32_t* outPlainTextLen ) ;

    
    
    
    
  static nsresult ConvertPlatformPlainTextToUnicode ( const char* inText, int32_t inTextLen, 
                                                    PRUnichar** outUnicode, int32_t* outUnicodeLen ) ;

}; 



class nsLinebreakHelpers
{
public:

    
    
    
    
    
    
  static nsresult ConvertPlatformToDOMLinebreaks ( const char* inFlavor, void** ioData, int32_t* ioLengthInBytes ) ;

}; 


#endif 
