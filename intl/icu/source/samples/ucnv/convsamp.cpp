


























#define DEBUG_TMI 0  /* define to 1 to enable Too Much Information */

#include <stdio.h>
#include <ctype.h>            
#include <assert.h>
#include <string.h>
#include <stdlib.h>  

#include "unicode/utypes.h"   
#include "unicode/ucnv.h"     
#include "unicode/ustring.h"  
#include "unicode/uchar.h"    
#include "unicode/uloc.h"
#include "unicode/unistr.h"

#include "flagcb.h"



static const UChar kNone[] = { 0x0000 };

#define U_ASSERT(x)  { if(U_FAILURE(x)) {fflush(stdout);fflush(stderr); fprintf(stderr, #x " == %s\n", u_errorName(x)); assert(U_SUCCESS(x)); }}


void prettyPrintUChar(UChar c)
{
  if(  (c <= 0x007F) &&
       (isgraph(c))  ) {
    printf(" '%c'   ", (char)(0x00FF&c));
  } else if ( c > 0x007F ) {
    char buf[1000];
    UErrorCode status = U_ZERO_ERROR;
    int32_t o;

    o = u_charName(c, U_EXTENDED_CHAR_NAME, buf, 1000, &status);
    if(U_SUCCESS(status) && (o>0) ) {
      buf[6] = 0;
      printf("%7s", buf);
    } else {
      printf(" ??????");
    }
  } else {
    switch((char)(c & 0x007F)) {
    case ' ':
      printf(" ' '   ");
      break;
    case '\t':
      printf(" \\t    ");
      break;
    case '\n':
      printf(" \\n    ");
      break;
    default:
      printf("  _    ");
      break;
    }
  }
}


void printUChars(const char  *name = "?", 
                 const UChar *uch  = kNone,
                 int32_t     len   = -1 )
{
  int32_t i;

  if( (len == -1) && (uch) ) {
    len = u_strlen(uch);
  }

  printf("%5s: ", name);
  for( i = 0; i <len; i++) {
    printf("%-6d ", i);
  }
  printf("\n");

  printf("%5s: ", "uni");
  for( i = 0; i <len; i++) {
    printf("\\u%04X ", (int)uch[i]);
  }
  printf("\n");

  printf("%5s:", "ch");
  for( i = 0; i <len; i++) {
    prettyPrintUChar(uch[i]);
  }
  printf("\n");
}

void printBytes(const char  *name = "?", 
                 const char *uch  = "",
                 int32_t     len   = -1 )
{
  int32_t i;

  if( (len == -1) && (uch) ) {
    len = strlen(uch);
  }

  printf("%5s: ", name);
  for( i = 0; i <len; i++) {
    printf("%-4d ", i);
  }
  printf("\n");

  printf("%5s: ", "uni");
  for( i = 0; i <len; i++) {
    printf("\\x%02X ", 0x00FF & (int)uch[i]);
  }
  printf("\n");

  printf("%5s:", "ch");
  for( i = 0; i <len; i++) {
    if(isgraph(0x00FF & (int)uch[i])) {
      printf(" '%c' ", (char)uch[i]);
    } else {
      printf("     ");
    }
  }
  printf("\n");
}

void printUChar(UChar32 ch32)
{
    if(ch32 > 0xFFFF) {
      printf("ch: U+%06X\n", ch32);
    }
    else {
      UChar ch = (UChar)ch32;
      printUChars("C", &ch, 1);
    }
}
































UErrorCode convsample_02()
{
  printf("\n\n==============================================\n"
         "Sample 02: C: simple Unicode -> koi8-r conversion\n");


  
  
  UChar source[] = { 0x041C, 0x043E, 0x0441, 0x043A, 0x0432,
                     0x0430, 0x0021, 0x0000 };
  char target[100];
  UErrorCode status = U_ZERO_ERROR;
  UConverter *conv;
  int32_t     len;

  
  
  conv = ucnv_open("koi8-r", &status);
  
  assert(U_SUCCESS(status));

  
  len = ucnv_fromUChars(conv, target, 100, source, -1, &status);
  assert(U_SUCCESS(status));

  
  ucnv_close(conv);

  
  
  
  printUChars("src", source);
  printf("\n");
  printBytes("targ", target, len);

  return U_ZERO_ERROR;
}


