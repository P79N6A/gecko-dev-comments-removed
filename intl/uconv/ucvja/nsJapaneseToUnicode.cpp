



#include "nsJapaneseToUnicode.h"

#include "nsUCSupport.h"

#include "japanese.map"

#include "nsICharsetConverterManager.h"
#include "nsServiceManagerUtils.h"

#include "mozilla/Assertions.h"

#ifdef XP_OS2
  
  
  
  
  #define SJIS_INDEX gIBM943Index[0]
  #define JIS0208_INDEX gIBM943Index[1]
#else
  
  #define SJIS_INDEX gCP932Index[0]
  #define JIS0208_INDEX gCP932Index[1]
#endif

#define JIS0212_INDEX gJIS0212Index
#define SJIS_UNMAPPED	0x30fb
#define UNICODE_REPLACEMENT_CHARACTER 0xfffd
#define IN_GR_RANGE(b) \
  ((uint8_t(0xa1) <= uint8_t(b)) && (uint8_t(b) <= uint8_t(0xfe)))

NS_IMETHODIMP nsShiftJISToUnicode::Convert(
   const char * aSrc, int32_t * aSrcLen,
     char16_t * aDest, int32_t * aDestLen)
{
   static const uint8_t sbIdx[256] =
   {
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  
        0,    1,    2,    3,    4,    5,    6,    7,  
        8,    9,   10,   11,   12,   13,   14,   15,  
       16,   17,   18,   19,   20,   21,   22,   23,  
       24,   25,   26,   27,   28,   29,   30,   31,  
       32,   33,   34,   35,   36,   37,   38,   39,  
       40,   41,   42,   43,   44,   45,   46,   47,  
       48,   49,   50,   51,   52,   53,   54,   55,  
       56,   57,   58,   59,   60,   61,   62, 0xFF,  
       63,   64,   65,   66,   67,   68,   69,   70,  
       71,   72,   73,   74,   75,   76,   77,   78,  
       79,   80,   81,   82,   83,   84,   85,   86,  
       87,   88,   89,   90,   91,   92,   93,   94,  
       95,   96,   97,   98,   99,  100,  101,  102,  
      103,  104,  105,  106,  107,  108,  109,  110,  
      111,  112,  113,  114,  115,  116,  117,  118,  
      119,  120,  121,  122,  123,  124,  125,  126,  
      127,  128,  129,  130,  131,  132,  133,  134,  
      135,  136,  137,  138,  139,  140,  141,  142,  
      143,  144,  145,  146,  147,  148,  149,  150,  
      151,  152,  153,  154,  155,  156,  157,  158,  
      159,  160,  161,  162,  163,  164,  165,  166,  
      167,  168,  169,  170,  171,  172,  173,  174,  
      175,  176,  177,  178,  179,  180,  181,  182,  
      183,  184,  185,  186,  187, 0xFF, 0xFF, 0xFF,  
   };

   const unsigned char* srcEnd = (unsigned char*)aSrc + *aSrcLen;
   const unsigned char* src =(unsigned char*) aSrc;
   char16_t* destEnd = aDest + *aDestLen;
   char16_t* dest = aDest;
   while (src < srcEnd) {
       switch (mState) {
          case 0:
          if (*src <= 0x80) {
            
            *dest++ = (char16_t) *src;
            if (dest >= destEnd) {
              goto error1;
            }
          } else {
            mData = SJIS_INDEX[*src & 0x7F];
            if (mData < 0xE000) {
              mState = 1; 
            } else if (mData < 0xF000) {
              mState = 2; 
            } else {
              *dest++ = mData; 
              if (dest >= destEnd) {
                goto error1;
              }
            }
          }
          break;

          case 1: 
          {
            MOZ_ASSERT(mData < 0xE000);
            uint8_t off = sbIdx[*src];

            
            
            
            
            
            if(0xFF == off) {
               src--;
               if (mErrBehavior == kOnError_Signal)
                 goto error_invalidchar;
               *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
               char16_t ch = gJapaneseMap[mData+off];
               if(ch == 0xfffd) {
                 if (mErrBehavior == kOnError_Signal)
                   goto error_invalidchar;
                 ch = SJIS_UNMAPPED;
               }
               *dest++ = ch;
            }
            mState = 0;
            if(dest >= destEnd)
              goto error1;
          }
          break;

          case 2: 
          {
            MOZ_ASSERT(0xE000 <= mData && mData < 0xF000);
            uint8_t off = sbIdx[*src];

            
            if(0xFF == off) {
               src--;
               if (mErrBehavior == kOnError_Signal)
                 goto error_invalidchar;

               *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
               *dest++ = mData + off;
            }
            mState = 0;
            if(dest >= destEnd)
              goto error1;
          }
          break;

       }
       src++;
   }
   *aDestLen = dest - aDest;
   return NS_OK;
error_invalidchar:
   *aDestLen = dest - aDest;
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_ERROR_ILLEGAL_INPUT;
error1:
   *aDestLen = dest - aDest;
   src++;
   if ((mState == 0) && (src == srcEnd)) {
     return NS_OK;
   }
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_OK_UDEC_MOREOUTPUT;
}

