








































#include "nscore.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeEncoder.h"
#include "nsIUnicodeDecoder.h"

#include "nsICharsetAlias.h"

static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void usage()
{
  printf(
    "nsconv -f fromcode -t tocode infile outfile\n"
    "nsconv -f fromcode -t tocode infile > outfile\n"
    "nsconv -f fromcode -t tocode < infile > outfile\n"
    );
}

#define INBUFSIZE (1024*16)
#define MEDBUFSIZE (1024*16*2)
#define OUTBUFSIZE (1024*16*8)
char inbuffer[INBUFSIZE];
char outbuffer[OUTBUFSIZE];
PRUnichar  medbuffer[MEDBUFSIZE];

int main(int argc, const char** argv)
{
  nsIUnicodeEncoder* encoder = nsnull;
  nsIUnicodeDecoder* decoder = nsnull;
  FILE* fin = 0;
  FILE* fout = 0;
  FILE* infile = 0;
  FILE* outfile = 0;
  nsresult res= NS_OK;

  NS_InitXPCOM2(nsnull, nsnull, nsnull);

  
  nsCOMPtr<nsICharsetConverterManager> ccMain =
      do_GetService(kCharsetConverterManagerCID, &res);
  if(NS_FAILED(res))
  {
    fprintf(stderr, "Cannot get Character Converter Manager %x\n", res);
    return -1;
  }

  
  nsCOMPtr<nsICharsetAlias> aliasmgr =
      do_GetService(NS_CHARSETALIAS_CONTRACTID, &res);
  if (NS_FAILED(res))
  {
    fprintf(stderr, "Cannot get Charset Alias Manager %x\n", res);
    return -1;
  }

  int i;
  if(argc > 4)
  {
    for(i =0; i < argc; i++)
    {
      if(strcmp(argv[i], "-f") == 0)
      {
        
        nsCAutoString str;

        
        
        res = aliasmgr->GetPreferred(nsDependentCString(argv[i+1]), str);
        if (NS_FAILED(res))
        {
          fprintf(stderr, "Cannot get charset alias for %s %x\n",
                  argv[i+1], res);
          goto error_exit;
        }

        
        res = ccMain->GetUnicodeDecoder(str.get(), &decoder);
        if(NS_FAILED(res)) {
          fprintf(stderr, "Cannot get Unicode decoder %s %x\n", 
                  argv[i+1],res);
          goto error_exit;
        }

      }

      if(strcmp(argv[i], "-t") == 0)
      {
        
        nsCAutoString str;

        
        
        res = aliasmgr->GetPreferred(nsDependentCString(argv[i+1]), str);
        if (NS_FAILED(res))
        {
          fprintf(stderr, "Cannot get charset alias for %s %x\n",
                  argv[i+1], res);
          goto error_exit;
        }

        
        res = ccMain->GetUnicodeEncoderRaw(str.get(), &encoder);
        if(NS_FAILED(res)) {
          fprintf(stderr, "Cannot get Unicode encoder %s %x\n", 
                  argv[i+1],res);
          goto error_exit;
        }
      }
    }

    if (argc > 5)
    {
      
      
      fin = infile = fopen(argv[5], "rb");
      if(NULL == infile) 
      {  
        usage();
        fprintf(stderr,"cannot open input file %s\n", argv[5]);
        goto error_exit; 
      }

      if (argc > 6)
      {
        
        
        fout = outfile = fopen(argv[6], "ab");
        if(NULL == outfile) 
        {  
          usage();
          fprintf(stderr,"cannot open output file %s\n", argv[6]);
          goto error_exit; 
        }
      }
      else
        fout = stdout;
    }
    else
    {
      
      
      fin = stdin;
      fout = stdout;
    }
    
    PRInt32 insize,medsize,outsize;
    while((insize=fread(inbuffer, 1,INBUFSIZE, fin)) > 0)
    {
      medsize=MEDBUFSIZE;
        
      res = decoder->Convert(inbuffer,&insize, medbuffer, &medsize);
      if(NS_FAILED(res)) {
        fprintf(stderr, "failed in decoder->Convert %x\n",res);
        goto error_exit;
      }
      outsize = OUTBUFSIZE;
      res = encoder->Convert(medbuffer, &medsize, outbuffer,&outsize);
      if(NS_FAILED(res)) {
        fprintf(stderr, "failed in encoder->Convert %x\n",res);
        goto error_exit;
      }
      fwrite(outbuffer, 1, outsize, fout);

    }
     
    
    if (infile != 0)
      fclose(infile);
    if (outfile != 0)
      fclose(outfile);
    fprintf(stderr, "Done!\n");
    NS_IF_RELEASE(encoder);
    NS_IF_RELEASE(decoder);
    return 0;
  }
  usage();
  error_exit:
  
  if (infile != 0)
    fclose(infile);
  if (outfile != 0)
    fclose(outfile);
  NS_IF_RELEASE(encoder);
  NS_IF_RELEASE(decoder);
  return -1;
}
