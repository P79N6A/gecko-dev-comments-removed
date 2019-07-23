





































 
#include "nsEPSObjectPS.h"
#include "prprf.h"













nsEPSObjectPS::nsEPSObjectPS(FILE *aFile) :
  mStatus(NS_ERROR_INVALID_ARG),
  mEPSF(aFile),
  mBBllx(0.0),
  mBBlly(0.0),
  mBBurx(0.0),
  mBBury(0.0)
{
  NS_PRECONDITION(aFile != nsnull,   "aFile == nsnull");
  NS_PRECONDITION(0 == fseek(aFile, 0L, SEEK_SET), "File isn't seekable");    
  Parse();
}


















PRBool
nsEPSObjectPS::EPSFFgets(nsACString& aBuffer)
{
  aBuffer.Truncate();
  while (1) {
    int ch = getc(mEPSF);
    if ('\n' == ch) {
      
      ch = getc(mEPSF);
      if ((EOF != ch) && ('\r' != ch))
        ungetc(ch, mEPSF);
      return PR_TRUE;
    }
    else if ('\r' == ch) {
      
      ch = getc(mEPSF);
      if ((EOF != ch) && ('\n' != ch))
        ungetc(ch, mEPSF);
      return PR_TRUE;
    }
    else if (EOF == ch) {
      
      return !aBuffer.IsEmpty();
    }

    
    aBuffer.Append((char)ch);
  }
}






void
nsEPSObjectPS::Parse()
{
  nsCAutoString line;

  NS_PRECONDITION(nsnull != mEPSF, "No file");

  rewind(mEPSF);
  while (EPSFFgets(line)) {
    if (PR_sscanf(line.get(), "%%%%BoundingBox: %lf %lf %lf %lf",
                  &mBBllx, &mBBlly, &mBBurx, &mBBury) == 4) {
      mStatus = NS_OK;
      return;
    }
  }
  mStatus = NS_ERROR_INVALID_ARG;
}







nsresult
nsEPSObjectPS::WriteTo(FILE *aDest)
{
  NS_PRECONDITION(NS_SUCCEEDED(mStatus), "Bad status");

  nsCAutoString line;
  PRBool        inPreview = PR_FALSE;

  rewind(mEPSF);
  while (EPSFFgets(line)) {
    if (inPreview) {
      
      if (StringBeginsWith(line, NS_LITERAL_CSTRING("%%EndPreview")))
          inPreview = PR_FALSE;
      continue;
    }
    else if (StringBeginsWith(line, NS_LITERAL_CSTRING("%%BeginPreview:"))){
      inPreview = PR_TRUE;
      continue;
    }

    
    fwrite(line.get(), line.Length(), 1, aDest);
    putc('\n', aDest);
  }
  return NS_OK;
}


