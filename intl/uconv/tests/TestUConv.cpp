




































#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsUCSupport.h"
#include "nsString.h"
#include "nsIStringEnumerator.h"
#include "nsVoidArray.h"




#define ARRAY_SIZE(_array)  (sizeof(_array) / sizeof(_array[0]))
#define SMALL_BUFFER_SIZE   512
#define MED_BUFFER_SIZE     1024
#define BIG_BUFFER_SIZE     2048

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);















class nsTestLog
{
private:

  static const char * kTraceDelimiter;

  nsCAutoString mTrace;

public:

  void AddTrace(char * aTrace);
  void DelTrace(char * aTrace);
  void PrintError(char * aCall, int aError);
  void PrintError(char * aCall, char * aMessage);
};
  













class nsTestUConv
{
private:

  nsTestLog mLog;

  


  nsresult TestEncoders();

  


  nsresult TestDecoders();

  


  nsresult TestCharsetManager();

  


  nsresult DisplayDetectors();

  


  nsresult DisplayCharsets();

  



  nsresult TestTempBug();

  nsresult Encode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, char ** aDest, 
    char * aDestEnd, const nsAFlatCString& aCharset);

  






  nsresult ConvertEncode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, char ** aDest, 
    char * aDestEnd, nsIUnicodeEncoder * aEncoder);

  


  nsresult FinishEncode(char ** aDest, char * aDestEnd, 
    nsIUnicodeEncoder * aEncoder);

  void PrintSpaces(int aCount);

public:

  


  nsresult Main(int aArgC, char ** aArgV);
};
  



int main(int argc, char ** argv)
{
  nsTestUConv testObj;
  nsresult res;
 
  res = testObj.Main(argc, argv);
  return (NS_FAILED(res));
}




const char * nsTestLog::kTraceDelimiter = ".";

void nsTestLog::AddTrace(char * aTrace)
{
  mTrace.Append(aTrace);
  mTrace.Append(kTraceDelimiter);
}

void nsTestLog::DelTrace(char * aTrace)
{
  mTrace.Truncate(mTrace.Length() - strlen(aTrace) - strlen(kTraceDelimiter));
}

void nsTestLog::PrintError(char * aCall, int aError)
{
  const char * trace = mTrace.get();
  printf("ERROR at %s%s code=0x%x.\n", trace, aCall, aError);
}

void nsTestLog::PrintError(char * aCall, char * aMessage)
{
  const char * trace = mTrace.get();
  printf("ERROR at %s%s reason: %s.\n", trace, aCall, aMessage);
}




