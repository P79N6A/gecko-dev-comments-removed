






#ifndef nsCSSScanner_h___
#define nsCSSScanner_h___

#include "nsString.h"

namespace mozilla {
namespace css {
class ErrorReporter;
}
}







enum nsCSSTokenType {
  
  
  
  eCSSToken_Whitespace,     

  
  
  
  
  
  eCSSToken_Ident,          
  eCSSToken_Function,       
  eCSSToken_AtKeyword,      
  eCSSToken_ID,             
  eCSSToken_Hash,           

  
  
  
  
  
  
  
  
  
  
  eCSSToken_Number,         
  eCSSToken_Dimension,      
  eCSSToken_Percentage,     

  
  
  
  
  
  eCSSToken_String,         
  eCSSToken_Bad_String,     
  eCSSToken_URL,            
  eCSSToken_Bad_URL,        

  
  eCSSToken_Symbol,         

  
  
  
  
  eCSSToken_Includes,       
  eCSSToken_Dashmatch,      
  eCSSToken_Beginsmatch,    
  eCSSToken_Endsmatch,      
  eCSSToken_Containsmatch,  

  
  
  
  
  
  
  
  
  eCSSToken_URange,         

  
  
  
  
  
  
  eCSSToken_HTMLComment,    
};









enum nsCSSTokenSerializationType {
  eCSSTokenSerialization_Nothing,
  eCSSTokenSerialization_Whitespace,
  eCSSTokenSerialization_AtKeyword_or_Hash,
  eCSSTokenSerialization_Number,
  eCSSTokenSerialization_Dimension,
  eCSSTokenSerialization_Percentage,
  eCSSTokenSerialization_URange,
  eCSSTokenSerialization_URL_or_BadURL,
  eCSSTokenSerialization_Function,
  eCSSTokenSerialization_Ident,
  eCSSTokenSerialization_CDC,
  eCSSTokenSerialization_DashMatch,
  eCSSTokenSerialization_ContainsMatch,
  eCSSTokenSerialization_Symbol_Hash,         
  eCSSTokenSerialization_Symbol_At,           
  eCSSTokenSerialization_Symbol_Dot_or_Plus,  
  eCSSTokenSerialization_Symbol_Minus,        
  eCSSTokenSerialization_Symbol_OpenParen,    
  eCSSTokenSerialization_Symbol_Question,     
  eCSSTokenSerialization_Symbol_Assorted,     
  eCSSTokenSerialization_Symbol_Equals,       
  eCSSTokenSerialization_Symbol_Bar,          
  eCSSTokenSerialization_Symbol_Slash,        
  eCSSTokenSerialization_Symbol_Asterisk,     
  eCSSTokenSerialization_Other                
};




struct nsCSSToken {
  nsAutoString    mIdent;
  float           mNumber;
  int32_t         mInteger;
  int32_t         mInteger2;
  nsCSSTokenType  mType;
  char16_t       mSymbol;
  bool            mIntegerValid;
  bool            mHasSign;

  nsCSSToken()
    : mNumber(0), mInteger(0), mInteger2(0), mType(eCSSToken_Whitespace),
      mSymbol('\0'), mIntegerValid(false), mHasSign(false)
  {}

  bool IsSymbol(char16_t aSymbol) const {
    return mType == eCSSToken_Symbol && mSymbol == aSymbol;
  }

  void AppendToString(nsString& aBuffer) const;
};


class nsCSSScannerPosition {
  friend class nsCSSScanner;
public:
  nsCSSScannerPosition() : mInitialized(false) { }

  uint32_t LineNumber() {
    MOZ_ASSERT(mInitialized);
    return mLineNumber;
  }

  uint32_t LineOffset() {
    MOZ_ASSERT(mInitialized);
    return mLineOffset;
  }

private:
  uint32_t mOffset;
  uint32_t mLineNumber;
  uint32_t mLineOffset;
  uint32_t mTokenLineNumber;
  uint32_t mTokenLineOffset;
  uint32_t mTokenOffset;
  bool mInitialized;
};




class nsCSSScanner {
  public:
  
  
  nsCSSScanner(const nsAString& aBuffer, uint32_t aLineNumber);
  ~nsCSSScanner();

