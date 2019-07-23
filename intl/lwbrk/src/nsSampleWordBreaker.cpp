





































#include "nsSampleWordBreaker.h"

#include "pratom.h"
#include "nsLWBRKDll.h"
nsSampleWordBreaker::nsSampleWordBreaker()
{
}
nsSampleWordBreaker::~nsSampleWordBreaker()
{
}

NS_IMPL_ISUPPORTS1(nsSampleWordBreaker, nsIWordBreaker)

PRBool nsSampleWordBreaker::BreakInBetween(
  const PRUnichar* aText1 , PRUint32 aTextLen1,
  const PRUnichar* aText2 , PRUint32 aTextLen2)
{
  NS_PRECONDITION( nsnull != aText1, "null ptr");
  NS_PRECONDITION( nsnull != aText2, "null ptr");

  if(!aText1 || !aText2 || (0 == aTextLen1) || (0 == aTextLen2))
    return PR_FALSE;

  return (this->GetClass(aText1[aTextLen1-1]) != this->GetClass(aText2[0]));
}


#define IS_ASCII(c)            (0 == ( 0xFF80 & (c)))
#define ASCII_IS_ALPHA(c)         ((( 'a' <= (c)) && ((c) <= 'z')) || (( 'A' <= (c)) && ((c) <= 'Z')))
#define ASCII_IS_DIGIT(c)         (( '0' <= (c)) && ((c) <= '9'))
#define ASCII_IS_SPACE(c)         (( ' ' == (c)) || ( '\t' == (c)) || ( '\r' == (c)) || ( '\n' == (c)))
#define IS_ALPHABETICAL_SCRIPT(c) ((c) < 0x2E80) 


#define IS_HAN(c)              (( 0x3400 <= (c)) && ((c) <= 0x9fff))||(( 0xf900 <= (c)) && ((c) <= 0xfaff))
#define IS_KATAKANA(c)         (( 0x30A0 <= (c)) && ((c) <= 0x30FF))
#define IS_HIRAGANA(c)         (( 0x3040 <= (c)) && ((c) <= 0x309F))
#define IS_HALFWIDTHKATAKANA(c)         (( 0xFF60 <= (c)) && ((c) <= 0xFF9F))
#define IS_THAI(c)         (0x0E00 == (0xFF80 & (c) )) // Look at the higest 9 bits

PRUint8 nsSampleWordBreaker::GetClass(PRUnichar c)
{
  

  if (IS_ALPHABETICAL_SCRIPT(c))  {
	  if(IS_ASCII(c))  {
		  if(ASCII_IS_SPACE(c)) {
			  return kWbClassSpace;
		  } else if(ASCII_IS_ALPHA(c) || ASCII_IS_DIGIT(c)) {
			  return kWbClassAlphaLetter;
		  } else {
			  return kWbClassPunct;
		  }
	  } else if(IS_THAI(c))	{
		  return kWbClassThaiLetter;
	  } else if (c == 0x00A0) {
      return kWbClassSpace;
    } else {
		  return kWbClassAlphaLetter;
	  }
  }  else {
	  if(IS_HAN(c)) {
		  return kWbClassHanLetter;
	  } else if(IS_KATAKANA(c))   {
		  return kWbClassKatakanaLetter;
	  } else if(IS_HIRAGANA(c))   {
		  return kWbClassHiraganaLetter;
	  } else if(IS_HALFWIDTHKATAKANA(c))  {
		  return kWbClassHWKatakanaLetter;
	  } else  {
		  return kWbClassAlphaLetter;
	  }
  }
  return 0;
}

nsWordRange nsSampleWordBreaker::FindWord(
  const PRUnichar* aText , PRUint32 aTextLen,
  PRUint32 aOffset)
{
  nsWordRange range;
  NS_PRECONDITION( nsnull != aText, "null ptr");
  NS_PRECONDITION( 0 != aTextLen, "len = 0");
  NS_PRECONDITION( aOffset <= aTextLen, "aOffset > aTextLen");

  range.mBegin = aTextLen + 1;
  range.mEnd = aTextLen + 1;

  if(!aText || aOffset > aTextLen)
    return range;

  PRUint8 c = this->GetClass(aText[aOffset]);
  PRUint32 i;
  
  range.mEnd--;
  for(i = aOffset +1;i <= aTextLen; i++)
  {
     if( c != this->GetClass(aText[i]))
     {
       range.mEnd = i;
       break;
     }
  }

  
  range.mBegin = 0;
  for(i = aOffset ;i > 0; i--)
  {
     if( c != this->GetClass(aText[i-1]))
     {
       range.mBegin = i;
       break;
     }
  }
  if(kWbClassThaiLetter == c)
  {
	
	
  }
  return range;
}

PRInt32 nsSampleWordBreaker::NextWord( 
  const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos) 
{
  PRInt8 c1, c2;
  PRUint32 cur = aPos;
  c1 = this->GetClass(aText[cur]);
 
  for(cur++; cur <aLen; cur++)
  {
     c2 = this->GetClass(aText[cur]);
     if(c2 != c1) 
       break;
  }
  if(kWbClassThaiLetter == c1)
  {
	
	
  }
  if (cur == aLen)
    return NS_WORDBREAKER_NEED_MORE_TEXT;
  return cur;
}

PRInt32 nsSampleWordBreaker::PrevWord(
  const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos) 
{
  PRInt8 c1, c2;
  PRUint32 cur = aPos;
  c1 = this->GetClass(aText[cur]);

  for(; cur > 0; cur--)
  {
     c2 = this->GetClass(aText[cur-1]);
     if(c2 != c1)
       break;
  }
  if(kWbClassThaiLetter == c1)
  {
	
	
  }
  if (!cur)
    return NS_WORDBREAKER_NEED_MORE_TEXT;
  return cur;
}