UErrorCode convsample_03()
{
  printf("\n\n==============================================\n"
         "Sample 03: C: print out all converters\n");

  int32_t count;
  int32_t i;

  
  count = ucnv_countAvailable();
  printf("Available converters: %d\n", count);
  
  for(i=0;i<count;i++) 
  {
    printf("%s ", ucnv_getAvailableName(i));
  }

  
  
  printf("\n");

  return U_ZERO_ERROR;
}



#define BUFFERSIZE 17 /* make it interesting :) */



























UErrorCode convsample_05()
{
  printf("\n\n==============================================\n"
         "Sample 05: C: count the number of letters in a UTF-8 document\n");

  FILE *f;
  int32_t count;
  char inBuf[BUFFERSIZE];
  const char *source;
  const char *sourceLimit;
  UChar *uBuf;
  UChar *target;
  UChar *targetLimit;
  UChar *p;
  int32_t uBufSize = 0;
  UConverter *conv;
  UErrorCode status = U_ZERO_ERROR;
  uint32_t letters=0, total=0;

  f = fopen("data01.txt", "r");
  if(!f)
  {
    fprintf(stderr, "Couldn't open file 'data01.txt' (UTF-8 data file).\n");
    return U_FILE_ACCESS_ERROR;
  }

  
  conv = ucnv_open("utf-8", &status);
  assert(U_SUCCESS(status));

  uBufSize = (BUFFERSIZE/ucnv_getMinCharSize(conv));
  printf("input bytes %d / min chars %d = %d UChars\n",
         BUFFERSIZE, ucnv_getMinCharSize(conv), uBufSize);
  uBuf = (UChar*)malloc(uBufSize * sizeof(UChar));
  assert(uBuf!=NULL);

  
  while((!feof(f)) && 
        ((count=fread(inBuf, 1, BUFFERSIZE , f)) > 0) )
  {
    
    source = inBuf;
    sourceLimit = inBuf + count;
    
    do
    {
        target = uBuf;
        targetLimit = uBuf + uBufSize;
        
        ucnv_toUnicode(conv, &target, targetLimit, 
                       &source, sourceLimit, NULL,
                       feof(f)?TRUE:FALSE,         
                                   
                       &status);
      
        if(status == U_BUFFER_OVERFLOW_ERROR)
        {
          
          
          status = U_ZERO_ERROR;
        }
        else
        {
          
          assert(U_SUCCESS(status));
          
        }

        
        

        for(p = uBuf; p<target; p++)
        {
          if(u_isalpha(*p))
            letters++;
          total++;
        }
    } while (source < sourceLimit); 
  }

  printf("%d letters out of %d total UChars.\n", letters, total);
  
  
  ucnv_close(conv);

  printf("\n");

  fclose(f);

  return U_ZERO_ERROR;
}
#undef BUFFERSIZE

#define BUFFERSIZE 1024
typedef struct
{
  UChar32  codepoint;
  uint32_t frequency;
} CharFreqInfo;

