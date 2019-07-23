




































#include "nsMemory.h"
#include "nsMacNativeUnicodeConverter.h"

#include <TextUtils.h>
#include <UnicodeConverter.h>
#include <Fonts.h>





nsresult nsMacNativeUnicodeConverter::ConvertScripttoUnicode(ScriptCode aScriptCode, 
                                                             const char *aMultibyteStr,
                                                             PRInt32 aMultibyteStrLen,
                                                             PRUnichar **aUnicodeStr,
                                                             PRInt32 *aUnicodeStrLen)
{
  NS_ENSURE_ARG(aUnicodeStr);
  NS_ENSURE_ARG(aMultibyteStr);

  *aUnicodeStr = nsnull;
  *aUnicodeStrLen = 0;
	
  TextEncoding textEncodingFromScript;
  OSErr err = ::UpgradeScriptInfoToTextEncoding(aScriptCode, 
                                                kTextLanguageDontCare, 
                                                kTextRegionDontCare, 
                                                nsnull,
                                                &textEncodingFromScript);
  NS_ENSURE_TRUE(err == noErr, NS_ERROR_FAILURE);
	
  TextToUnicodeInfo	textToUnicodeInfo;
  err = ::CreateTextToUnicodeInfoByEncoding(textEncodingFromScript, &textToUnicodeInfo);
  NS_ENSURE_TRUE(err == noErr, NS_ERROR_FAILURE);

  
  
  UInt32 factor;
  if (aScriptCode == smArabic ||
      aScriptCode == smHebrew ||
      aScriptCode == smExtArabic)
    factor = 6;
  else
    factor = 2;
    
  UniChar *unicodeStr = (UniChar *) nsMemory::Alloc(aMultibyteStrLen * factor);
  if (!unicodeStr) {
    ::DisposeTextToUnicodeInfo(&textToUnicodeInfo);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  ByteCount sourceRead;
  ByteCount unicodeLen;
  
  err = ::ConvertFromTextToUnicode(textToUnicodeInfo,
                                   aMultibyteStrLen,
                                   (ConstLogicalAddress) aMultibyteStr,
                                   kUnicodeUseFallbacksMask | 
                                   kUnicodeLooseMappingsMask,
                                   0,nsnull,nsnull,nsnull,
                                   aMultibyteStrLen * factor,
                                   &sourceRead,
                                   &unicodeLen,
                                   unicodeStr);
				
  if (err == noErr)
  {
    *aUnicodeStr = (PRUnichar *) unicodeStr;
    *aUnicodeStrLen = unicodeLen / sizeof(UniChar); 
  }
          									
  ::DisposeTextToUnicodeInfo(&textToUnicodeInfo);
  										                          
  return err == noErr ? NS_OK : NS_ERROR_FAILURE;                                  
}

nsresult nsMacNativeUnicodeConverter::ConvertUnicodetoScript(const PRUnichar *aUnicodeStr, 
                                                             PRInt32 aUnicodeStrLen,
                                                             char **aMultibyteStr,
                                                             PRInt32 *aMultibyteStrlen,
                                                             ScriptCodeRun **aScriptCodeRuns,
                                                             PRInt32 *aScriptCodeRunLen)
{
  NS_ENSURE_ARG(aUnicodeStr);
  NS_ENSURE_ARG(aMultibyteStr);
  NS_ENSURE_ARG(aMultibyteStrlen);
  NS_ENSURE_ARG(aScriptCodeRuns);
  NS_ENSURE_ARG(aScriptCodeRunLen);
  
  *aMultibyteStr = nsnull;
  *aMultibyteStrlen = 0;
  *aScriptCodeRuns = nsnull;
  *aScriptCodeRunLen = 0;
  
  
  ItemCount numberOfScriptCodes = ::GetScriptManagerVariable(smEnabled);
  ScriptCode *scriptArray = (ScriptCode *) nsMemory::Alloc(sizeof(ScriptCode) * numberOfScriptCodes);
  NS_ENSURE_TRUE(scriptArray, NS_ERROR_OUT_OF_MEMORY);
	
  for (ScriptCode i = 0, j = 0; i <= smUninterp && j < numberOfScriptCodes; i++)
  {
    if (::GetScriptVariable(i, smScriptEnabled))
      scriptArray[j++] = i;
  }

  OSErr err;
  UnicodeToTextRunInfo unicodeToTextInfo;
  err = ::CreateUnicodeToTextRunInfoByScriptCode(numberOfScriptCodes,
                                                 scriptArray,
                                                 &unicodeToTextInfo); 
  nsMemory::Free(scriptArray);                                                 
  NS_ENSURE_TRUE(err == noErr, NS_ERROR_FAILURE);
                                              
  ByteCount inputRead;
  ItemCount scriptRunOutLen;
  ScriptCodeRun *scriptCodeRuns = NS_REINTERPRET_CAST(ScriptCodeRun*,
                                                      nsMemory::Alloc(sizeof(ScriptCodeRun) * 
                                                      aUnicodeStrLen));
  if (!scriptCodeRuns) {
    ::DisposeUnicodeToTextRunInfo(&unicodeToTextInfo);
    return NS_ERROR_OUT_OF_MEMORY;
  }
	
  
  
  LogicalAddress outputStr = (LogicalAddress) nsMemory::Alloc(aUnicodeStrLen * 2);
  if (!outputStr)
  {
    ::DisposeUnicodeToTextRunInfo(&unicodeToTextInfo);
    nsMemory::Free(scriptCodeRuns);
    return NS_ERROR_OUT_OF_MEMORY;
  }
  ByteCount outputLen = 0;

  err = ::ConvertFromUnicodeToScriptCodeRun(unicodeToTextInfo,
                                            aUnicodeStrLen * sizeof(UniChar),
                                            (UniChar *) aUnicodeStr,
                                            kUnicodeUseFallbacksMask | 
                                            kUnicodeLooseMappingsMask |
                                            kUnicodeTextRunMask,
                                            0,
                                            nsnull,
                                            nsnull,
                                            nsnull,
                                            aUnicodeStrLen * sizeof(PRUnichar), 
                                            &inputRead,
                                            &outputLen,
                                            outputStr,
                                            aUnicodeStrLen,
                                            &scriptRunOutLen,
                                            scriptCodeRuns);
                                                   
  if (outputLen > 0 &&
      (err == noErr ||
       err == kTECUnmappableElementErr ||
       err == kTECOutputBufferFullStatus))
  {
    
    
    *aMultibyteStr = (char *) outputStr;
    *aMultibyteStrlen = outputLen;
    *aScriptCodeRuns = scriptCodeRuns;
    *aScriptCodeRunLen = scriptRunOutLen;
    err = noErr;
  }

  ::DisposeUnicodeToTextRunInfo(&unicodeToTextInfo);
  
  return err == noErr ? NS_OK : NS_ERROR_FAILURE;                                  
}
	