  void SetErrorReporter(mozilla::css::ErrorReporter* aReporter) {
    mReporter = aReporter;
  }
  
  void SetSVGMode(bool aSVGMode) {
    mSVGMode = aSVGMode;
  }
  bool IsSVGMode() const {
    return mSVGMode;
  }

  
  void ClearSeenBadToken() { mSeenBadToken = false; }
  bool SeenBadToken() const { return mSeenBadToken; }

  
  void ClearSeenVariableReference() { mSeenVariableReference = false; }
  bool SeenVariableReference() const { return mSeenVariableReference; }

  
  
  uint32_t GetLineNumber() const { return mTokenLineNumber; }

  
  
  uint32_t GetColumnNumber() const
  { return mTokenOffset - mTokenLineOffset; }

  
  
  nsDependentSubstring GetCurrentLine() const;

  
  
  
  bool Next(nsCSSToken& aTokenResult, bool aSkipWS);

  
  
  
  
  
  void NextURL(nsCSSToken& aTokenResult);

  
  
  
  
  
  
  
  void Backup(uint32_t n);

  
  void StartRecording();

  
  void StopRecording();

  
  
  void StopRecording(nsString& aBuffer);

  
  uint32_t RecordingLength() const;

#ifdef DEBUG
  bool IsRecording() const;
#endif

  
  void SavePosition(nsCSSScannerPosition& aState);

  
  void RestoreSavedPosition(const nsCSSScannerPosition& aState);

  enum EOFCharacters {
    eEOFCharacters_None =                    0x0000,

    
    eEOFCharacters_DropBackslash =           0x0001,

    
    eEOFCharacters_ReplacementChar =         0x0002,

    
    eEOFCharacters_Asterisk =                0x0004,
    eEOFCharacters_Slash =                   0x0008,

    
    eEOFCharacters_DoubleQuote =             0x0010,

    
    eEOFCharacters_SingleQuote =             0x0020,

    
    eEOFCharacters_CloseParen =              0x0040,
  };

  
  
  
  
  static void AppendImpliedEOFCharacters(EOFCharacters aEOFCharacters,
                                         nsAString& aString);

  EOFCharacters GetEOFCharacters() const {
#ifdef DEBUG
    AssertEOFCharactersValid(mEOFCharacters);
#endif
    return mEOFCharacters;
  }

#ifdef DEBUG
  static void AssertEOFCharactersValid(uint32_t c);
#endif

protected:
  int32_t Peek(uint32_t n = 0);
  void Advance(uint32_t n = 1);
  void AdvanceLine();

  void SkipWhitespace();
  void SkipComment();

  bool GatherEscape(nsString& aOutput, bool aInString);
  bool GatherText(uint8_t aClass, nsString& aIdent);

  bool ScanIdent(nsCSSToken& aResult);
  bool ScanAtKeyword(nsCSSToken& aResult);
  bool ScanHash(nsCSSToken& aResult);
  bool ScanNumber(nsCSSToken& aResult);
  bool ScanString(nsCSSToken& aResult);
  bool ScanURange(nsCSSToken& aResult);

  void SetEOFCharacters(uint32_t aEOFCharacters);
  void AddEOFCharacters(uint32_t aEOFCharacters);

  const char16_t *mBuffer;
  uint32_t mOffset;
  uint32_t mCount;

  uint32_t mLineNumber;
  uint32_t mLineOffset;

  uint32_t mTokenLineNumber;
  uint32_t mTokenLineOffset;
  uint32_t mTokenOffset;

  uint32_t mRecordStartOffset;
  EOFCharacters mEOFCharacters;

  mozilla::css::ErrorReporter *mReporter;

  
  bool mSVGMode;
  bool mRecording;
  bool mSeenBadToken;
  bool mSeenVariableReference;
};



struct MOZ_STACK_CLASS nsCSSGridTemplateAreaToken {
  nsAutoString mName;  
  bool isTrash;  
};


class nsCSSGridTemplateAreaScanner {
public:
  explicit nsCSSGridTemplateAreaScanner(const nsAString& aBuffer);

  
  
  bool Next(nsCSSGridTemplateAreaToken& aTokenResult);

private:
  const char16_t *mBuffer;
  uint32_t mOffset;
  uint32_t mCount;
};

#endif 