UErrorCode convsample_06()
{
  printf("\n\n==============================================\n"
         "Sample 06: C: frequency distribution of letters in a UTF-8 document\n");

  FILE *f;
  int32_t count;
  char inBuf[BUFFERSIZE];
  const char *source;
  const char *sourceLimit;
  int32_t uBufSize = 0;
  UConverter *conv;
  UErrorCode status = U_ZERO_ERROR;
  uint32_t letters=0, total=0;

  CharFreqInfo   *info;
  UChar32   charCount = 0x10000;  
  UChar32   p;

  uint32_t ie = 0;
  uint32_t gh = 0;
  UChar32 l = 0;

  f = fopen("data06.txt", "r");
  if(!f)
  {
    fprintf(stderr, "Couldn't open file 'data06.txt' (UTF-8 data file).\n");
    return U_FILE_ACCESS_ERROR;
  }

  info = (CharFreqInfo*)malloc(sizeof(CharFreqInfo) * charCount);
  if(!info)
  {
    fprintf(stderr, " Couldn't allocate %d bytes for freq counter\n", sizeof(CharFreqInfo)*charCount);
  }

  
  for(p=0;p<charCount;p++)
  {
    info[p].codepoint = p;
    info[p].frequency = 0;
  }

  
  conv = ucnv_open("utf-8", &status);
  assert(U_SUCCESS(status));

  uBufSize = (BUFFERSIZE/ucnv_getMinCharSize(conv));
  printf("input bytes %d / min chars %d = %d UChars\n",
         BUFFERSIZE, ucnv_getMinCharSize(conv), uBufSize);

  
  while((!feof(f)) && 
        ((count=fread(inBuf, 1, BUFFERSIZE , f)) > 0) )
  {
    
    source = inBuf;
    sourceLimit = inBuf + count;
    
    while(source < sourceLimit)
    {
      p = ucnv_getNextUChar(conv, &source, sourceLimit, &status);
      if(U_FAILURE(status))
      {
        fprintf(stderr, "%s @ %d\n", u_errorName(status), total);
        status = U_ZERO_ERROR;
        continue;
      }
      U_ASSERT(status);
      total++;

      if(u_isalpha(p))
        letters++;

      if((u_tolower(l) == 'i') && (u_tolower(p) == 'e'))
        ie++;

      if((u_tolower(l) == 'g') && (u_tolower(p) == 0x0127))
        gh++;

      if(p>charCount)
      {
        fprintf(stderr, "U+%06X: oh.., we only handle BMP characters so far.. redesign!\n", p);
        free(info);
        fclose(f);
        ucnv_close(conv);
        return U_UNSUPPORTED_ERROR;
      }
      info[p].frequency++;
      l = p;
    }
  }

  fclose(f);
  ucnv_close(conv);

  printf("%d letters out of %d total UChars.\n", letters, total);
  printf("%d ie digraphs, %d gh digraphs.\n", ie, gh);

  

  

  for(p=0;p<charCount;p++)
  {
    if(info[p].frequency)
    {
      printf("% 5d U+%06X ", info[p].frequency, p);
      if(p <= 0xFFFF)
      {
        prettyPrintUChar((UChar)p);
      }
      printf("\n");
    }
  }
  free(info);
  

  printf("\n");

  return U_ZERO_ERROR;
}
#undef BUFFERSIZE










UErrorCode convsample_12()
{
  printf("\n\n==============================================\n"
         "Sample 12: C: simple sjis -> unicode conversion\n");


  

  char source[] = { 0x63, 0x61, 0x74, (char)0x94, 0x4C, (char)0x82, 0x6E, (char)0x82, 0x6A, 0x00 };
  UChar target[100];
  UErrorCode status = U_ZERO_ERROR;
  UConverter *conv;
  int32_t     len;

  
  conv = ucnv_open("shift_jis", &status);
  assert(U_SUCCESS(status));

  
  
  target[6] = 0xFDCA;
  len = ucnv_toUChars(conv, target, 100, source, strlen(source), &status);
  U_ASSERT(status);
  
  ucnv_close(conv);

  
  
  
  printBytes("src", source, strlen(source) );
  printf("\n");
  printUChars("targ", target, len);

  return U_ZERO_ERROR;
}




  
UErrorCode convsample_13()
{
  printf("\n\n==============================================\n"
         "Sample 13: C: simple Big5 -> unicode conversion, char at a time\n");


  const char sourceChars[] = { 0x7a, 0x68, 0x3d, (char)0xa4, (char)0xa4, (char)0xa4, (char)0xe5, (char)0x2e };
  
  const char *source, *sourceLimit;
  UChar32 target;
  UErrorCode status = U_ZERO_ERROR;
  UConverter *conv = NULL;
  int32_t srcCount=0;
  int32_t dstCount=0;
  
  srcCount = sizeof(sourceChars);

  conv = ucnv_open("Big5", &status);
  U_ASSERT(status);

  source = sourceChars;
  sourceLimit = sourceChars + sizeof(sourceChars);

  


  printBytes("src",source,sourceLimit-source);

  while(source < sourceLimit)
  {
    puts("");
    target = ucnv_getNextUChar (conv,
                                &source,
                                sourceLimit,
                                &status);
    
    
    U_ASSERT(status);
    printUChar(target);
    dstCount++;
  }
  
  
  
  
  printf("src=%d bytes, dst=%d uchars\n", srcCount, dstCount);
  ucnv_close(conv);

  return U_ZERO_ERROR;
}




