











































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
                  const char* aCharSet, bool aIsCopying,
                  bool aRewriteEncodingDeclaration);

  NS_IMETHOD AppendText(nsIContent* aText, PRInt32 aStartOffset,
                        PRInt32 aEndOffset, nsAString& aStr);

  NS_IMETHOD AppendCDATASection(nsIContent* aCDATASection,
                                PRInt32 aStartOffset, PRInt32 aEndOffset,
                                nsAString& aStr);

  NS_IMETHOD AppendProcessingInstruction(nsIContent* aPI,
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset,
                                         nsAString& aStr);

  NS_IMETHOD AppendComment(nsIContent* aComment, PRInt32 aStartOffset,
                           PRInt32 aEndOffset, nsAString& aStr);
  
  NS_IMETHOD AppendDoctype(nsIContent *aDoctype,
                           nsAString& aStr);

  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr);

  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr);

  NS_IMETHOD Flush(nsAString& aStr) { return NS_OK; }

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr);

 protected:

  


  void AppendToString(const PRUnichar aChar,
                      nsAString& aOutputStr);

  


  void AppendToString(const nsAString& aStr,
                      nsAString& aOutputStr);

  




  void AppendToStringConvertLF(const nsAString& aStr,
                               nsAString& aOutputStr);

  



  void AppendToStringWrapped(const nsASingleFragmentString& aStr,
                             nsAString& aOutputStr);

  



  void AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
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
          bool &aMayIgnoreStartOfLineWhitespaceSequence,
          nsAString &aOutputStr);

  
  void AppendWrapped_NonWhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          bool &aMayIgnoreStartOfLineWhitespaceSequence,
          bool &aSequenceStartAfterAWhiteSpace,
          nsAString &aOutputStr);

  



  void AppendNewLineToString(nsAString& aOutputStr);


  



  virtual void AppendAndTranslateEntities(const nsAString& aStr,
                                          nsAString& aOutputStr);

  



  nsresult AppendTextData(nsIContent* aNode,
                          PRInt32 aStartOffset,
                          PRInt32 aEndOffset,
                          nsAString& aStr,
                          bool aTranslateEntities);

  virtual nsresult PushNameSpaceDecl(const nsAString& aPrefix,
                                     const nsAString& aURI,
                                     nsIContent* aOwner);
  void PopNameSpaceDeclsFor(nsIContent* aOwner);

  

















  bool ConfirmPrefix(nsAString& aPrefix,
                       const nsAString& aURI,
                       nsIContent* aElement,
                       bool aIsAttribute);
  


  void GenerateNewPrefix(nsAString& aPrefix);

  PRUint32 ScanNamespaceDeclarations(nsIContent* aContent,
                                     nsIContent *aOriginalElement,
                                     const nsAString& aTagNamespaceURI);

  virtual void SerializeAttributes(nsIContent* aContent,
                                   nsIContent *aOriginalElement,
                                   nsAString& aTagPrefix,
                                   const nsAString& aTagNamespaceURI,
                                   nsIAtom* aTagName,
                                   nsAString& aStr,
                                   PRUint32 aSkipAttr,
                                   bool aAddNSAttr);

  void SerializeAttr(const nsAString& aPrefix,
                     const nsAString& aName,
                     const nsAString& aValue,
                     nsAString& aStr,
                     bool aDoEscapeEntities);

  bool IsJavaScript(nsIContent * aContent,
                      nsIAtom* aAttrNameAtom,
                      PRInt32 aAttrNamespaceID,
                      const nsAString& aValueString);

  







  virtual bool CheckElementStart(nsIContent * aContent,
                                   bool & aForceFormat,
                                   nsAString& aStr);

  



  virtual void AppendEndOfElementStart(nsIContent *aOriginalElement,
                                       nsIAtom * aName,
                                       PRInt32 aNamespaceID,
                                       nsAString& aStr);

  




  virtual void AfterElementStart(nsIContent * aContent,
                                 nsIContent *aOriginalElement,
                                 nsAString& aStr) { };

  







  virtual bool CheckElementEnd(nsIContent * aContent,
                                 bool & aForceFormat,
                                 nsAString& aStr);

  




  virtual void AfterElementEnd(nsIContent * aContent,
                               nsAString& aStr) { };

  


  virtual bool LineBreakBeforeOpen(PRInt32 aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakAfterOpen(PRInt32 aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakBeforeClose(PRInt32 aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakAfterClose(PRInt32 aNamespaceID, nsIAtom* aName);

  



  void AppendIndentation(nsAString& aStr);
  void IncrIndentation(nsIAtom* aName);
  void DecrIndentation(nsIAtom* aName);

  
  
  void MaybeAddNewlineForRootNode(nsAString& aStr);
  void MaybeFlagNewlineForRootNode(nsINode* aNode);

  
  virtual void MaybeEnterInPreContent(nsIContent* aNode);
  virtual void MaybeLeaveFromPreContent(nsIContent* aNode);

  PRInt32 mPrefixIndex;

  struct NameSpaceDecl {
    nsString mPrefix;
    nsString mURI;
    nsIContent* mOwner;
  };

  nsTArray<NameSpaceDecl> mNameSpaceStack;

  
  PRUint32  mFlags;

  
  nsString  mLineBreak;

  
  nsCString mCharset;
  
  
  PRUint32   mColPos;

  
  bool mDoFormat;

  
  
  bool mDoRaw;

  
  bool mDoWrap;

  
  PRUint32   mMaxColumn;

  
  nsString   mIndent;

  
  
  PRInt32    mIndentOverflow;

  
  bool mIsIndentationAddedOnCurrentLine;

  
  bool mInAttribute;

  
  
  
  bool mAddNewlineForRootNode;

  
  
  
  
  bool          mAddSpace;

  
  
  
  bool          mMayIgnoreLineBreakSequence;

  bool          mBodyOnly;
  PRInt32       mInBody;

  
  PRInt32       mPreLevel;
};

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer);

#endif 
