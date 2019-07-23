





































#include <stdio.h>
#include <stdlib.h>
#if defined(XP_WIN) || defined(XP_OS2)
#include <io.h>
#endif
#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>
#endif
#include "nscore.h"
#include "nsUniversalDetector.h"

#define MAXBSIZE (1L << 13)

void usage() {
   printf("Usage: DetectFile blocksize\n"
        "    blocksize: 1 ~ %ld\n"
          "  Data are passed in from STDIN\n"
          ,  MAXBSIZE);
}

class nsUniversalChardetTest : public nsUniversalDetector
{
 public:
   nsUniversalChardetTest() { };
   virtual ~nsUniversalChardetTest() { };

  PRBool done() const { return mDone; }

 private:
   virtual void Report(const char* aCharset)
    {
        printf("RESULT CHARSET : %s\n", aCharset);
    };
};

int main(int argc, char** argv) {
  char buf[MAXBSIZE];
  PRUint32 bs;
  if( 2 != argc )
  {
    usage();
    printf("Need 1 arguments\n");
    return(-1);
  }
  bs = atoi(argv[1]);
  if((bs <1)||(bs>MAXBSIZE))
  {
    usage();
    printf("blocksize out of range - %s\n", argv[2]);
    return(-1);
  }
  nsresult rev = NS_OK;
  nsUniversalChardetTest *det = new nsUniversalChardetTest;
  if(nsnull == det){
    usage();
    printf("Error: Could not find Universal Detector\n");
    return(-1);
  }

  size_t sz;
  PRBool done = PR_FALSE;
  do
  {
    sz = read(0, buf, bs);
    if(sz > 0) {
      if(! done) {
        rev = det->HandleData( buf, sz);
        if(NS_FAILED(rev))
        {
          printf("HANDLEDATA ERROR CODE = %x\n", rev);
          return(-1);
        }
      }
    }
  } while((sz > 0) &&  (!det->done()) );
  
  det->DataEnd();

  return (0);
}