UBool convsample_20_didSubstitute(const char *source)
{
  UChar uchars[100];
  char bytes[100];
  UConverter *conv = NULL;
  UErrorCode status = U_ZERO_ERROR;
  uint32_t len, len2;
  UBool  flagVal;
  
  FromUFLAGContext * context = NULL;

  printf("\n\n==============================================\n"
         "Sample 20: C: Test for substitution using callbacks\n");

  
  printBytes("src", source);
  printf("\n");

  
  conv = ucnv_open("utf-8", &status);
  U_ASSERT(status);

  len = ucnv_toUChars(conv, uchars, 100, source, strlen(source), &status);
  U_ASSERT(status);
 
  printUChars("uch", uchars, len);
  printf("\n");

  
  ucnv_close(conv);

  
  conv = ucnv_open("windows-1252", &status);
  U_ASSERT(status);

  

  
  context = flagCB_fromU_openContext();

  
  ucnv_setFromUCallBack(conv,
                        flagCB_fromU,
                        context,
                        &(context->subCallback),
                        &(context->subContext),
                        &status);

  U_ASSERT(status);

  len2 = ucnv_fromUChars(conv, bytes, 100, uchars, len, &status);
  U_ASSERT(status);

  flagVal = context->flag;  

  ucnv_close(conv);

  
  printBytes("bytes", bytes, len2);

  return flagVal; 
}

UErrorCode convsample_20()
{
  const char *sample1 = "abc\xdf\xbf";
  const char *sample2 = "abc_def";


  if(convsample_20_didSubstitute(sample1))
  {
    printf("DID substitute.\n******\n");
  }
  else
  {
    printf("Did NOT substitute.\n*****\n");
  }

  if(convsample_20_didSubstitute(sample2))
  {
    printf("DID substitute.\n******\n");
  }
  else
  {
    printf("Did NOT substitute.\n*****\n");
  }

  return U_ZERO_ERROR;
}





UBool convsample_21_didSubstitute(const char *source)
{
  UChar uchars[100];
  char bytes[100];
  UConverter *conv = NULL, *cloneCnv = NULL;
  UErrorCode status = U_ZERO_ERROR;
  uint32_t len, len2;
  int32_t  cloneLen;
  UBool  flagVal = FALSE;
  UConverterFromUCallback junkCB;
  
  FromUFLAGContext *flagCtx = NULL, 
                   *cloneFlagCtx = NULL;

  debugCBContext   *debugCtx1 = NULL,
                   *debugCtx2 = NULL,
                   *cloneDebugCtx = NULL;

  printf("\n\n==============================================\n"
         "Sample 21: C: Test for substitution w/ callbacks & clones \n");

  
  printBytes("src", source);
  printf("\n");

  
  conv = ucnv_open("utf-8", &status);
  U_ASSERT(status);

  len = ucnv_toUChars(conv, uchars, 100, source, strlen(source), &status);
  U_ASSERT(status);
 
  printUChars("uch", uchars, len);
  printf("\n");

  
  ucnv_close(conv);

  
  conv = ucnv_open("windows-1252", &status);
  U_ASSERT(status);

  

  
  


#if DEBUG_TMI
  printf("flagCB_fromU = %p\n", &flagCB_fromU);
  printf("debugCB_fromU = %p\n", &debugCB_fromU);
#endif

  debugCtx1 = debugCB_openContext();
   flagCtx  = flagCB_fromU_openContext();
  debugCtx2 = debugCB_openContext();

  debugCtx1->subCallback =  flagCB_fromU;  
  debugCtx1->subContext  =  flagCtx;

  flagCtx->subCallback   =  debugCB_fromU; 
  flagCtx->subContext    =  debugCtx2;

  debugCtx2->subCallback =  UCNV_FROM_U_CALLBACK_SUBSTITUTE;
  debugCtx2->subContext  = NULL;

  

  ucnv_setFromUCallBack(conv,
                        debugCB_fromU,
                        debugCtx1,
                        &(debugCtx2->subCallback),
                        &(debugCtx2->subContext),
                        &status);

  U_ASSERT(status);

#if DEBUG_TMI
  printf("Callback chain now: Converter %p -> debug1:%p-> (%p:%p)==flag:%p -> debug2:%p -> cb %p\n",
         conv, debugCtx1, debugCtx1->subCallback,
         debugCtx1->subContext, flagCtx, debugCtx2, debugCtx2->subCallback);
#endif

  cloneCnv = ucnv_safeClone(conv, NULL, NULL, &status);

  U_ASSERT(status);

#if DEBUG_TMI
  printf("Cloned converter from %p -> %p.  Closing %p.\n", conv, cloneCnv, conv);
#endif
  
  ucnv_close(conv);

#if DEBUG_TMI
  printf("%p closed.\n", conv);
#endif 

  U_ASSERT(status);
  
  cloneDebugCtx = NULL;
  cloneFlagCtx  = NULL;

  ucnv_getFromUCallBack(cloneCnv, &junkCB, (const void **)&cloneDebugCtx);
  if(cloneDebugCtx != NULL) {
      cloneFlagCtx = (FromUFLAGContext*) cloneDebugCtx -> subContext;
  }

  printf("Cloned converter chain: %p -> %p[debug1] -> %p[flag] -> %p[debug2] -> substitute\n",
         cloneCnv, cloneDebugCtx, cloneFlagCtx, cloneFlagCtx?cloneFlagCtx->subContext:NULL );

  len2 = ucnv_fromUChars(cloneCnv, bytes, 100, uchars, len, &status);
  U_ASSERT(status);

  if(cloneFlagCtx != NULL) {
      flagVal = cloneFlagCtx->flag;  
  } else {
      printf("** Warning, couldn't get the subcallback \n");
  }

  ucnv_close(cloneCnv);

  
  printBytes("bytes", bytes, len2);

  return flagVal; 
}

