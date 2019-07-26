



 
#include "nsCharSetProber.h"
#include "prmem.h"


bool nsCharSetProber::FilterWithoutEnglishLetters(const char* aBuf, uint32_t aLen, char** newBuf, uint32_t& newLen)
{
  char *newptr;
  char *prevPtr, *curPtr;
  
  bool meetMSB = false;   
  newptr = *newBuf = (char*)PR_Malloc(aLen);
  if (!newptr)
    return false;

  for (curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++)
  {
    if (*curPtr & 0x80)
    {
      meetMSB = true;
    }
    else if (*curPtr < 'A' || (*curPtr > 'Z' && *curPtr < 'a') || *curPtr > 'z') 
    {
      
      if (meetMSB && curPtr > prevPtr) 
      
      {
        while (prevPtr < curPtr) *newptr++ = *prevPtr++;  
        prevPtr++;
        *newptr++ = ' ';
        meetMSB = false;
      }
      else 
        prevPtr = curPtr+1;
    }
  }
  if (meetMSB && curPtr > prevPtr) 
    while (prevPtr < curPtr) *newptr++ = *prevPtr++;  

  newLen = newptr - *newBuf;

  return true;
}


bool nsCharSetProber::FilterWithEnglishLetters(const char* aBuf, uint32_t aLen, char** newBuf, uint32_t& newLen)
{
  
  char *newptr;
  char *prevPtr, *curPtr;
  bool isInTag = false;

  newptr = *newBuf = (char*)PR_Malloc(aLen);
  if (!newptr)
    return false;

  for (curPtr = prevPtr = (char*)aBuf; curPtr < aBuf+aLen; curPtr++)
  {
    if (*curPtr == '>')
      isInTag = false;
    else if (*curPtr == '<')
      isInTag = true;

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

  return true;
}