nsresult nsTestUConv::TestEncoders()
{
  char * trace = "TestEncoders";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) return res;
  
  nsCOMPtr<nsIUTF8StringEnumerator> encoders;
  res = ccMan->GetEncoderList(getter_AddRefs(encoders));
  if (NS_FAILED(res)) return res;

  PRUint32 encoderCount=0;

  PRBool hasMore;
  encoders->HasMore(&hasMore);
  
  nsCAutoString charset;
  while (hasMore) {
    encoders->GetNext(charset);

    encoders->HasMore(&hasMore);
  }
  
  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::TestDecoders()
{
  char * trace = "TestDecoders";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::TestCharsetManager()
{
  char * trace = "TestCharsetManager";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;
  nsAutoString name;
  nsCOMPtr<nsIAtom> csAtom;

  nsCOMPtr<nsICharsetConverterManager> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::DisplayDetectors()
{
  char * trace = "DisplayDetectors";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  
  nsCOMPtr<nsIUTF8StringEnumerator> detectors;

  res = ccMan->GetCharsetDetectorList(getter_AddRefs(detectors));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetCharsetDetectorList()", res);
    return res;
  }

  printf("***** Character Set Detectors *****\n");

  PRBool hasMore;
  detectors->HasMore(&hasMore);
  while (hasMore) {
    nsCAutoString detectorName;
    res = detectors->GetNext(detectorName);
    if (NS_FAILED(res)) {
      mLog.PrintError("GetNext()", res);
      return res;
    }

    printf("%s", detectorName.get());
    PrintSpaces(36 - detectorName.Length()); 

    nsAutoString title;
    res = ccMan->GetCharsetTitle(detectorName.get(), title);
    if (NS_FAILED(res)) title.SetLength(0);
    printf("\"%s\"\n", NS_LossyConvertUTF16toASCII(title).get());

    detectors->HasMore(&hasMore);
  }
  
  mLog.DelTrace(trace);
  return NS_OK;
}

nsresult nsTestUConv::DisplayCharsets()
{
  char * trace = "DisplayCharsets";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  nsCOMPtr<nsIUTF8StringEnumerator> decoders;
  nsCOMPtr<nsIUTF8StringEnumerator> encoders;

  res = ccMan->GetDecoderList(getter_AddRefs(decoders));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetDecoderList()", res);
    return res;
  }

  res = ccMan->GetEncoderList(getter_AddRefs(encoders));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetEncoderList()", res);
    return res;
  }


  printf("***** Character Sets *****\n");

  PRUint32 encCount = 0, decCount = 0;
  PRUint32 basicEncCount = 0, basicDecCount = 0;

  nsCStringArray allCharsets;
  
  nsCAutoString charset;
  PRBool hasMore;
  encoders->HasMore(&hasMore);
  while (hasMore) {
    res = encoders->GetNext(charset);
    if (NS_SUCCEEDED(res))
      allCharsets.AppendCString(charset);

    encoders->HasMore(&hasMore);
  }

  nsAutoString prop, str;
  PRUint32 count = allCharsets.Count();
  for (PRUint32 i = 0; i < count; i++) {

    const nsCString* charset = allCharsets[i];
    printf("%s", charset->get());
    PrintSpaces(24 - charset->Length());  


    nsCOMPtr<nsIUnicodeDecoder> dec = NULL;
    res = ccMan->GetUnicodeDecoder(charset->get(), getter_AddRefs(dec));
    if (NS_FAILED(res)) printf (" "); 
    else {
      printf("D");
      decCount++;
    }
#ifdef NS_DEBUG
    
    if (dec) {
      nsCOMPtr<nsIBasicDecoder> isBasic = do_QueryInterface(dec);
      if (isBasic) {
        basicDecCount++;
        printf("b");
      }
      else printf(" ");
    }
    else printf(" ");
#endif

    nsCOMPtr<nsIUnicodeEncoder> enc = NULL;
    res = ccMan->GetUnicodeEncoder(charset->get(), getter_AddRefs(enc));
    if (NS_FAILED(res)) printf (" "); 
    else {
      printf("E");
      encCount++;
    }

#ifdef NS_DEBUG
    if (enc) {
      nsCOMPtr<nsIBasicEncoder> isBasic = do_QueryInterface(enc);
      if (isBasic) {
        basicEncCount++;
        printf("b");
      }
      else printf(" ");
    }
    else printf(" ");
#endif
    
    printf(" ");

    prop.AssignLiteral(".notForBrowser");
    res = ccMan->GetCharsetData(charset->get(), prop.get(), str);
    if ((dec != NULL) && (NS_FAILED(res))) printf ("B"); 
    else printf("X");

    prop.AssignLiteral(".notForComposer");
    res = ccMan->GetCharsetData(charset->get(), prop.get(), str);
    if ((enc != NULL) && (NS_FAILED(res))) printf ("C"); 
    else printf("X");

    prop.AssignLiteral(".notForMailView");
    res = ccMan->GetCharsetData(charset->get(), prop.get(), str);
    if ((dec != NULL) && (NS_FAILED(res))) printf ("V"); 
    else printf("X");

    prop.AssignLiteral(".notForMailEdit");
    res = ccMan->GetCharsetData(charset->get(), prop.get(), str);
    if ((enc != NULL) && (NS_FAILED(res))) printf ("E"); 
    else printf("X");

    printf("(%3d, %3d) ", encCount, decCount);
    res = ccMan->GetCharsetTitle(charset->get(), str);
    if (NS_FAILED(res)) str.SetLength(0);
    NS_LossyConvertUTF16toASCII buff2(str);
    printf(" \"%s\"\n", buff2.get());
  }

  printf("%u of %u decoders are basic (%d%%)\n",
         basicDecCount, decCount, (basicDecCount * 100) / decCount);

  printf("%u of %u encoders are basic (%d%%)\n",
         basicEncCount, encCount, (basicEncCount * 100) / encCount);
  mLog.DelTrace(trace);
  return NS_OK;
}