UErrorCode convsample_21()
{
  const char *sample1 = "abc\xdf\xbf";
  const char *sample2 = "abc_def";

  if(convsample_21_didSubstitute(sample1))
  {
    printf("DID substitute.\n******\n");
  }
  else
  {
    printf("Did NOT substitute.\n*****\n");
  }

  if(convsample_21_didSubstitute(sample2))
  {
    printf("DID substitute.\n******\n");
  }
  else
  {
    printf("Did NOT substitute.\n*****\n");
  }

  return U_ZERO_ERROR;
}




#define BUFFERSIZE 17 /* make it interesting :) */

UErrorCode convsample_40()
{
  printf("\n\n==============================================\n"
    "Sample 40: C: convert data02.bin from cp37 to UTF16 [data40.utf16]\n");

  FILE *f;
  FILE *out;
  int32_t count;
  char inBuf[BUFFERSIZE];
  const char *source;
  const char *sourceLimit;
  UChar *uBuf;
  UChar *target;
  UChar *targetLimit;
  int32_t uBufSize = 0;
  UConverter *conv = NULL;
  UErrorCode status = U_ZERO_ERROR;
  uint32_t inbytes=0, total=0;

  f = fopen("data02.bin", "rb");
  if(!f)
  {
    fprintf(stderr, "Couldn't open file 'data02.bin' (cp37 data file).\n");
    return U_FILE_ACCESS_ERROR;
  }

  out = fopen("data40.utf16", "wb");
  if(!out)
  {
    fprintf(stderr, "Couldn't create file 'data40.utf16'.\n");
    fclose(f);
    return U_FILE_ACCESS_ERROR;
  }

  
  conv = ucnv_openCCSID(37, UCNV_IBM, &status);
  assert(U_SUCCESS(status));

  uBufSize = (BUFFERSIZE/ucnv_getMinCharSize(conv));
  printf("input bytes %d / min chars %d = %d UChars\n",
         BUFFERSIZE, ucnv_getMinCharSize(conv), uBufSize);
  uBuf = (UChar*)malloc(uBufSize * sizeof(UChar));
  assert(uBuf!=NULL);

  
  while((!feof(f)) && 
        ((count=fread(inBuf, 1, BUFFERSIZE , f)) > 0) )
  {
    inbytes += count;

    
    source = inBuf;
    sourceLimit = inBuf + count;
    
    do
    {
        target = uBuf;
        targetLimit = uBuf + uBufSize;
        
        ucnv_toUnicode( conv, &target, targetLimit, 
                       &source, sourceLimit, NULL,
                       feof(f)?TRUE:FALSE,         
                                   
                         &status);
      
        if(status == U_BUFFER_OVERFLOW_ERROR)
        {
          
          
          status = U_ZERO_ERROR;
        }
        else
        {
          
          assert(U_SUCCESS(status));
          
        }

        
        
        assert(fwrite(uBuf, sizeof(uBuf[0]), (target-uBuf), out) ==
               (size_t)(target-uBuf));
        total += (target-uBuf);
    } while (source < sourceLimit); 
  }

  printf("%d bytes in,  %d UChars out.\n", inbytes, total);
  
  
  ucnv_close(conv);

  fclose(f);
  fclose(out);
  printf("\n");

  return U_ZERO_ERROR;
}
#undef BUFFERSIZE





