

















#ifndef SCANNER
#define SCANNER

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsIParser.h"
#include "nsIUnicodeDecoder.h"
#include "nsScannerString.h"

class nsReadEndCondition {
public:
  const char16_t *mChars;
  char16_t mFilter;
  explicit nsReadEndCondition(const char16_t* aTerminateChars);
private:
  nsReadEndCondition(const nsReadEndCondition& aOther); 
  void operator=(const nsReadEndCondition& aOther); 
};

class nsScanner {
  public:

      


      explicit nsScanner(const nsAString& anHTMLString);

      



      nsScanner(nsString& aFilename, bool aCreateStream);

      ~nsScanner();

      






      nsresult GetChar(char16_t& ch);

      







      nsresult Peek(char16_t& ch, uint32_t aOffset=0);

      nsresult Peek(nsAString& aStr, int32_t aNumChars, int32_t aOffset = 0);

      






      nsresult SkipOver(char16_t aSkipChar);

      





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
                         char16_t aTerminal,
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
                            char16_t aChar);

      





      bool      IsIncremental(void) {return mIncremental;}
      void      SetIncremental(bool anIncrValue) {mIncremental=anIncrValue;}

      




      int32_t FirstNonWhitespacePosition()
      {
        return mFirstNonWhitespacePosition;
      }

      






      void OverrideReplacementCharacter(char16_t aReplacementCharacter);

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
      char16_t       mReplacementCharacter;
      int32_t         mFirstNonWhitespacePosition;
      int32_t         mCharsetSource;
      nsCString       mCharset;
      nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;

  private:
      nsScanner &operator =(const nsScanner &); 
};

#endif