char16_t
nsShiftJISToUnicode::GetCharacterForUnMapped()
{
  return char16_t(SJIS_UNMAPPED);
}

NS_IMETHODIMP nsEUCJPToUnicodeV2::Convert(
   const char * aSrc, int32_t * aSrcLen,
     char16_t * aDest, int32_t * aDestLen)
{
   static const uint8_t sbIdx[256] =
   {

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0,    1,    2,    3,    4,    5,    6,  
     7,    8 ,   9,    10,   11,   12,   13,   14,

     15,   16,   17,   18,   19,   20,   21,   22, 
     23,   24,   25,   26,   27,   28,   29,   30, 

     31,   32,   33,   34,   35,   36,   37,   38, 
     39,   40,   41,   42,   43,   44,   45,   46, 

     47,   48,   49,   50,   51,   52,   53,   54, 
     55,   56,   57,   58,   59,   60,   61,   62, 

     63,   64,   65,   66,   67,   68,   69,   70, 
     71,   72,   73,   74,   75,   76,   77,   78, 

     79,   80,   81,   82,   83,   84,   85,   86, 
     87,   88,   89,   90,   91,   92,   93,   0xFF, 
   };

   const unsigned char* srcEnd = (unsigned char*)aSrc + *aSrcLen;
   const unsigned char* src =(unsigned char*) aSrc;
   char16_t* destEnd = aDest + *aDestLen;
   char16_t* dest = aDest;
   while((src < srcEnd))
   {
       switch(mState)
       {
          case 0:
          if(*src & 0x80  && *src != (unsigned char)0xa0)
          {
            mData = JIS0208_INDEX[*src & 0x7F];
            if(mData != 0xFFFD )
            {
               mState = 1; 
            } else {
               if( 0x8e == *src) {
                 
                 mState = 2; 
               } else if(0x8f == *src) {
                 
                 mState = 3; 
               } else {
                 
                 if (mErrBehavior == kOnError_Signal)
                   goto error_invalidchar;
                 *dest++ = 0xFFFD;
                 if(dest >= destEnd)
                   goto error1;
               }
            }
          } else {
            
            *dest++ = (char16_t) *src;
            if(dest >= destEnd)
              goto error1;
          }
          break;

          case 1: 
          {
            uint8_t off = sbIdx[*src];
            if(0xFF == off) {
              if (mErrBehavior == kOnError_Signal)
                goto error_invalidchar;
              *dest++ = 0xFFFD;
               
               
               
              if ( (uint8_t)*src < (uint8_t)0x7f )
                --src;
            } else {
               *dest++ = gJapaneseMap[mData+off];
            }
            mState = 0;
            if(dest >= destEnd)
              goto error1;
          }
          break;

          case 2: 
          {
            if((0xA1 <= *src) && (*src <= 0xDF)) {
              *dest++ = (0xFF61-0x00A1) + *src;
            } else {
              if (mErrBehavior == kOnError_Signal)
                goto error_invalidchar;
              *dest++ = 0xFFFD;             
              
              
              if ( (uint8_t)*src < (uint8_t)0x7f )
                --src;
            }
            mState = 0;
            if(dest >= destEnd)
              goto error1;
          }
          break;

          case 3: 
          {
            if (IN_GR_RANGE(*src))
            {
              mData = JIS0212_INDEX[*src & 0x7F];
              if(mData != 0xFFFD )
              {
                 mState = 4; 
              } else {
                 mState = 5; 
              }
            } else {
              
              if (mErrBehavior == kOnError_Signal)
                goto error_invalidchar;
              *dest++ = 0xFFFD;
              --src;
              mState = 0;
              if(dest >= destEnd)
                goto error1;
            }
          }
          break;
          case 4:
          {
            uint8_t off = sbIdx[*src];
            if(0xFF != off) {
              *dest++ = gJapaneseMap[mData+off];
              mState = 0;
              if(dest >= destEnd)
                goto error1;
              break;
            }
            
          }
          case 5: 
          {
            if (mErrBehavior == kOnError_Signal)
              goto error_invalidchar;
            *dest++ = 0xFFFD;
            
            
            
            if (!IN_GR_RANGE(*src))
              --src;
            mState = 0;
            if(dest >= destEnd)
              goto error1;
          }
          break;
       }
       src++;
   }
   *aDestLen = dest - aDest;
   return NS_OK;
error_invalidchar:
   *aDestLen = dest - aDest;
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_ERROR_ILLEGAL_INPUT;
error1:
   *aDestLen = dest - aDest;
   src++;
   if ((mState == 0) && (src == srcEnd)) {
     return NS_OK;
   } 
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_OK_UDEC_MOREOUTPUT;
}



