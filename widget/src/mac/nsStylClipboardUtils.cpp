




































#include "nsStylClipboardUtils.h"
#include "nsMemory.h"

#include <Appearance.h>

nsresult CreateStylFromScriptRuns(ScriptCodeRun *scriptCodeRuns,
                                  ItemCount scriptRunOutLen,
                                  char **stylData,
                                  PRInt32 *stylLen)
{
  PRInt32 scrpRecLen = sizeof(short) + sizeof(ScrpSTElement) * scriptRunOutLen;
  StScrpRec *scrpRec = reinterpret_cast<StScrpRec*>(nsMemory::Alloc(scrpRecLen));
  NS_ENSURE_TRUE(scrpRec, NS_ERROR_OUT_OF_MEMORY);
  
  OSErr err = noErr;    
  Str255 themeFontName;
  SInt16 textSize;
  Style textStyle;
  short fontFamilyID;
  FontInfo fontInfo;
  RGBColor textColor;
  textColor.red = textColor.green = textColor.blue = 0;
  
  
  CGrafPtr curPort;
  short saveFontFamilyID;
  SInt16 saveTextSize;
  Style saveTextStyle;
  ::GetPort((GrafPtr*)&curPort);
  saveFontFamilyID = ::GetPortTextFont(curPort);
  saveTextSize = ::GetPortTextSize(curPort);
  saveTextStyle = ::GetPortTextFace(curPort);
  
  scrpRec->scrpNStyles = scriptRunOutLen;
  for (ItemCount i = 0; i < scriptRunOutLen; i++) {
    scrpRec->scrpStyleTab[i].scrpStartChar = scriptCodeRuns[i].offset;
    
    err = ::GetThemeFont(
                         kThemeApplicationFont, 
                         scriptCodeRuns[i].script, 
                         themeFontName, 
                         &textSize, 
                         &textStyle);
    if (err != noErr)
      break;
      
    ::GetFNum(themeFontName, &fontFamilyID);
      
    ::TextFont(fontFamilyID);
    ::TextSize(textSize);
    ::TextFace(textStyle);
    ::GetFontInfo(&fontInfo);
    
    scrpRec->scrpStyleTab[i].scrpFont = fontFamilyID;
    scrpRec->scrpStyleTab[i].scrpHeight = fontInfo.ascent +
                                          fontInfo.descent +
                                          fontInfo.leading;
    scrpRec->scrpStyleTab[i].scrpAscent = fontInfo.ascent;
    scrpRec->scrpStyleTab[i].scrpFace = textStyle;
    scrpRec->scrpStyleTab[i].scrpSize = textSize;
    scrpRec->scrpStyleTab[i].scrpColor = textColor;
  }
       
  
  ::TextFont(saveFontFamilyID);
  ::TextSize(saveTextSize);
  ::TextFace(saveTextStyle);
  
  if (err != noErr) {
    nsMemory::Free(scrpRec);
    return NS_ERROR_FAILURE;
  }

  *stylData = reinterpret_cast<char*>(scrpRec);
  *stylLen = scrpRecLen;
         
  return NS_OK;
}
