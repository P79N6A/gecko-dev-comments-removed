

















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

      










      nsScanner(const nsAString& anHTMLString, const nsACString& aCharset, int32_t aSource);

      









      nsScanner(nsString& aFilename,bool aCreateStream, const nsACString& aCharset, int32_t aSource);

      ~nsScanner();

      






      nsresult GetChar(PRUnichar& ch);

      







      nsresult Peek(PRUnichar& ch, uint32_t aOffset=0);

      nsresult Peek(nsAString& aStr, int32_t aNumChars, int32_t aOffset = 0);

      






      nsresult SkipOver(PRUnichar aSkipChar);

      





      nsresult SkipWhitespace(int32_t& aNewlinesSkipped);

      





      nsresult ReadTagIdentifier(nsScannerSharedSubstring& aString);

      






      nsresult ReadEntityIdentifier(nsString& aString);
      nsresult ReadNumber(nsString& aString,int32_t aBase);
      nsresult ReadWhitespace(nsScannerSharedSubstring& aString, 
                              int32_t& aNewlinesSkipped,
                              bool& aHaveCR);
      nsresult ReadWhitespace(nsScannerIterator& aStart, 
                              nsScannerIterator& aEnd,
                              int32_t& aNewlinesSkipped);

      








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

      








      int32_t Mark(void);

      









      void RewindToMark(void);


      






      bool UngetReadable(const nsAString& aBuffer);

      






      nsresult Append(const nsAString& aBuffer);

      






      nsresult Append(const char* aBuffer, uint32_t aLen,
                      nsIRequest *aRequest);

      







      void CopyUnusedData(nsString& aCopyBuffer);

      







      nsString& GetFilename(void);

      static void SelfTest();

      







      nsresult SetDocumentCharset(const nsACString& aCharset, int32_t aSource);

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

      




      int32_t FirstNonWhitespacePosition()
      {
        return mFirstNonWhitespacePosition;
      }

      






      void OverrideReplacementCharacter(PRUnichar aReplacementCharacter);

  protected:

      bool AppendToBuffer(nsScannerString::Buffer *, nsIRequest *aRequest, int32_t aErrorPos = -1);
      bool AppendToBuffer(const nsAString& aStr)
      {
        nsScannerString::Buffer* buf = nsScannerString::AllocBufferFromString(aStr);
        if (!buf)
          return false;
        AppendToBuffer(buf, nullptr);
        return true;
      }

      nsScannerString*             mSlidingBuffer;
      nsScannerIterator            mCurrentPosition; 
      nsScannerIterator            mMarkPosition;    
      nsScannerIterator            mEndPosition;     
      nsScannerIterator            mFirstInvalidPosition; 
      nsString        mFilename;
      uint32_t        mCountRemaining; 
                                       
      bool            mIncremental;
      bool            mHasInvalidCharacter;
      PRUnichar       mReplacementCharacter;
      int32_t         mFirstNonWhitespacePosition;
      int32_t         mCharsetSource;
      nsCString       mCharset;
      nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;

  private:
      nsScanner &operator =(const nsScanner &); 
};

#endif


