







































#include "nsPangoLineBreaker.h"

#include <pango/pango.h>

#include "nsLWBRKDll.h"
#include "nsUnicharUtils.h"
#include "nsUTF8Utils.h"
#include "nsString.h"
#include "nsTArray.h"


NS_IMPL_ISUPPORTS1(nsPangoLineBreaker, nsILineBreaker)

PRBool
nsPangoLineBreaker::BreakInBetween(const PRUnichar* aText1 , PRUint32 aTextLen1,
                                   const PRUnichar* aText2 , PRUint32 aTextLen2)
{
  if (!aText1 || !aText2 || (0 == aTextLen1) || (0 == aTextLen2) ||
      NS_IS_HIGH_SURROGATE(aText1[aTextLen1-1]) && 
      NS_IS_LOW_SURROGATE(aText2[0]) )  
  {
    return PR_FALSE;
  }

  nsAutoString concat(aText1, aTextLen1);
  concat.Append(aText2, aTextLen2);

  nsAutoTArray<PRPackedBool, 2000> breakState;
  if (!breakState.AppendElements(concat.Length()))
    return NS_ERROR_OUT_OF_MEMORY;

  GetJISx4051Breaks(concat.Data(), concat.Length(), breakState.Elements());

  return breakState[aTextLen1];
}


PRInt32
nsPangoLineBreaker::Next(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen > aPos, "Illegal value (length > position)");

  nsAutoTArray<PRPackedBool, 2000> breakState;
  if (!breakState.AppendElements(aLen))
    return NS_ERROR_OUT_OF_MEMORY;

  GetJISx4051Breaks(aText, aLen, breakState.Elements());

  while (++aPos < aLen)
    if (breakState[aPos])
      return aPos;

  return NS_LINEBREAKER_NEED_MORE_TEXT;
}


PRInt32
nsPangoLineBreaker::Prev(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen > aPos, "Illegal value (length > position)");

  nsAutoTArray<PRPackedBool, 2000> breakState;
  if (!breakState.AppendElements(aLen))
    return NS_ERROR_OUT_OF_MEMORY;

  GetJISx4051Breaks(aText, aLen, breakState.Elements());

  while (aPos > 0)
    if (breakState[--aPos])
      return aPos;

  return NS_LINEBREAKER_NEED_MORE_TEXT;
}

void
nsPangoLineBreaker::GetJISx4051Breaks(const PRUnichar* aText, PRUint32 aLen,
                                      PRPackedBool* aBreakBefore)
{
  NS_ASSERTION(aText, "aText shouldn't be null");

  nsAutoTArray<PangoLogAttr, 2000> attrBuffer;
  if (!attrBuffer.AppendElements(aLen + 1))
    return;

  NS_ConvertUTF16toUTF8 aUTF8(aText, aLen);

  const gchar* p = aUTF8.Data();
  const gchar* end = p + aUTF8.Length();
  PRUint32     u16Offset = 0;

  static PangoLanguage* language = pango_language_from_string("en");

  while (p < end)
  {
    PangoLogAttr* attr = attrBuffer.Elements();
    pango_get_log_attrs(p, end - p, -1, language, attr, attrBuffer.Length());

    while (p < end)
    {
      aBreakBefore[u16Offset] = attr->is_line_break;
      if (NS_IS_LOW_SURROGATE(aText[u16Offset]))
        aBreakBefore[++u16Offset] = PR_FALSE; 
      ++u16Offset;

      PRUint32 ch = UTF8CharEnumerator::NextChar(&p, end);
      ++attr;

      if (ch == 0) {
        
        
        
        
        
        break;
      }
    }
  }
}

