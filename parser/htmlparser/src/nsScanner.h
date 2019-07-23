

















































#ifndef SCANNER
#define SCANNER

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIParser.h"
#include "prtypes.h"
#include "nsIUnicodeDecoder.h"
#include "nsScannerString.h"

class nsParser;

class nsReadEndCondition {
public:
  const PRUnichar *mChars;
  PRUnichar mFilter;
  explicit nsReadEndCondition(const PRUnichar* aTerminateChars);
private:
  nsReadEndCondition(const nsReadEndCondition& aOther); 
  void operator=(const nsReadEndCondition& aOther); 
};

class nsScanner {
  public:

      










      nsScanner(const nsAString& anHTMLString, const nsACString& aCharset, PRInt32 aSource);

      









      nsScanner(nsString& aFilename,PRBool aCreateStream, const nsACString& aCharset, PRInt32 aSource);

      ~nsScanner();

      






      nsresult GetChar(PRUnichar& ch);

      







      nsresult Peek(PRUnichar& ch, PRUint32 aOffset=0);

      nsresult Peek(nsAString& aStr, PRInt32 aNumChars, PRInt32 aOffset = 0);

      






      nsresult SkipOver(PRUnichar aSkipChar);

      





      nsresult SkipWhitespace(PRInt32& aNewlinesSkipped);

      





      nsresult ReadTagIdentifier(nsScannerSharedSubstring& aString);

      






      nsresult ReadEntityIdentifier(nsString& aString);
      nsresult ReadNumber(nsString& aString,PRInt32 aBase);
      nsresult ReadWhitespace(nsScannerSharedSubstring& aString, 
                              PRInt32& aNewlinesSkipped,
                              PRBool& aHaveCR);
      nsresult ReadWhitespace(nsScannerIterator& aStart, 
                              nsScannerIterator& aEnd,
                              PRInt32& aNewlinesSkipped);

      








      nsresult ReadUntil(nsAString& aString,
                         PRUnichar aTerminal,
                         PRBool addTerminal);

      









      nsresult ReadUntil(nsAString& aString,
                         const nsReadEndCondition& aEndCondition, 
                         PRBool addTerminal);

      nsresult ReadUntil(nsScannerSharedSubstring& aString,
                         const nsReadEndCondition& aEndCondition,
                         PRBool addTerminal);

      nsresult ReadUntil(nsScannerIterator& aStart,
                         nsScannerIterator& aEnd,
                         const nsReadEndCondition& aEndCondition, 
                         PRBool addTerminal);

      








      void Mark(void);

      









      void RewindToMark(void);


      






      PRBool UngetReadable(const nsAString& aBuffer);

      






      nsresult Append(const nsAString& aBuffer);

      






      nsresult Append(const char* aBuffer, PRUint32 aLen,
                      nsIRequest *aRequest);

      







      void CopyUnusedData(nsString& aCopyBuffer);

      







      nsString& GetFilename(void);

      static void SelfTest();

      







      nsresult SetDocumentCharset(const nsACString& aCharset, PRInt32 aSource);

      void BindSubstring(nsScannerSubstring& aSubstring, const nsScannerIterator& aStart, const nsScannerIterator& aEnd);
      void CurrentPosition(nsScannerIterator& aPosition);
      void EndReading(nsScannerIterator& aPosition);
      void SetPosition(nsScannerIterator& aPosition,
                       PRBool aTruncate = PR_FALSE,
                       PRBool aReverse = PR_FALSE);
      void ReplaceCharacter(nsScannerIterator& aPosition,
                            PRUnichar aChar);

      





      PRBool    IsIncremental(void) {return mIncremental;}
      void      SetIncremental(PRBool anIncrValue) {mIncremental=anIncrValue;}

      




      PRInt32 FirstNonWhitespacePosition()
      {
        return mFirstNonWhitespacePosition;
      }

      void SetParser(nsParser *aParser)
      {
        mParser = aParser;
      }

  protected:

      void AppendToBuffer(nsScannerString::Buffer *, nsIRequest *aRequest);
      void AppendToBuffer(const nsAString& aStr)
      {
        AppendToBuffer(nsScannerString::AllocBufferFromString(aStr), nsnull);
      }

      nsScannerString*             mSlidingBuffer;
      nsScannerIterator            mCurrentPosition; 
      nsScannerIterator            mMarkPosition;    
      nsScannerIterator            mEndPosition;     
      nsString        mFilename;
      PRUint32        mCountRemaining; 
                                       
      PRPackedBool    mIncremental;
      PRInt32         mFirstNonWhitespacePosition;
      PRInt32         mCharsetSource;
      nsCString       mCharset;
      nsIUnicodeDecoder *mUnicodeDecoder;
      nsParser        *mParser;
};

#endif