#define BUFFERSIZE 24 /* make it interesting :) */

UErrorCode convsample_46()
{
  printf("\n\n==============================================\n"
    "Sample 46: C: convert data40.utf16 from UTF16 to latin2 [data46.out]\n");

  FILE *f;
  FILE *out;
  int32_t count;
  UChar inBuf[BUFFERSIZE];
  const UChar *source;
  const UChar *sourceLimit;
  char *buf;
  char *target;
  char *targetLimit;

  int32_t bufSize = 0;
  UConverter *conv = NULL;
  UErrorCode status = U_ZERO_ERROR;
  uint32_t inchars=0, total=0;

  f = fopen("data40.utf16", "rb");
  if(!f)
  {
    fprintf(stderr, "Couldn't open file 'data40.utf16' (did you run convsample_40() ?)\n");
    return U_FILE_ACCESS_ERROR;
  }

  out = fopen("data46.out", "wb");
  if(!out)
  {
    fprintf(stderr, "Couldn't create file 'data46.out'.\n");
    fclose(f);
    return U_FILE_ACCESS_ERROR;
  }

  
  conv = ucnv_open( "iso-8859-2", &status);
  assert(U_SUCCESS(status));

  bufSize = (BUFFERSIZE*ucnv_getMaxCharSize(conv));
  printf("input UChars[16] %d * max charsize %d = %d bytes output buffer\n",
         BUFFERSIZE, ucnv_getMaxCharSize(conv), bufSize);
  buf = (char*)malloc(bufSize * sizeof(char));
  assert(buf!=NULL);

  
  while((!feof(f)) && 
        ((count=fread(inBuf, sizeof(UChar), BUFFERSIZE , f)) > 0) )
  {
    inchars += count;

    
    source = inBuf;
    sourceLimit = inBuf + count;
    
    do
    {
        target = buf;
        targetLimit = buf + bufSize;
        
        ucnv_fromUnicode( conv, &target, targetLimit, 
                       &source, sourceLimit, NULL,
                       feof(f)?TRUE:FALSE,         
                                   
                         &status);
      
        if(status == U_BUFFER_OVERFLOW_ERROR)
        {
          
          
          status = U_ZERO_ERROR;
        }
        else
        {
          
          assert(U_SUCCESS(status));
          
        }

        
        assert(fwrite(buf, sizeof(buf[0]), (target-buf), out) ==
               (size_t)(target-buf));
        total += (target-buf);
    } while (source < sourceLimit); 
  }

  printf("%d Uchars (%d bytes) in, %d chars out.\n", inchars, inchars * sizeof(UChar), total);
  
  
  ucnv_close(conv);

  fclose(f);
  fclose(out);
  printf("\n");

  return U_ZERO_ERROR;
}
#undef BUFFERSIZE

#define BUFFERSIZE 219

void convsample_50() {
  printf("\n\n==============================================\n"
         "Sample 50: C: ucnv_detectUnicodeSignature\n");

  
  UErrorCode err = U_ZERO_ERROR;
  UBool discardSignature = TRUE; 
  char input[] = { '\xEF','\xBB', '\xBF','\x41','\x42','\x43' };
  int32_t signatureLength = 0;
  const char *encoding = ucnv_detectUnicodeSignature(input,sizeof(input),&signatureLength,&err);
  UConverter *conv = NULL;
  UChar output[100];
  UChar *target = output, *out;
  const char *source = input;
  if(encoding!=NULL && U_SUCCESS(err)){
    
    conv = ucnv_open(encoding, &err);
    
    ucnv_toUnicode(conv,
                   &target, output + sizeof(output)/U_SIZEOF_UCHAR,
                   &source, input + sizeof(input),
                   NULL, TRUE, &err);
    out = output;
    if (discardSignature){
      ++out; 
    }
    while(out != target) {
      printf("%04x ", *out++);
    }
    puts("");
  }
  
  puts("");
}





int main()
{

  printf("Default Converter=%s\n", ucnv_getDefaultName() );
  
  convsample_02();  
  convsample_03();  

  convsample_05();  
  convsample_06(); 

  convsample_12();  
  convsample_13();  
  
  convsample_20();  
  convsample_21();  
  
  convsample_40();  
  
  convsample_46();  

  convsample_50();  
  
  printf("End of converter samples.\n");
  
  fflush(stdout);
  fflush(stderr);
  
  return 0;
}
