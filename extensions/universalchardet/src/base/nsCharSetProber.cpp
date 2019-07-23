




































 
#include "nsCharSetProber.h"
#include "prmem.h"


PRBool nsCharSetProber::FilterWithoutEnglishLetters(const char* aBuf, PRUint32 aLen, char** newBuf, PRUint32& newLen)
{
  char *newptr;
  char *prevPtr, *curPtr;
  
  PRBool meetMSB = PR_FALSE;   
  newptr = *newBuf = (char*)PR_Malloc(aLen);
  if (!newptr)
    return PR_FALSE;

  for (curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++)
  {
    if (*curPtr & 0x80)
    {
      meetMSB = PR_TRUE;
    }
    else if (*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z') 
    {
      
      if (meetMSB && curPtr > prevPtr) 
      
      {
        while (prevPtr < curPtr) *newptr++ = *prevPtr++;  
        prevPtr++;
        *newptr++ = ' ';
        meetMSB = PR_FALSE;
      }
      else 
        prevPtr = curPtr+1;
    }
  }
  if (meetMSB && curPtr > prevPtr) 
    while (prevPtr < curPtr) *newptr++ = *prevPtr++;  

  newLen = newptr - *newBuf;

  return PR_TRUE;
}


PRBool nsCharSetProber::FilterWithEnglishLetters(const char* aBuf, PRUint32 aLen, char** newBuf, PRUint32& newLen)
{
  
  char *newptr;
  char *prevPtr, *curPtr;
  PRBool isInTag = PR_FALSE;

  newptr = *newBuf = (char*)PR_Malloc(aLen);
  if (!newptr)
    return PR_FALSE;

  for (curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++)
  {
    if (*curPtr == '>')
      isInTag = PR_FALSE;
    else if (*curPtr == '<')
      isInTag = PR_TRUE;

    if (!(*curPtr & 0x80) &&
        (*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z') )
    {
      if (curPtr > prevPtr && !isInTag) 
                                        
      {
        while (prevPtr < curPtr) *newptr++ = *prevPtr++;  
        prevPtr++;
        *newptr++ = ' ';
      }
      else
        prevPtr = curPtr+1;
    }
  }

  
  
  if (!isInTag)
    while (prevPtr < curPtr)
      *newptr++ = *prevPtr++;  

  newLen = newptr - *newBuf;

  return PR_TRUE;
}
