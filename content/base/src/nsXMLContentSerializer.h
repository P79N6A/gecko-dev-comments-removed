











































#ifndef nsXMLContentSerializer_h__
#define nsXMLContentSerializer_h__

#include "nsIContent.h"
#include "nsIContentSerializer.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIParser.h"

#define kIndentStr NS_LITERAL_STRING("  ")
#define kEndTag NS_LITERAL_STRING("</")

class nsIDOMNode;
class nsIAtom;

class nsXMLContentSerializer : public nsIContentSerializer {
 public:
  nsXMLContentSerializer();
  virtual ~nsXMLContentSerializer();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(PRUint32 flags, PRUint32 aWrapColumn,
                  const char* aCharSet, PRBool aIsCopying,
                  PRBool aRewriteEncodingDeclaration);

  NS_IMETHOD AppendText(nsIDOMText* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);

  NS_IMETHOD AppendCDATASection(nsIDOMCDATASection* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr);

  NS_IMETHOD AppendProcessingInstruction(nsIDOMProcessingInstruction* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr);

  NS_IMETHOD AppendComment(nsIDOMComment* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr);
  
  NS_IMETHOD AppendDoctype(nsIDOMDocumentType *aDoctype,
                           nsAString& aStr);

  NS_IMETHOD AppendElementStart(nsIDOMElement *aElement,
                                nsIDOMElement *aOriginalElement,
                                nsAString& aStr);
  
  NS_IMETHOD AppendElementEnd(nsIDOMElement *aElement,
                              nsAString& aStr);

  NS_IMETHOD Flush(nsAString& aStr) { return NS_OK; }

  NS_IMETHOD AppendDocumentStart(nsIDOMDocument *aDocument,
                                 nsAString& aStr);

 protected:

  


  virtual void AppendToString(const PRUnichar* aStr,
                              PRInt32 aLength,
                              nsAString& aOutputStr);

  


  virtual void AppendToString(const PRUnichar aChar,
                              nsAString& aOutputStr);

  


  virtual void AppendToString(const nsAString& aStr,
                              nsAString& aOutputStr);

  




  virtual void AppendToStringConvertLF(const nsAString& aStr,
                                       nsAString& aOutputStr);

  



  virtual void AppendToStringWrapped(const nsASingleFragmentString& aStr,
                                     nsAString& aOutputStr);

  



  virtual void AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
                                             nsAString& aOutputStr);

  
  void AppendWrapped_WhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          nsAString &aOutputStr);

  
  void AppendFormatedWrapped_WhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          PRBool &aMayIgnoreStartOfLineWhitespaceSequence,
          nsAString &aOutputStr);

  
  void AppendWrapped_NonWhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          PRBool &aMayIgnoreStartOfLineWhitespaceSequence,
          PRBool &aSequenceStartAfterAWhiteSpace,
          nsAString &aOutputStr);

  



  virtual void AppendNewLineToString(nsAString& aOutputStr);


  



  virtual void AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr);

  



  nsresult AppendTextData(nsIDOMNode* aNode,
                          PRInt32 aStartOffset,
                          PRInt32 aEndOffset,
                          nsAString& aStr,
                          PRBool aTranslateEntities);

  virtual nsresult PushNameSpaceDecl(const nsAString& aPrefix,
                                     const nsAString& aURI,
                                     nsIDOMElement* aOwner);
  void PopNameSpaceDeclsFor(nsIDOMElement* aOwner);

  

















  PRBool ConfirmPrefix(nsAString& aPrefix,
                       const nsAString& aURI,
                       nsIDOMElement* aElement,
                       PRBool aIsAttribute);
  


  void GenerateNewPrefix(nsAString& aPrefix);

  PRUint32 ScanNamespaceDeclarations(nsIContent* aContent,
                                     nsIDOMElement *aOriginalElement,
                                     const nsAString& aTagNamespaceURI);

  virtual void SerializeAttributes(nsIContent* aContent,
                                   nsIDOMElement *aOriginalElement,
                                   nsAString& aTagPrefix,
                                   const nsAString& aTagNamespaceURI,
                                   nsIAtom* aTagName,
                                   nsAString& aStr,
                                   PRUint32 aSkipAttr,
                                   PRBool aAddNSAttr);

  void SerializeAttr(const nsAString& aPrefix,
                     const nsAString& aName,
                     const nsAString& aValue,
                     nsAString& aStr,
                     PRBool aDoEscapeEntities);

  virtual PRBool IsJavaScript(nsIContent * aContent,
                              nsIAtom* aAttrNameAtom,
                              PRInt32 aAttrNamespaceID,
                              const nsAString& aValueString);

  







  virtual PRBool CheckElementStart(nsIContent * aContent,
                                   PRBool & aForceFormat,
                                   nsAString& aStr);

  



  virtual void AppendEndOfElementStart(nsIDOMElement *aOriginalElement,
                                       nsIAtom * aName,
                                       PRInt32 aNamespaceID,
                                       nsAString& aStr);

  




  virtual void AfterElementStart(nsIContent * aContent,
                                 nsIDOMElement *aOriginalElement,
                                 nsAString& aStr) { };

  







  virtual PRBool CheckElementEnd(nsIContent * aContent,
                                 PRBool & aForceFormat,
                                 nsAString& aStr);

  




  virtual void AfterElementEnd(nsIContent * aContent,
                               nsAString& aStr) { };

  


  virtual PRBool LineBreakBeforeOpen(PRInt32 aNamespaceID, nsIAtom* aName);

  


  virtual PRBool LineBreakAfterOpen(PRInt32 aNamespaceID, nsIAtom* aName);

  


  virtual PRBool LineBreakBeforeClose(PRInt32 aNamespaceID, nsIAtom* aName);

  


  virtual PRBool LineBreakAfterClose(PRInt32 aNamespaceID, nsIAtom* aName);

  



  void AppendIndentation(nsAString& aStr);
  virtual void IncrIndentation(nsIAtom* aName);
  virtual void DecrIndentation(nsIAtom* aName);

  
  
  void MaybeAddNewlineForRootNode(nsAString& aStr);
  void MaybeFlagNewlineForRootNode(nsIDOMNode* aNode);

  
  virtual void MaybeEnterInPreContent(nsIContent* aNode);
  virtual void MaybeLeaveFromPreContent(nsIContent* aNode);

  PRInt32 mPrefixIndex;

  struct NameSpaceDecl {
    nsString mPrefix;
    nsString mURI;
    nsIDOMElement* mOwner;
  };

  nsTArray<NameSpaceDecl> mNameSpaceStack;

  
  PRUint32  mFlags;

  
  nsString  mLineBreak;

  
  nsCString mCharset;
  
  
  PRInt32   mColPos;

  
  PRPackedBool mDoFormat;

  
  
  PRPackedBool mDoRaw;

  
  PRPackedBool mDoWrap;

  
  PRInt32   mMaxColumn;

  
  nsString   mIndent;

  
  
  PRInt32    mIndentOverflow;

  
  PRPackedBool mIsIndentationAddedOnCurrentLine;

  
  PRPackedBool mInAttribute;

  
  
  
  PRPackedBool mAddNewlineForRootNode;

  
  
  
  
  PRPackedBool  mAddSpace;

  
  
  
  PRPackedBool  mMayIgnoreLineBreakSequence;

  
  PRInt32       mPreLevel;
};

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer);

#endif 
