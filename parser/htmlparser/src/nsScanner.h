

















































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

      









      nsScanner(nsString& aFilename,bool aCreateStream, const nsACString& aCharset, PRInt32 aSource);

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
                              bool& aHaveCR);
      nsresult ReadWhitespace(nsScannerIterator& aStart, 
                              nsScannerIterator& aEnd,
                              PRInt32& aNewlinesSkipped);

      








      nsresult ReadUntil(nsAString& aString,
                         PRUnichar aTerminal,
                         bool addTerminal);

      









      nsresult ReadUntil(nsAString& aString,
                         const nsReadEndCondition& aEndCondition, 
                         bool addTerminal);

      nsresult ReadUntil(nsScannerSharedSubstring& aString,
                         const nsReadEndCondition& aEndCondition,
                         bool addTerminal);

      nsresult ReadUntil(nsScannerIterator& aStart,
                         nsScannerIterator& aEnd,
                         const nsReadEndCondition& aEndCondition, 
                         bool addTerminal);

      








      PRInt32 Mark(void);

      









      void RewindToMark(void);


      






      bool UngetReadable(const nsAString& aBuffer);

      






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
                       bool aTruncate = false,
                       bool aReverse = false);
      void ReplaceCharacter(nsScannerIterator& aPosition,
                            PRUnichar aChar);

      





      bool      IsIncremental(void) {return mIncremental;}
      void      SetIncremental(bool anIncrValue) {mIncremental=anIncrValue;}

      




      PRInt32 FirstNonWhitespacePosition()
      {
        return mFirstNonWhitespacePosition;
      }

      






      void OverrideReplacementCharacter(PRUnichar aReplacementCharacter);

  protected:

      bool AppendToBuffer(nsScannerString::Buffer *, nsIRequest *aRequest, PRInt32 aErrorPos = -1);
      bool AppendToBuffer(const nsAString& aStr)
      {
        nsScannerString::Buffer* buf = nsScannerString::AllocBufferFromString(aStr);
        if (!buf)
          return PR_FALSE;
        AppendToBuffer(buf, nsnull);
        return PR_TRUE;
      }

      nsScannerString*             mSlidingBuffer;
      nsScannerIterator            mCurrentPosition; 
      nsScannerIterator            mMarkPosition;    
      nsScannerIterator            mEndPosition;     
      nsScannerIterator            mFirstInvalidPosition; 
      nsString        mFilename;
      PRUint32        mCountRemaining; 
                                       
      bool            mIncremental;
      bool            mHasInvalidCharacter;
      PRUnichar       mReplacementCharacter;
      PRInt32         mFirstNonWhitespacePosition;
      PRInt32         mCharsetSource;
      nsCString       mCharset;
      nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;

  private:
      nsScanner &operator =(const nsScanner &); 
};

#endif


