




































#ifndef nsMacNativeUnicodeConverter_h___
#define nsMacNativeUnicodeConverter_h___

#include "prtypes.h"
#include "nsError.h"
#include "nscore.h"
#include <MacTypes.h>

class nsISupports;


class nsMacNativeUnicodeConverter
{
public:

  
  static nsresult ConvertScripttoUnicode(ScriptCode aScriptCode, 
                                         const char *aMultibyteStr,
                                         PRInt32 aMultibyteStrLen,
                                         PRUnichar **aUnicodeStr,
                                         PRInt32 *aUnicodeStrLen);
                                         
  static nsresult ConvertUnicodetoScript(const PRUnichar *aUnicodeStr, 
                                         PRInt32 aUnicodeStrLen,
                                         char **aMultibyteStr,
                                         PRInt32 *aMultibyteStrlen,
                                         ScriptCodeRun **aScriptCodeRuns,
                                         PRInt32 *aScriptCodeRunLen);

};


#endif 