NS_IMETHODIMP nsISO2022JPToUnicodeV2::Convert(
   const char * aSrc, int32_t * aSrcLen,
     char16_t * aDest, int32_t * aDestLen)
{
   static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

   static const uint16_t fbIdx[128] =
   {

     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,

     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,
     0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD, 0xFFFD,

     0xFFFD, 0,      94,     94* 2,  94* 3,  94* 4,  94* 5,  94* 6,  
     94* 7,  94* 8 , 94* 9,  94*10,  94*11,  94*12,  94*13,  94*14,

     94*15,  94*16,  94*17,  94*18,  94*19,  94*20,  94*21,  94*22,
     94*23,  94*24,  94*25,  94*26,  94*27,  94*28,  94*29,  94*30,

     94*31,  94*32,  94*33,  94*34,  94*35,  94*36,  94*37,  94*38,
     94*39,  94*40,  94*41,  94*42,  94*43,  94*44,  94*45,  94*46,

     94*47,  94*48,  94*49,  94*50,  94*51,  94*52,  94*53,  94*54,
     94*55,  94*56,  94*57,  94*58,  94*59,  94*60,  94*61,  94*62,

     94*63,  94*64,  94*65,  94*66,  94*67,  94*68,  94*69,  94*70,
     94*71,  94*72,  94*73,  94*74,  94*75,  94*76,  94*77,  94*78,

     94*79,  94*80,  94*81,  94*82,  94*83,  94*84,  94*85,  94*86,
     94*87,  94*88,  94*89,  94*90,  94*91,  94*92,  94*93,  0xFFFD,
   };
   static const uint8_t sbIdx[256] =
   {

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0,    1,    2,    3,    4,    5,    6,  
     7,    8 ,   9,    10,   11,   12,   13,   14,

     15,   16,   17,   18,   19,   20,   21,   22, 
     23,   24,   25,   26,   27,   28,   29,   30, 

     31,   32,   33,   34,   35,   36,   37,   38, 
     39,   40,   41,   42,   43,   44,   45,   46, 

     47,   48,   49,   50,   51,   52,   53,   54, 
     55,   56,   57,   58,   59,   60,   61,   62, 

     63,   64,   65,   66,   67,   68,   69,   70, 
     71,   72,   73,   74,   75,   76,   77,   78, 

     79,   80,   81,   82,   83,   84,   85,   86, 
     87,   88,   89,   90,   91,   92,   93,   0xFF, 

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   };

   const unsigned char* srcEnd = (unsigned char*)aSrc + *aSrcLen;
   const unsigned char* src =(unsigned char*) aSrc;
   char16_t* destEnd = aDest + *aDestLen;
   char16_t* dest = aDest;
   while((src < srcEnd))
   {
     
       switch(mState)
       {
          case mState_ASCII:
            if(0x1b == *src)
            {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              if (CHECK_OVERRUN(dest, destEnd, 1))
                goto error1;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
              if (CHECK_OVERRUN(dest, destEnd, 1))
                goto error1;
              *dest++ = (char16_t) *src;
            }
          break;
          
          case mState_ESC:
            if( '(' == *src) {
              mState = mState_ESC_28;
            } else if ('$' == *src)  {
              mState = mState_ESC_24;
            } else if ('.' == *src)  { 
              mState = mState_ESC_2e;
            } else if ('N' == *src)  { 
              mState = mState_ESC_4e;
            } else  {
              if (CHECK_OVERRUN(dest, destEnd, 2))
                goto error1;
              *dest++ = (char16_t) 0x1b;
              if (0x80 & *src) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              } else {
                *dest++ = (char16_t) *src;
              }
              mState = mLastLegalState;
            }
          break;

          case mState_ESC_28: 
            if( 'B' == *src) {
              mState = mState_ASCII;
              if (mRunLength == 0) {
                if (CHECK_OVERRUN(dest, destEnd, 1))
                  goto error1;
                *dest++ = 0xFFFD;
              }
              mRunLength = 0;
            } else if ('J' == *src)  {
              mState = mState_JISX0201_1976Roman;
              if (mRunLength == 0 && mLastLegalState != mState_ASCII) {
                if (CHECK_OVERRUN(dest, destEnd, 1))
                  goto error1;
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = 0xFFFD;
              }
              mRunLength = 0;
            } else if ('I' == *src)  {
              mState = mState_JISX0201_1976Kana;
              mRunLength = 0;
            } else  {
              if (CHECK_OVERRUN(dest, destEnd, 3))
                goto error1;
              *dest++ = (char16_t) 0x1b;
              *dest++ = (char16_t) '(';
              if (0x80 & *src) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              } else {
                *dest++ = (char16_t) *src;
              }
              mState = mLastLegalState;
            }
          break;

          case mState_ESC_24: 
            if( '@' == *src) {
              mState = mState_JISX0208_1978;
              mRunLength = 0;
            } else if ('A' == *src)  {
              mState = mState_GB2312_1980;
              mRunLength = 0;
            } else if ('B' == *src)  {
              mState = mState_JISX0208_1983;
              mRunLength = 0;
            } else if ('(' == *src)  {
              mState = mState_ESC_24_28;
            } else  {
              if (CHECK_OVERRUN(dest, destEnd, 3))
                goto error1;
              *dest++ = (char16_t) 0x1b;
              *dest++ = (char16_t) '$';
              if (0x80 & *src) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              } else {
                *dest++ = (char16_t) *src;
              }
              mState = mLastLegalState;
            }
          break;

          case mState_ESC_24_28: 
            if( 'C' == *src) {
              mState = mState_KSC5601_1987;
              mRunLength = 0;
            } else if ('D' == *src) {
              mState = mState_JISX0212_1990;
              mRunLength = 0;
            } else  {
              if (CHECK_OVERRUN(dest, destEnd, 4))
                goto error1;
              *dest++ = (char16_t) 0x1b;
              *dest++ = (char16_t) '$';
              *dest++ = (char16_t) '(';
              if (0x80 & *src) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              } else {
                *dest++ = (char16_t) *src;
              }
              mState = mLastLegalState;
            }
          break;

          case mState_JISX0201_1976Roman:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              if (CHECK_OVERRUN(dest, destEnd, 1))
                goto error1;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              ++mRunLength;
            } else {
              
              
              
              if (CHECK_OVERRUN(dest, destEnd, 1))
                goto error1;
              *dest++ = (char16_t) *src;
              ++mRunLength;
            }
          break;

          case mState_JISX0201_1976Kana:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else {
              if (CHECK_OVERRUN(dest, destEnd, 1))
                goto error1;
              if((0x21 <= *src) && (*src <= 0x5F)) {
                *dest++ = (0xFF61-0x0021) + *src;
              } else {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              }
              ++mRunLength;
            }
          break;

          case mState_JISX0208_1978:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              mLastLegalState = mState;
              mState = mState_ERROR;
            } else {
              mData = JIS0208_INDEX[*src & 0x7F];
              if (0xFFFD == mData) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                mState = mState_ERROR;
              } else {
                mState = mState_JISX0208_1978_2ndbyte;
              }
            }
          break;

          case mState_GB2312_1980:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              mLastLegalState = mState;
              mState = mState_ERROR;
            } else {
              mData = fbIdx[*src & 0x7F];
              if (0xFFFD == mData) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                mState = mState_ERROR;
              } else {
                mState = mState_GB2312_1980_2ndbyte;
              }
            }
          break;

          case mState_JISX0208_1983:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              mLastLegalState = mState;
              mState = mState_ERROR;
            } else {
              mData = JIS0208_INDEX[*src & 0x7F];
              if (0xFFFD == mData) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                mState = mState_ERROR;
              } else {
                mState = mState_JISX0208_1983_2ndbyte;
              }
            }
          break;

          case mState_KSC5601_1987:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              mLastLegalState = mState;
              mState = mState_ERROR;
            } else {
              mData = fbIdx[*src & 0x7F];
              if (0xFFFD == mData) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                mState = mState_ERROR;
              } else {
                mState = mState_KSC5601_1987_2ndbyte;
              }
            }
          break;

          case mState_JISX0212_1990:
            if(0x1b == *src) {
              mLastLegalState = mState;
              mState = mState_ESC;
            } else if(*src & 0x80) {
              mLastLegalState = mState;
              mState = mState_ERROR;
            } else {
              mData = JIS0212_INDEX[*src & 0x7F];
              if (0xFFFD == mData) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                mState = mState_ERROR;
              } else {
                mState = mState_JISX0212_1990_2ndbyte;
              }
            }
          break;

          case mState_JISX0208_1978_2ndbyte:
          {
            if (CHECK_OVERRUN(dest, destEnd, 1))
              goto error1;
            uint8_t off = sbIdx[*src];
            if(0xFF == off) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
               
               
              *dest++ = gJapaneseMap[mData+off];
            }
            ++mRunLength;
            mState = mState_JISX0208_1978;
          }
          break;

          case mState_GB2312_1980_2ndbyte:
          {
            if (CHECK_OVERRUN(dest, destEnd, 1))
              goto error1;
            uint8_t off = sbIdx[*src];
            if(0xFF == off) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
              if (!mGB2312Decoder) {
                
                nsresult rv;
                nsCOMPtr<nsICharsetConverterManager> ccm = 
                         do_GetService(kCharsetConverterManagerCID, &rv);
                if (NS_SUCCEEDED(rv)) {
                  rv = ccm->GetUnicodeDecoderRaw("GB2312", &mGB2312Decoder);
                }
              }
              if (!mGB2312Decoder) {
                goto error2;
              } else {
                unsigned char gb[2];
                char16_t uni;
                int32_t gbLen = 2, uniLen = 1;
                
                
                
                gb[0] = ((mData / 94) + 0x21) | 0x80;
                gb[1] = *src | 0x80;
                
                mGB2312Decoder->Convert((const char *)gb, &gbLen,
                                        &uni, &uniLen);
                *dest++ = uni;
              }
            }
            ++mRunLength;
            mState = mState_GB2312_1980;
          }
          break;

          case mState_JISX0208_1983_2ndbyte:
          {
            if (CHECK_OVERRUN(dest, destEnd, 1))
              goto error1;
            uint8_t off = sbIdx[*src];
            if(0xFF == off) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
              *dest++ = gJapaneseMap[mData+off];
            }
            ++mRunLength;
            mState = mState_JISX0208_1983;
          }
          break;

          case mState_KSC5601_1987_2ndbyte:
          {
            if (CHECK_OVERRUN(dest, destEnd, 1))
              goto error1;
            uint8_t off = sbIdx[*src];
            if(0xFF == off) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
              if (!mEUCKRDecoder) {
                
                nsresult rv;
                nsCOMPtr<nsICharsetConverterManager> ccm = 
                         do_GetService(kCharsetConverterManagerCID, &rv);
                if (NS_SUCCEEDED(rv)) {
                  rv = ccm->GetUnicodeDecoderRaw("EUC-KR", &mEUCKRDecoder);
                }
              }
              if (!mEUCKRDecoder) {
                goto error2;
              } else {              
                unsigned char ksc[2];
                char16_t uni;
                int32_t kscLen = 2, uniLen = 1;
                
                
                
                ksc[0] = ((mData / 94) + 0x21) | 0x80;
                ksc[1] = *src | 0x80;
                
                mEUCKRDecoder->Convert((const char *)ksc, &kscLen,
                                       &uni, &uniLen);
                *dest++ = uni;
              }
            }
            ++mRunLength;
            mState = mState_KSC5601_1987;
          }
          break;

          case mState_JISX0212_1990_2ndbyte:
          {
            uint8_t off = sbIdx[*src];
            if (CHECK_OVERRUN(dest, destEnd, 1))
              goto error1;
            if(0xFF == off) {
              if (mErrBehavior == kOnError_Signal)
                goto error3;
              *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            } else {
              *dest++ = gJapaneseMap[mData+off];
            }
            ++mRunLength;
            mState = mState_JISX0212_1990;
          }
          break;

          case mState_ESC_2e: 
            
            mState = mLastLegalState;
            if( 'A' == *src) {
              G2charset = G2_ISO88591;
            } else if ('F' == *src) {
              G2charset = G2_ISO88597;
            } else  {
              if (CHECK_OVERRUN(dest, destEnd, 3))
                goto error1;
              *dest++ = (char16_t) 0x1b;
              *dest++ = (char16_t) '.';
              if (0x80 & *src) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              } else {
                *dest++ = (char16_t) *src;
              }
            }
          break;

          case mState_ESC_4e: 
            
            
            
            mState = mLastLegalState;
            if((0x20 <= *src) && (*src <= 0x7F)) {
              if (CHECK_OVERRUN(dest, destEnd, 1))
                goto error1;
              if (G2_ISO88591 == G2charset) {
                *dest++ = *src | 0x80;
              } else if (G2_ISO88597 == G2charset) {
                if (!mISO88597Decoder) {
                  
                  nsresult rv;
                  nsCOMPtr<nsICharsetConverterManager> ccm = 
                           do_GetService(kCharsetConverterManagerCID, &rv);
                  if (NS_SUCCEEDED(rv)) {
                    rv = ccm->GetUnicodeDecoderRaw("ISO-8859-7", &mISO88597Decoder);
                  }
                }
                if (!mISO88597Decoder) {
                  goto error2;
                } else {
                  
                  unsigned char gr = *src | 0x80;
                  char16_t uni;
                  int32_t grLen = 1, uniLen = 1;
                  
                  mISO88597Decoder->Convert((const char *)&gr, &grLen,
                                            &uni, &uniLen);
                  *dest++ = uni;
                }
              } else {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              }
              ++mRunLength;
            } else {
              if (CHECK_OVERRUN(dest, destEnd, 3))
                goto error1;
              *dest++ = (char16_t) 0x1b;
              *dest++ = (char16_t) 'N';
              if (0x80 & *src) {
                if (mErrBehavior == kOnError_Signal)
                  goto error3;
                *dest++ = UNICODE_REPLACEMENT_CHARACTER;
              } else {
                *dest++ = (char16_t) *src;
              }
            }
          break;

          case mState_ERROR:
            mState = mLastLegalState;
            if (mErrBehavior == kOnError_Signal) {
              mRunLength = 0;
              goto error3;
            }
            if (CHECK_OVERRUN(dest, destEnd, 1))
              goto error1;
            *dest++ = UNICODE_REPLACEMENT_CHARACTER;
            ++mRunLength;
          break;

       } 
       src++;
   }
   *aDestLen = dest - aDest;
   return NS_OK;
error1:
   *aDestLen = dest - aDest;
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_OK_UDEC_MOREOUTPUT;
error2:
   *aDestLen = dest - aDest;
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_ERROR_UNEXPECTED;
error3:
   *aDestLen = dest - aDest;
   *aSrcLen = src - (const unsigned char*)aSrc;
   return NS_ERROR_ILLEGAL_INPUT;
}
