






































#include <ctype.h>      

#include "nsXPCOM.h"
#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsIHTMLContentSink.h"
#include "nsIContentSerializer.h"
#include "nsLayoutCID.h"
#include "nsIHTMLToTextSink.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIComponentRegistrar.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"

static NS_DEFINE_IID(kIParserIID, NS_IPARSER_IID);
static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);

int
Compare(nsString& str, nsString& aFileName)
{
  
  
  char* filename = ToNewCString(aFileName);
  FILE* file = fopen(filename, "r");
  if (!file)
  {
    fprintf(stderr, "Can't open file %s", filename);
    perror(" ");
    delete[] filename;
    return 2;
  }
  delete[] filename;

  
  nsString inString;
  int c;
  int index = 0;
  int different = 0;
  while ((c = getc(file)) != EOF)
  {
    inString.Append(PRUnichar(c));
    
    
    if (c == '\n' && str[index] == '\r')
      ++index;
    if (c != str[index++])
    {
      
      
      different = index;
      break;
    }
  }
  if (file != stdin)
    fclose(file);

  if (!different)
    return 0;
  else
  {
    nsAutoString left;
    str.Left(left, different);
    char* cstr = ToNewUTF8String(left);
    printf("Comparison failed at char %d:\n-----\n%s\n-----\n",
           different, cstr);
    Recycle(cstr);
    return 1;
  }
}




nsresult
HTML2text(nsString& inString, nsString& inType, nsString& outType,
          int flags, int wrapCol, nsString& compareAgainst)
{
  nsresult rv = NS_OK;

  nsString outString;

  
  nsIParser* parser;
  rv = CallCreateInstance(kParserCID, &parser);
  if (NS_FAILED(rv))
  {
    printf("Unable to create a parser : 0x%x\n", rv);
    return NS_ERROR_FAILURE;
  }

  
#ifdef USE_SERIALIZER
  nsCAutoString progId(NS_CONTENTSERIALIZER_CONTRACTID_PREFIX);
  progId.AppendWithConversion(outType);

  
  nsCOMPtr<nsIContentSerializer> mSerializer;
  mSerializer = do_CreateInstance(static_cast<const char *>(progId));
  NS_ENSURE_TRUE(mSerializer, NS_ERROR_NOT_IMPLEMENTED);

  mSerializer->Init(flags, wrapCol);

  nsCOMPtr<nsIHTMLContentSink> sink (do_QueryInterface(mSerializer));
  if (!sink)
  {
    printf("Couldn't get content sink!\n");
    return NS_ERROR_UNEXPECTED;
  }
#else 
  nsCOMPtr<nsIContentSink> sink;
  if (!inType.EqualsLiteral("text/html")
      || !outType.EqualsLiteral("text/plain"))
  {
    char* in = ToNewCString(inType);
    char* out = ToNewCString(outType);
    printf("Don't know how to convert from %s to %s\n", in, out);
    Recycle(in);
    Recycle(out);
    return NS_ERROR_FAILURE;
  }

  sink = do_CreateInstance(NS_PLAINTEXTSINK_CONTRACTID);
  NS_ENSURE_TRUE(sink, NS_ERROR_FAILURE);

  nsCOMPtr<nsIHTMLToTextSink> textSink(do_QueryInterface(sink));
  NS_ENSURE_TRUE(textSink, NS_ERROR_FAILURE);

  textSink->Initialize(&outString, flags, wrapCol);
#endif 

  parser->SetContentSink(sink);
  if (!inType.EqualsLiteral("text/html"))
  {
    printf("Don't know how to deal with non-html input!\n");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  rv = parser->Parse(inString, 0, NS_LossyConvertUTF16toASCII(inType), PR_TRUE);
  if (NS_FAILED(rv))
  {
    printf("Parse() failed! 0x%x\n", rv);
    return rv;
  }
  NS_RELEASE(parser);

  if (compareAgainst.Length() > 0)
    return Compare(outString, compareAgainst);

  char* charstar = ToNewUTF8String(outString);
  printf("Output string is:\n--------------------\n%s--------------------\n",
         charstar);
  delete[] charstar;

  return NS_OK;
}



int main(int argc, char** argv)
{
  nsString inType(NS_LITERAL_STRING("text/html"));
  nsString outType(NS_LITERAL_STRING("text/plain"));
  int wrapCol = 72;
  int flags = 0;
  nsString compareAgainst;


  
  const char* progname = argv[0];
  --argc; ++argv;

  
  while (argc > 0 && argv[0][0] == '-')
  {
    switch (argv[0][1])
    {
      case 'h':
        printf("\
Usage: %s [-i intype] [-o outtype] [-f flags] [-w wrapcol] [-c comparison_file] infile\n\
\tIn/out types are mime types (e.g. text/html)\n\
\tcomparison_file is a file against which to compare the output\n\
\n\
\tDefaults are -i text/html -o text/plain -f 0 -w 72 [stdin]\n",
               progname);
        exit(0);

        case 'i':
        if (argv[0][2] != '\0')
          inType.AssignWithConversion(argv[0]+2);
        else {
          inType.AssignWithConversion(argv[1]);
          --argc;
          ++argv;
        }
        break;

      case 'o':
        if (argv[0][2] != '\0')
          outType.AssignWithConversion(argv[0]+2);
        else {
          outType.AssignWithConversion(argv[1]);
          --argc;
          ++argv;
        }
        break;

      case 'w':
        if (isdigit(argv[0][2]))
          wrapCol = atoi(argv[0]+2);
        else {
          wrapCol = atoi(argv[1]);
          --argc;
          ++argv;
        }
        break;

      case 'f':
        if (isdigit(argv[0][2]))
          flags = atoi(argv[0]+2);
        else {
          flags = atoi(argv[1]);
          --argc;
          ++argv;
        }
        break;

      case 'c':
        if (argv[0][2] != '\0')
          compareAgainst.AssignWithConversion(argv[0]+2);
        else {
          compareAgainst.AssignWithConversion(argv[1]);
          --argc;
          ++argv;
        }
        break;
    }
    ++argv;
    --argc;
  }

  FILE* file = 0;
  if (argc > 0)         
  {
    
    
    file = fopen(argv[0], "r");
    if (!file)
    {
      fprintf(stderr, "Can't open file %s", argv[0]);
      perror(" ");
      exit(1);
    }
  }
  else
    file = stdin;

  nsresult ret;
  {
    nsCOMPtr<nsIServiceManager> servMan;
    NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    NS_ASSERTION(registrar, "Null nsIComponentRegistrar");
    registrar->AutoRegister(nsnull);

    
    nsString inString;
    int c;
    while ((c = getc(file)) != EOF)
      inString.Append(PRUnichar(c));

    if (file != stdin)
      fclose(file);

    ret = HTML2text(inString, inType, outType, flags, wrapCol, compareAgainst);
  } 
  
  nsresult rv = NS_ShutdownXPCOM( NULL );
  NS_ASSERTION(NS_SUCCEEDED(rv), "NS_ShutdownXPCOM failed");
  return ret;
}
