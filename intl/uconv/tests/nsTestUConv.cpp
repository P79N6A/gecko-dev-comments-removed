





































#include <stdio.h>
#include <string.h>
#include "nsXPCOM.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsISupports.h"
#include "nsICharsetConverterManager.h"
#include "nsIPlatformCharset.h"
#include "nsICharRepresentable.h"
#include "prmem.h"
#include "nsReadableUtils.h"


static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);
static NS_DEFINE_CID(kPlatformCharsetCID, NS_PLATFORMCHARSET_CID);






















#define GENERAL_BUFFER 20000 // general purpose buffer; for Unicode divide by 2

#define ARRAY_SIZE(_array)                                      \
     (sizeof(_array) / sizeof(_array[0]))

nsICharsetConverterManager * ccMan = NULL;





char bLatin1_d0[] = {
  "\x00\x0d\x7f\x80\xff"
};

PRUnichar cLatin1_d0[] = {
  0x0000,0x000d,0x007f,0x20ac,0x00ff
};

PRInt32 bLatin1_s0 = ARRAY_SIZE(bLatin1_d0)-1;
PRInt32 cLatin1_s0 = ARRAY_SIZE(cLatin1_d0);




nsresult testCharsetConverterManager()
{
  printf("\n[T001] CharsetConverterManager\n");

  return NS_OK;
}




#define CREATE_DECODER(_charset)                                \
    nsIUnicodeDecoder * dec;                                    \
    nsAutoString str;str.AssignWithConversion(_charset);        \
    nsresult res = ccMan->GetUnicodeDecoder(&str,&dec);         \
    if (NS_FAILED(res)) {                                       \
      printf("ERROR at GetUnicodeDecoder() code=0x%x.\n",res);  \
      return res;                                               \
    }

#define CREATE_ENCODER(_charset)                                \
    nsIUnicodeEncoder * enc;                                    \
    nsAutoString str; str.AssignWithConversion(_charset);       \
    nsresult res = ccMan->GetUnicodeEncoder(&str,&enc);         \
    if (NS_FAILED(res)) {                                       \
      printf("ERROR at GetUnicodeEncoder() code=0x%x.\n",res);  \
      return res;                                               \
    }