nsresult nsTestUConv::TestTempBug()
{
  char * trace = "TestTempBug";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  NS_NAMED_LITERAL_CSTRING(charset, "ISO-2022-JP");
  PRUnichar src[] = {0x0043, 0x004e, 0x0045, 0x0054, 0x0020, 0x004A, 0x0061, 
    0x0070, 0x0061, 0x006E, 0x0020, 0x7DE8, 0x96C6, 0x5C40};
  PRUnichar * srcEnd = src + ARRAY_SIZE(src);
  char dest[BIG_BUFFER_SIZE];
  char * destEnd = dest + BIG_BUFFER_SIZE;

  PRUnichar * p = src;
  char * q = dest;
  res = Encode(&p, srcEnd, &q, destEnd, charset);

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::Encode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, 
                             char ** aDest, char * aDestEnd, 
                             const nsAFlatCString& aCharset)
{
  char * trace = "Encode";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  nsCOMPtr<nsICharsetConverterManager> ccMan = 
           do_GetService(kCharsetConverterManagerCID, &res);
  if (NS_FAILED(res)) {
    mLog.PrintError("NS_WITH_SERVICE", res);
    return res;
  }

  nsCOMPtr<nsIUnicodeEncoder> enc;
  res = ccMan->GetUnicodeEncoder(aCharset.get(), getter_AddRefs(enc));
  if (NS_FAILED(res)) {
    mLog.PrintError("GetUnicodeEncoder()", res);
    return res;
  }

  res = ConvertEncode(aSrc, aSrcEnd, aDest, aDestEnd, enc);
  if (NS_FAILED(res)) {
    mLog.PrintError("Convert()", res);
    return res;
  }

  res = FinishEncode(aDest, aDestEnd, enc);
  if (NS_FAILED(res)) {
    mLog.PrintError("Finish()", res);
    return res;
  }

  mLog.DelTrace(trace);
  return res;
}

nsresult nsTestUConv::ConvertEncode(PRUnichar ** aSrc, PRUnichar * aSrcEnd, 
                                    char ** aDest, char * aDestEnd, 
                                    nsIUnicodeEncoder * aEncoder)
{
  PRUnichar * src = (*aSrc);
  char * dest = (*aDest);
  PRInt32 srcLen = aSrcEnd - src;
  PRInt32 destLen = aDestEnd - dest;

  nsresult res = aEncoder->Convert(src, &srcLen, dest, &destLen);

  (*aSrc) = src + srcLen;
  (*aDest) = dest + destLen;
  return res;
}

nsresult nsTestUConv::FinishEncode(char ** aDest, char * aDestEnd, 
                                   nsIUnicodeEncoder * aEncoder)
{
  char * dest = (*aDest);
  PRInt32 destLen = aDestEnd - dest;

  nsresult res = aEncoder->Finish(dest, &destLen);

  (*aDest) = dest + destLen;
  return res;
}

void nsTestUConv::PrintSpaces(int aCount)
{
  for (int i = 0; i < aCount; i++) printf(" ");
}

nsresult nsTestUConv::Main(int aArgC, char ** aArgV)
{
  char * trace = "Main";
  mLog.AddTrace(trace);
  nsresult res = NS_OK;

  if (aArgC < 2) {
    
    res = TestCharsetManager();
    if (NS_SUCCEEDED(res)) res = TestEncoders();
    if (NS_SUCCEEDED(res)) res = TestDecoders();
  } else if (!strcmp(aArgV[1], "-tempbug")) {
    
    res = TestTempBug();
  } else if (!strcmp(aArgV[1], "-display")) {
    
    res = DisplayDetectors();
    if (NS_SUCCEEDED(res)) res = DisplayCharsets();
  }

  mLog.DelTrace(trace);
  return res;
}
