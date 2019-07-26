























#ifndef HTMLTOKENS_H
#define HTMLTOKENS_H

#include "nsToken.h"
#include "nsHTMLTags.h"
#include "nsString.h"
#include "nsScannerString.h"

class nsScanner;

  



enum eHTMLTokenTypes {
  eToken_unknown=0,
  eToken_start=1,      eToken_end,          eToken_comment,         eToken_entity,
  eToken_whitespace,   eToken_newline,      eToken_text,            eToken_attribute,
  eToken_instruction,  eToken_cdatasection, eToken_doctypeDecl,     eToken_markupDecl,
  eToken_last 
};

nsresult      ConsumeQuotedString(PRUnichar aChar,nsString& aString,nsScanner& aScanner);
nsresult      ConsumeAttributeText(PRUnichar aChar,nsString& aString,nsScanner& aScanner);
const PRUnichar* GetTagName(int32_t aTag);








class CHTMLToken : public CToken {
public:
  virtual ~CHTMLToken();
  CHTMLToken(eHTMLTags aTag);

  virtual eContainerInfo GetContainerInfo(void) const {return eFormUnknown;}
  virtual void SetContainerInfo(eContainerInfo aInfo) { }

protected:
};







class CStartToken: public CHTMLToken {
public:
  CStartToken(eHTMLTags aTag=eHTMLTag_unknown);
  CStartToken(const nsAString& aString);
  CStartToken(const nsAString& aName,eHTMLTags aTag);

  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTypeID(void);
  virtual int32_t GetTokenType(void);

  virtual bool IsEmpty(void);
  virtual void SetEmpty(bool aValue);

  virtual const nsSubstring& GetStringValue();
  virtual void GetSource(nsString& anOutputString);
  virtual void AppendSourceTo(nsAString& anOutputString);

  
  virtual eContainerInfo GetContainerInfo(void) const {return mContainerInfo;}
  virtual void SetContainerInfo(eContainerInfo aContainerInfo) {
    if (eFormUnknown==mContainerInfo) {
      mContainerInfo=aContainerInfo;
    }
  }
  virtual bool IsWellFormed(void) const {
    return eWellFormed == mContainerInfo;
  }

  nsString mTextValue;
protected:
  eContainerInfo mContainerInfo;
  bool mEmpty;
#ifdef DEBUG
  bool mAttributed;
#endif
};









class CEndToken: public CHTMLToken {
public:
  CEndToken(eHTMLTags aTag);
  CEndToken(const nsAString& aString);
  CEndToken(const nsAString& aName,eHTMLTags aTag);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTypeID(void);
  virtual int32_t GetTokenType(void);

  virtual const nsSubstring& GetStringValue();
  virtual void GetSource(nsString& anOutputString);
  virtual void AppendSourceTo(nsAString& anOutputString);

protected:
  nsString mTextValue;
};










class CCommentToken: public CHTMLToken {
public:
  CCommentToken();
  CCommentToken(const nsAString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);
  virtual void AppendSourceTo(nsAString& anOutputString);

  nsresult ConsumeStrictComment(nsScanner& aScanner);
  nsresult ConsumeQuirksComment(nsScanner& aScanner);

protected:
  nsScannerSubstring mComment; 
  nsScannerSubstring mCommentDecl; 
};









class CEntityToken : public CHTMLToken {
public:
  CEntityToken();
  CEntityToken(const nsAString& aString);
  virtual int32_t GetTokenType(void);
  int32_t TranslateToUnicodeStr(nsString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  static nsresult ConsumeEntity(PRUnichar aChar, nsString& aString,
                                nsScanner& aScanner);
  static int32_t TranslateToUnicodeStr(int32_t aValue,nsString& aString);

  virtual const nsSubstring& GetStringValue(void);
  virtual void GetSource(nsString& anOutputString);
  virtual void AppendSourceTo(nsAString& anOutputString);

protected:
  nsString mTextValue;
};









class CWhitespaceToken: public CHTMLToken {
public:
  CWhitespaceToken();
  CWhitespaceToken(const nsAString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);

protected:
  nsScannerSharedSubstring mTextValue;
};








class CTextToken: public CHTMLToken {
public:
  CTextToken();
  CTextToken(const nsAString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual int32_t GetTextLength(void);
  virtual void CopyTo(nsAString& aStr);
  virtual const nsSubstring& GetStringValue(void);
  virtual void Bind(nsScanner* aScanner, nsScannerIterator& aStart,
                    nsScannerIterator& aEnd);
  virtual void Bind(const nsAString& aStr);

  nsresult ConsumeCharacterData(bool aIgnoreComments,
                                nsScanner& aScanner,
                                const nsAString& aEndTagName,
                                int32_t aFlag,
                                bool& aFlushTokens);

  nsresult ConsumeParsedCharacterData(bool aDiscardFirstNewline,
                                      bool aConservativeConsume,
                                      nsScanner& aScanner,
                                      const nsAString& aEndTagName,
                                      int32_t aFlag,
                                      bool& aFound);

protected:
  nsScannerSubstring mTextValue;
};









class CCDATASectionToken : public CHTMLToken {
public:
  CCDATASectionToken(eHTMLTags aTag = eHTMLTag_unknown);
  CCDATASectionToken(const nsAString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);

protected:
  nsString mTextValue;
};








class CMarkupDeclToken : public CHTMLToken {
public:
  CMarkupDeclToken();
  CMarkupDeclToken(const nsAString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);

protected:
  nsScannerSubstring  mTextValue;
};










class CAttributeToken: public CHTMLToken {
public:
  CAttributeToken();
  CAttributeToken(const nsAString& aString);
  CAttributeToken(const nsAString& aKey, const nsAString& aString);
  ~CAttributeToken() {}
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  const nsSubstring&     GetKey(void) { return mTextKey.AsString(); }
  virtual void SetKey(const nsAString& aKey);
  virtual void BindKey(nsScanner* aScanner, nsScannerIterator& aStart,
                       nsScannerIterator& aEnd);
  const nsSubstring& GetValue(void) {return mTextValue.str();}
  virtual const nsSubstring& GetStringValue(void);
  virtual void GetSource(nsString& anOutputString);
  virtual void AppendSourceTo(nsAString& anOutputString);

  bool mHasEqualWithoutValue;
protected:
  nsScannerSharedSubstring mTextValue;
  nsScannerSubstring mTextKey;
};








class CNewlineToken: public CHTMLToken {
public:
  CNewlineToken();
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);

  static void AllocNewline();
  static void FreeNewline();
};









class CInstructionToken: public CHTMLToken {
public:
  CInstructionToken();
  CInstructionToken(const nsAString& aString);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);

protected:
  nsString mTextValue;
};








class CDoctypeDeclToken: public CHTMLToken {
public:
  CDoctypeDeclToken(eHTMLTags aTag=eHTMLTag_unknown);
  CDoctypeDeclToken(const nsAString& aString,eHTMLTags aTag=eHTMLTag_unknown);
  virtual nsresult Consume(PRUnichar aChar,nsScanner& aScanner,int32_t aMode);
  virtual int32_t GetTokenType(void);
  virtual const nsSubstring& GetStringValue(void);
  virtual void SetStringValue(const nsAString& aStr);

protected:
  nsString mTextValue;
};

#endif