nsresult testDecoder(nsIUnicodeDecoder * aDec, 
                     const char * aSrc, PRInt32 aSrcLength, 
                     const PRUnichar * aRes, PRInt32 aResLength,
                     const char * aTestName)
{
  nsresult res;

  
  PRInt32 srcLen = aSrcLength;
  PRUnichar dest[GENERAL_BUFFER/2];
  PRInt32 destLen = GENERAL_BUFFER/2;

  
  res = aDec->Convert(aSrc, &srcLen, dest, &destLen);
  
  if (res != NS_OK) {
    printf("ERROR at %s.easy.Decode() code=0x%x.\n",aTestName,res);
    return NS_ERROR_UNEXPECTED;
  }

  
  if (aResLength != destLen) {
      printf("ERROR at %s.easy.DecResLen expected=0x%x result=0x%x.\n", 
          aTestName, aResLength, destLen);
      return NS_ERROR_UNEXPECTED;
  }
  for (PRInt32 i=0; i<aResLength; i++) if (aRes[i] != dest[i]) {
      printf("ERROR at %s.easy.DecResChar[%d] expected=0x%x result=0x%x.\n", 
          aTestName, i, aRes[i], dest[i]);
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}






nsresult testEncoder(nsIUnicodeEncoder * aEnc, 
                     const PRUnichar * aSrc, PRInt32 aSrcLength, 
                     const char * aRes, PRInt32 aResLength,
                     const char * aTestName)
{
  nsresult res;

  
  PRInt32 srcLen = 0;
  char dest[GENERAL_BUFFER];
  PRInt32 destLen = 0;
  PRInt32 bcr, bcw;

  
  bcr = aSrcLength;
  bcw = GENERAL_BUFFER;
  res = aEnc->Convert(aSrc, &bcr, dest, &bcw);
  srcLen += bcr;
  destLen += bcw;
  
  if (res != NS_OK) {
    printf("ERROR at %s.easy.Encode() code=0x%x.\n",aTestName,res);
    return NS_ERROR_UNEXPECTED;
  }

  
  bcw = GENERAL_BUFFER - destLen;
  res = aEnc->Finish(dest + destLen, &bcw);
  destLen += bcw;
  
  if (res != NS_OK) {
    printf("ERROR at %s.easy.Finish() code=0x%x.\n",aTestName,res);
    return NS_ERROR_UNEXPECTED;
  }

  
  if (aResLength != destLen) {
      printf("ERROR at %s.easy.EncResLen expected=0x%x result=0x%x.\n", 
          aTestName, aResLength, destLen);
      return NS_ERROR_UNEXPECTED;
  }
  for (PRInt32 i=0; i<aResLength; i++) if (aRes[i] != dest[i]) {
      printf("ERROR at %s.easy.EncResChar[%d] expected=0x%x result=0x%x.\n", 
          aTestName, i, aRes[i], dest[i]);
      return NS_ERROR_UNEXPECTED;
  }
  
#ifdef TEST_IS_REPRESENTABLE
  nsICharRepresentable* rp = nsnull;
  res = aEnc->QueryInterface(NS_GET_IID(nsICharRepresentable),(void**) &rp);
  if(NS_SUCCEEDED(res))  {
    PRUint32 *info= (PRUint32*)PR_Calloc((0x10000 >> 5), 4);
    rp->FillInfo(info);
    for(int i=0;i< 0x10000;i++)
    {
       if(IS_REPRESENTABLE(info, i)) 
           printf("%4x\n", i);
    }
  }
#endif

  return NS_OK;
}







nsresult testStressDecoder(nsIUnicodeDecoder * aDec, 
                           const char * aSrc, PRInt32 aSrcLength, 
                           const PRUnichar * aRes, PRInt32 aResLength,
                           const char * aTestName)
{
  nsresult res;

  
  PRInt32 estimatedLength;
  res = aDec->GetMaxLength(aSrc, aSrcLength, &estimatedLength);
  if (NS_FAILED(res)) {
    printf("ERROR at %s.stress.Length() code=0x%x.\n",aTestName,res);
    return res;
  }
  PRBool exactLength = (res == NS_EXACT_LENGTH);

  
  PRInt32 srcLen = 0;
  PRInt32 srcOff = 0;
  PRUnichar dest[1024];
  PRInt32 destLen = 0;
  PRInt32 destOff = 0;

  
  for (;srcOff < aSrcLength;) {
    res = aDec->Convert(aSrc + srcOff, &srcLen, dest + destOff, &destLen);
    if (NS_FAILED(res)) {
      printf("ERROR at %s.stress.Convert() code=0x%x.\n",aTestName,res);
      return res;
    }

    srcOff+=srcLen;
    destOff+=destLen;

    
    srcLen = 1;
    
    if (res == NS_PARTIAL_MORE_OUTPUT) {
      destLen = 1;
    } else {
      destLen = 0;
    }
  }

  
  if (res != NS_OK) {
    printf("ERROR at %s.stress.postConvert() code=0x%x.\n",aTestName,res);
    return NS_ERROR_UNEXPECTED;
  }

  
  if (exactLength) {
    if (destOff != estimatedLength) {
      printf("ERROR at %s.stress.EstimatedLen expected=0x%x result=0x%x.\n",
          aTestName, estimatedLength, destOff);
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    if (destOff > estimatedLength) {
      printf("ERROR at %s.stress.EstimatedLen expected<=0x%x result=0x%x.\n",
          aTestName, estimatedLength, destOff);
      return NS_ERROR_UNEXPECTED;
    }
  }

  
  if (aResLength != destOff) {
      printf("ERROR at %s.stress.ConvResLen expected=0x%x result=0x%x.\n", 
          aTestName, aResLength, destOff);
      return NS_ERROR_UNEXPECTED;
  }
  for (PRInt32 i=0; i<aResLength; i++) if (aRes[i] != dest[i]) {
      printf("ERROR at %s.stress.ConvResChar[%d] expected=0x%x result=0x%x.\n", 
          aTestName, i, aRes[i], dest[i]);
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}







nsresult testStressEncoder(nsIUnicodeEncoder * aEnc, 
                           const PRUnichar * aSrc, PRInt32 aSrcLength,
                           const char * aRes, PRInt32 aResLength, 
                           const char * aTestName)
{
  nsresult res;

  
  PRInt32 estimatedLength;
  res = aEnc->GetMaxLength(aSrc, aSrcLength, &estimatedLength);
  if (NS_FAILED(res)) {
    printf("ERROR at %s.stress.Length() code=0x%x.\n",aTestName,res);
    return res;
  }
  PRBool exactLength = (res == NS_OK_UENC_EXACTLENGTH);

  
  PRInt32 srcLen = 0;
  PRInt32 srcOff = 0;
  char dest[GENERAL_BUFFER];
  PRInt32 destLen = 0;
  PRInt32 destOff = 0;

  
  for (;srcOff < aSrcLength;) {
    res = aEnc->Convert(aSrc + srcOff, &srcLen, dest + destOff, &destLen);
    if (NS_FAILED(res)) {
      printf("ERROR at %s.stress.Convert() code=0x%x.\n",aTestName,res);
      return res;
    }

    srcOff+=srcLen;
    destOff+=destLen;

    
    srcLen = 1;
    
    if (res == NS_OK_UENC_MOREOUTPUT) {
      destLen = 1;
    } else {
      destLen = 0;
    }
  }

  if (res != NS_OK) if (res != NS_OK_UENC_MOREOUTPUT) {
    printf("ERROR at %s.stress.postConvert() code=0x%x.\n",aTestName,res);
    return NS_ERROR_UNEXPECTED;
  } 
  
  for (;;) {
    res = aEnc->Finish(dest + destOff, &destLen);
    if (NS_FAILED(res)) {
      printf("ERROR at %s.stress.Finish() code=0x%x.\n",aTestName,res);
      return res;
    }

    destOff+=destLen;

    
    if (res == NS_OK_UENC_MOREOUTPUT) {
      destLen = 1;
    } else break;
  }

  
  if (exactLength) {
    if (destOff != estimatedLength) {
      printf("ERROR at %s.stress.EstimatedLen expected=0x%x result=0x%x.\n",
          aTestName, estimatedLength, destOff);
      return NS_ERROR_UNEXPECTED;
    }
  } else {
    if (destOff > estimatedLength) {
      printf("ERROR at %s.stress.EstimatedLen expected<=0x%x result=0x%x.\n",
          aTestName, estimatedLength, destOff);
      return NS_ERROR_UNEXPECTED;
    }
  }

  
  if (aResLength != destOff) {
      printf("ERROR at %s.stress.ConvResLen expected=0x%x result=0x%x.\n", 
          aTestName, aResLength, destOff);
      return NS_ERROR_UNEXPECTED;
  }
  for (PRInt32 i=0; i<aResLength; i++) if (aRes[i] != dest[i]) {
      printf("ERROR at %s.stress.ConvResChar[%d] expected=0x%x result=0x%x.\n", 
          aTestName, i, aRes[i], dest[i]);
      return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}




nsresult resetDecoder(nsIUnicodeDecoder * aDec, const char * aTestName)
{
  nsresult res = aDec->Reset();

  if (NS_FAILED(res)) {
    printf("ERROR at %s.dec.Reset() code=0x%x.\n",aTestName,res);
    return res;
  }

  return res;
}




nsresult resetEncoder(nsIUnicodeEncoder * aEnc, const char * aTestName)
{
  nsresult res = aEnc->Reset();

  if (NS_FAILED(res)) {
    printf("ERROR at %s.enc.Reset() code=0x%x.\n",aTestName,res);
    return res;
  }

  return res;
}




nsresult standardDecoderTest(char * aTestName, char * aCharset, char * aSrc, 
  PRInt32 aSrcLen, PRUnichar * aRes, PRInt32 aResLen)
{
  printf("\n[%s] Unicode <- %s\n", aTestName, aCharset);

  
  CREATE_DECODER(aCharset);

  
  res = testDecoder(dec, aSrc, aSrcLen, aRes, aResLen, aTestName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, aTestName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, aSrc, aSrcLen, aRes, aResLen, aTestName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}

nsresult loadBinaryFile(char * aFile, char * aBuff, PRInt32 * aBuffLen)
{
  FILE * f = fopen(aFile, "rb");
  if (f == NULL) {
    printf("ERROR at opening file: \"%s\".\n", aFile);
    return NS_ERROR_UNEXPECTED;
  }

  PRInt32 n = fread(aBuff, 1, *aBuffLen, f);
  if (n >= *aBuffLen) {
    printf("ERROR at reading from file \"%s\": too much input data.\n", aFile);
    return NS_ERROR_UNEXPECTED;
  }

  *aBuffLen = n;
  fclose(f);
  return NS_OK;
}

nsresult loadUnicodeFile(char * aFile, PRUnichar * aBuff, PRInt32 * aBuffLen)
{
  PRInt32 buffLen = 2*(*aBuffLen);

  nsresult res = loadBinaryFile(aFile, (char *)aBuff, &buffLen);
  if (NS_FAILED(res)) return res;

  *aBuffLen = buffLen/2;
  return NS_OK;
}

nsresult testDecoderFromFiles(char * aCharset, char * aSrcFile, char * aResultFile)
{
  
  CREATE_DECODER(aCharset);

  PRInt32 srcLen = GENERAL_BUFFER;
  char src[GENERAL_BUFFER];
  PRInt32 expLen = GENERAL_BUFFER/2;
  PRUnichar exp[GENERAL_BUFFER/2];

  res = loadBinaryFile(aSrcFile, src, &srcLen);
  if (NS_FAILED(res)) return res;

  res = loadUnicodeFile(aResultFile, exp, &expLen);
  if (NS_FAILED(res)) return res;

  
  res = testDecoder(dec, src, srcLen, exp, expLen, "dec");

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }

  return NS_OK;
}

nsresult testEncoderFromFiles(char * aCharset, char * aSrcFile, char * aResultFile)
{
  
  return NS_OK;
}







nsresult testISO2022JPDecoder()
{
  char * testName = "T102";
  printf("\n[%s] Unicode <- ISO2022JP\n", testName);

  
  CREATE_DECODER("iso-2022-jp");

  
  char src[] = {"\x0d\x7f\xdd" "\x1b(J\xaa\xdc\x41" "\x1b$B\x21\x21" "\x1b$@\x32\x37" "\x1b(J\x1b(B\xcc"};
  PRUnichar exp[] = {0x000d,0x007f,0xfffd, 0xff6a,0xFF9C,0x0041, 0x3000, 0x5378, 0xfffd};

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testEUCJPDecoder()
{
  char * testName = "T103";
  printf("\n[%s] Unicode <- EUCJP\n", testName);

  
  CREATE_DECODER("euc-jp");

  
  char src[] = {"\x45"};
  PRUnichar exp[] = {0x0045};

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testISO88597Decoder()
{
  char * testName = "T104";
  printf("\n[%s] Unicode <- ISO88597\n", testName);

  
  CREATE_DECODER("iso-8859-7");

  
  char src[] = {
    "\x09\x0d\x20\x40"
    "\x80\x98\xa3\xaf"
    "\xa7\xb1\xb3\xc9"
    "\xd9\xe3\xf4\xff"
  };
  PRUnichar exp[] = {
    0x0009, 0x000d, 0x0020, 0x0040, 
    0xfffd, 0xfffd, 0x00a3, 0x2015,
    0x00a7, 0x00b1, 0x00b3, 0x0399,
    0x03a9, 0x03b3, 0x03c4, 0xfffd
  };

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testSJISDecoder()
{
  char * testName = "T105";
  printf("\n[%s] Unicode <- SJIS\n", testName);

  
  CREATE_DECODER("Shift_JIS");

  
  char src[] = {
    "Japanese" 
    "\x8a\xbf\x8e\x9a" 
    "\x83\x4a\x83\x5e\x83\x4a\x83\x69" 
    "\x82\xd0\x82\xe7\x82\xaa\x82\xc8" 
    "\x82\x50\x82\x51\x82\x52\x82\x60\x82\x61\x82\x62" 
  };
  PRUnichar exp[] = {
    0x004A, 0x0061, 0x0070, 0x0061, 0x006E, 0x0065, 0x0073, 0x0065,
    0x6f22, 0x5b57,
    0x30ab, 0x30bf, 0x30ab, 0x30ca,
    0x3072, 0x3089, 0x304c, 0x306a,
    0xff11, 0xff12, 0xff13, 0xff21, 0xff22, 0xff23
  };

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testUTF8Decoder()
{
  char * testName = "T106";
  printf("\n[%s] Unicode <- UTF8\n", testName);

  
  CREATE_DECODER("utf-8");

#ifdef NOPE 
  
  char src[] = {};
  PRUnichar exp[] = {};

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);
#endif

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testMUTF7Decoder()
{
  char * testName = "T107";
  printf("\n[%s] Unicode <- MUTF7\n", testName);

  
  CREATE_DECODER("x-imap4-modified-utf7");

  
  char src[] = {"\x50\x51\x52\x53&AAAAAAAA-&-&AAA-"};
  PRUnichar exp[] = {0x0050,0x0051,0x0052,0x0053,0x0000,0x0000,0x0000,'&',0x0000};

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testUTF7Decoder()
{
  char * testName = "T108";
  printf("\n[%s] Unicode <- UTF7\n", testName);

  
  CREATE_DECODER("utf-7");

  
  char src[] = {"+ADwAIQ-DOC"};
  PRUnichar exp[] = {'<','!','D','O','C'};

  
  res = testDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  if (NS_SUCCEEDED(res)) res = resetDecoder(dec, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressDecoder(dec, src, ARRAY_SIZE(src)-1, exp, ARRAY_SIZE(exp), testName);

  
  NS_RELEASE(dec);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}







nsresult testLatin1Encoder()
{
  char * testName = "T201";
  printf("\n[%s] Unicode -> Latin1\n", testName);

  
  CREATE_ENCODER("iso-8859-1");
  enc->SetOutputErrorBehavior(enc->kOnError_Replace, NULL, 0x00cc);

  
  PRUnichar src[] = {0x0001,0x0002,0xffff,0x00e3};
  char exp[] = {"\x01\x02\xcc\xe3"};

  
  res = testEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  if (NS_SUCCEEDED(res)) res = resetEncoder(enc, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  NS_RELEASE(enc);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testSJISEncoder()
{
  char * testName = "T202";
  printf("\n[%s] Unicode -> SJIS\n", testName);

  
  CREATE_ENCODER("Shift_JIS");
  enc->SetOutputErrorBehavior(enc->kOnError_Replace, NULL, 0x00cc);

  
  PRUnichar src[] = {
    0x004A, 0x0061, 0x0070, 0x0061, 0x006E, 0x0065, 0x0073, 0x0065,
    0x6f22, 0x5b57,
    0x30ab, 0x30bf, 0x30ab, 0x30ca,
    0x3072, 0x3089, 0x304c, 0x306a,
    0xff11, 0xff12, 0xff13, 0xff21, 0xff22, 0xff23
  };
  char exp[] = {
    "Japanese" 
    "\x8a\xbf\x8e\x9a" 
    "\x83\x4a\x83\x5e\x83\x4a\x83\x69" 
    "\x82\xd0\x82\xe7\x82\xaa\x82\xc8" 
    "\x82\x50\x82\x51\x82\x52\x82\x60\x82\x61\x82\x62" 
  };

  
  res = testEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  if (NS_SUCCEEDED(res)) res = resetEncoder(enc, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  NS_RELEASE(enc);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testEUCJPEncoder()
{
  char * testName = "T203";
  printf("\n[%s] Unicode -> EUCJP\n", testName);

  
  CREATE_ENCODER("euc-jp");
  enc->SetOutputErrorBehavior(enc->kOnError_Replace, NULL, 0x00cc);

  
  PRUnichar src[] = {0x0045, 0x0054};
  char exp[] = {"\x45\x54"};

  
  res = testEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  if (NS_SUCCEEDED(res)) res = resetEncoder(enc, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  NS_RELEASE(enc);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testISO2022JPEncoder()
{
  char * testName = "T204";
  printf("\n[%s] Unicode -> ISO2022JP\n", testName);

  
  CREATE_ENCODER("iso-2022-jp");
  enc->SetOutputErrorBehavior(enc->kOnError_Replace, NULL, 0x00cc);

  
  PRUnichar src[] = {0x000d,0x007f, 0xff6a,0xFF9C, 0x3000, 0x5378};
  char exp[] = {"\x0d\x7f" "\x1b(J\xaa\xdc" "\x1b$@\x21\x21\x32\x37\x1b(B"};

  
  res = testEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  if (NS_SUCCEEDED(res)) res = resetEncoder(enc, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  NS_RELEASE(enc);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testMUTF7Encoder()
{
  char * testName = "T205";
  printf("\n[%s] Unicode -> MUTF-7\n", testName);

  
  CREATE_ENCODER("x-imap4-modified-utf7");
  enc->SetOutputErrorBehavior(enc->kOnError_Replace, NULL, 0x00cc);

  
  PRUnichar src[] = {0x0050,0x0051,0x0052,0x0053,0x0000,0x0000,0x0000,'&',0x0000};
  char exp[] = {"\x50\x51\x52\x53&AAAAAAAA-&-&AAA-"};

  
  res = testEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  if (NS_SUCCEEDED(res)) res = resetEncoder(enc, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  NS_RELEASE(enc);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}




nsresult testUTF7Encoder()
{
  char * testName = "T206";
  printf("\n[%s] Unicode -> UTF-7\n", testName);

  
  CREATE_ENCODER("utf-7");
  enc->SetOutputErrorBehavior(enc->kOnError_Replace, NULL, 0x00cc);

  
  PRUnichar src[] = {'e','t','i','r','a',0x0a};
  char exp[] = {"etira\x0a"};

  
  res = testEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  if (NS_SUCCEEDED(res)) res = resetEncoder(enc, testName);

  
  if (NS_SUCCEEDED(res)) 
    res = testStressEncoder(enc, src, ARRAY_SIZE(src), exp, ARRAY_SIZE(exp)-1, testName);

  
  NS_RELEASE(enc);

  if (NS_FAILED(res)) {
    return res;
  } else {
    printf("Test Passed.\n");
    return NS_OK;
  }
}

nsresult  testPlatformCharset()
{
  nsIPlatformCharset *cinfo;
  nsresult res = CallGetService(kPlatformCharsetCID, &cinfo);
  if (NS_FAILED(res)) {
    printf("ERROR at GetService() code=0x%x.\n",res);
    return res;
  }

  nsString value;
  res = cinfo->GetCharset(kPlatformCharsetSel_PlainTextInClipboard , value);
  printf("Clipboard plain text encoding = %s\n", NS_LossyConvertUTF16toASCII(value).get());
  
  res = cinfo->GetCharset(kPlatformCharsetSel_FileName , value);
  printf("File Name encoding = %s\n", NS_LossyConvertUTF16toASCII(value).get());

  res = cinfo->GetCharset(kPlatformCharsetSel_Menu , value);
  printf("Menu encoding = %s\n", NS_LossyConvertUTF16toASCII(value).get());

  cinfo->Release();
  return res;
  
}




nsresult testAll()
{
  nsresult res;

  
  res = testCharsetConverterManager();
  if (NS_FAILED(res)) return res;

  testPlatformCharset();

  
  standardDecoderTest("T101", "ISO-8859-1", bLatin1_d0, bLatin1_s0, cLatin1_d0, cLatin1_s0);
  testISO2022JPDecoder();
  testEUCJPDecoder();
  testISO88597Decoder();
  testSJISDecoder();
  testUTF8Decoder();
  testMUTF7Decoder();
  testUTF7Decoder();

  
  testLatin1Encoder();
  testSJISEncoder();
  testEUCJPEncoder();
  testISO2022JPEncoder();
  testMUTF7Encoder();
  testUTF7Encoder();

  
  return NS_OK;
}

nsresult testFromArgs(int argc, char **argv)
{
  nsresult res = NS_OK;
  if ((argc == 5) && (!strcmp(argv[1], "-tdec"))) {
    res = testDecoderFromFiles(argv[2], argv[3], argv[4]);
  } else if ((argc == 5) && (!strcmp(argv[1], "-tenc"))) {
    res = testEncoderFromFiles(argv[2], argv[3], argv[4]);
  } else {
    printf("Usage:\n");
    printf("  TestUConv.exe\n");
    printf("  TestUConv.exe -tdec encoding inputEncodedFile expectedResultUnicodeFile\n");
    printf("  TestUConv.exe -tenc encoding inputUnicodeFile expectedResultEncodedFile\n");
  }

  return res;
}




nsresult init()
{
  nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
  if (NS_FAILED(rv))
    return rv;
  return CallGetService(kCharsetConverterManagerCID, &ccMan);
}

nsresult done()
{
  NS_RELEASE(ccMan);
  return NS_OK;
}

int main(int argc, char **argv)
{
  nsresult res;

  res = init();
  if (NS_FAILED(res)) return -1;

  if (argc <= 1) {
    printf("*** Unicode Converters Test ***\n");
    res = testAll();
    printf("\n***---------  Done  --------***\n");
  } else {
    res = testFromArgs(argc, argv);
  }

  done();

  if (NS_FAILED(res)) return -1;
  else return 0;
}
