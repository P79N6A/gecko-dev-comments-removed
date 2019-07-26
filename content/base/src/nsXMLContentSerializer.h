










#ifndef nsXMLContentSerializer_h__
#define nsXMLContentSerializer_h__

#include "nsIContentSerializer.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"

#define kIndentStr NS_LITERAL_STRING("  ")
#define kEndTag NS_LITERAL_STRING("</")

class nsIAtom;
class nsIDOMNode;
class nsINode;

class nsXMLContentSerializer : public nsIContentSerializer {
 public:
  nsXMLContentSerializer();
  virtual ~nsXMLContentSerializer();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(uint32_t flags, uint32_t aWrapColumn,
                  const char* aCharSet, bool aIsCopying,
                  bool aRewriteEncodingDeclaration);

  NS_IMETHOD AppendText(nsIContent* aText, int32_t aStartOffset,
                        int32_t aEndOffset, nsAString& aStr);

  NS_IMETHOD AppendCDATASection(nsIContent* aCDATASection,
                                int32_t aStartOffset, int32_t aEndOffset,
                                nsAString& aStr);

  NS_IMETHOD AppendProcessingInstruction(nsIContent* aPI,
                                         int32_t aStartOffset,
                                         int32_t aEndOffset,
                                         nsAString& aStr);

  NS_IMETHOD AppendComment(nsIContent* aComment, int32_t aStartOffset,
                           int32_t aEndOffset, nsAString& aStr);
  
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
                          int32_t aStartOffset,
                          int32_t aEndOffset,
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

  uint32_t ScanNamespaceDeclarations(nsIContent* aContent,
                                     nsIContent *aOriginalElement,
                                     const nsAString& aTagNamespaceURI);

  virtual void SerializeAttributes(nsIContent* aContent,
                                   nsIContent *aOriginalElement,
                                   nsAString& aTagPrefix,
                                   const nsAString& aTagNamespaceURI,
                                   nsIAtom* aTagName,
                                   nsAString& aStr,
                                   uint32_t aSkipAttr,
                                   bool aAddNSAttr);

  void SerializeAttr(const nsAString& aPrefix,
                     const nsAString& aName,
                     const nsAString& aValue,
                     nsAString& aStr,
                     bool aDoEscapeEntities);

  bool IsJavaScript(nsIContent * aContent,
                      nsIAtom* aAttrNameAtom,
                      int32_t aAttrNamespaceID,
                      const nsAString& aValueString);

  







  virtual bool CheckElementStart(nsIContent * aContent,
                                   bool & aForceFormat,
                                   nsAString& aStr);

  



  virtual void AppendEndOfElementStart(nsIContent *aOriginalElement,
                                       nsIAtom * aName,
                                       int32_t aNamespaceID,
                                       nsAString& aStr);

  




  virtual void AfterElementStart(nsIContent * aContent,
                                 nsIContent *aOriginalElement,
                                 nsAString& aStr) { };

  







  virtual bool CheckElementEnd(nsIContent * aContent,
                                 bool & aForceFormat,
                                 nsAString& aStr);

  




  virtual void AfterElementEnd(nsIContent * aContent,
                               nsAString& aStr) { };

  


  virtual bool LineBreakBeforeOpen(int32_t aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakAfterOpen(int32_t aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakBeforeClose(int32_t aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakAfterClose(int32_t aNamespaceID, nsIAtom* aName);

  



  void AppendIndentation(nsAString& aStr);
  void IncrIndentation(nsIAtom* aName);
  void DecrIndentation(nsIAtom* aName);

  
  
  void MaybeAddNewlineForRootNode(nsAString& aStr);
  void MaybeFlagNewlineForRootNode(nsINode* aNode);

  
  virtual void MaybeEnterInPreContent(nsIContent* aNode);
  virtual void MaybeLeaveFromPreContent(nsIContent* aNode);

  int32_t mPrefixIndex;

  struct NameSpaceDecl {
    nsString mPrefix;
    nsString mURI;
    nsIContent* mOwner;
  };

  nsTArray<NameSpaceDecl> mNameSpaceStack;

  
  uint32_t  mFlags;

  
  nsString  mLineBreak;

  
  nsCString mCharset;
  
  
  uint32_t   mColPos;

  
  bool mDoFormat;

  
  
  bool mDoRaw;

  
  bool mDoWrap;

  
  uint32_t   mMaxColumn;

  
  nsString   mIndent;

  
  
  int32_t    mIndentOverflow;

  
  bool mIsIndentationAddedOnCurrentLine;

  
  bool mInAttribute;

  
  
  
  bool mAddNewlineForRootNode;

  
  
  
  
  bool          mAddSpace;

  
  
  
  bool          mMayIgnoreLineBreakSequence;

  bool          mBodyOnly;
  int32_t       mInBody;

  
  int32_t       mPreLevel;
};

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer);

#endif 
