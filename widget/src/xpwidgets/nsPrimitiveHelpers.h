






































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
                                         PRUint32 aDataLen, nsISupports** aPrimitive ) ;

    
    
    
  static void CreateDataFromPrimitive ( const char* aFlavor, nsISupports* aPrimitive, 
                                         void** aDataBuff, PRUint32 aDataLen ) ;

    
    
    
    
  static nsresult ConvertUnicodeToPlatformPlainText ( PRUnichar* inUnicode, PRInt32 inUnicodeLen, 
                                                    char** outPlainTextData, PRInt32* outPlainTextLen ) ;

    
    
    
    
  static nsresult ConvertPlatformPlainTextToUnicode ( const char* inText, PRInt32 inTextLen, 
                                                    PRUnichar** outUnicode, PRInt32* outUnicodeLen ) ;

}; 



class nsLinebreakHelpers
{
public:

    
    
    
    
    
    
  static nsresult ConvertPlatformToDOMLinebreaks ( const char* inFlavor, void** ioData, PRInt32* ioLengthInBytes ) ;

}; 


#endif 
