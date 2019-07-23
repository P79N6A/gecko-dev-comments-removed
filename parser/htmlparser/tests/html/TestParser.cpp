




































#include "nsXPCOM.h"
#include "nsIComponentManager.h"
#include "nsComponentManagerUtils.h"
#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsILoggingSink.h"
#include "nsIInputStream.h"
#include "nsCOMPtr.h"


static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);
static NS_DEFINE_CID(kLoggingSinkCID, NS_LOGGING_SINK_CID);



nsresult ParseData(char* anInputStream,char* anOutputStream) {
  NS_ENSURE_ARG_POINTER(anInputStream);
  NS_ENSURE_ARG_POINTER(anOutputStream);
	
  nsresult result = NS_OK;

  
  nsCOMPtr<nsIParser> parser(do_CreateInstance(kParserCID, &result));
  if (NS_FAILED(result)) {
    printf("\nUnable to create a parser\n");
    return result;
  }
  
  nsCOMPtr<nsILoggingSink> sink(do_CreateInstance(kLoggingSinkCID, &result));
  if (NS_FAILED(result)) {
    printf("\nUnable to create a sink\n");
    return result;
  }

  PRFileDesc* in = PR_Open(anInputStream, PR_RDONLY, 0777);
  if (!in) {
    printf("\nUnable to open input file - %s\n", anInputStream);
    return result;
  }
  
  PRFileDesc* out = PR_Open(anOutputStream,
                            PR_CREATE_FILE|PR_TRUNCATE|PR_RDWR, 0777);
  if (!out) {
    printf("\nUnable to open output file - %s\n", anOutputStream);
    return result;
  }

  nsString stream;
  char buffer[1024] = {0}; 
  PRBool done = PR_FALSE;
  PRInt32 length = 0;
  while(!done) {
    length = PR_Read(in, buffer, sizeof(buffer));
    if (length != 0) {
      stream.Append(NS_ConvertUTF8toUTF16(buffer, length));
    }
    else {
      done=PR_TRUE;
    }
  }

  sink->SetOutputStream(out);
  parser->SetContentSink(sink);
  result = parser->Parse(stream, 0, NS_LITERAL_CSTRING("text/html"), PR_TRUE);
  
  PR_Close(in);
  PR_Close(out);

  return result;
}




int main(int argc, char** argv)
{
  if (argc < 3) {
    printf("\nUsage: <inputfile> <outputfile>\n"); 
    return -1;
  }

  nsresult rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
  if (NS_FAILED(rv)) {
    printf("NS_InitXPCOM2 failed\n");
    return -1;
  }

  ParseData(argv[1],argv[2]);

  return 0;
}
