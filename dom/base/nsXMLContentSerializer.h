










#ifndef nsXMLContentSerializer_h__
#define nsXMLContentSerializer_h__

#include "mozilla/Attributes.h"
#include "nsIContentSerializer.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsString.h"

#define kIndentStr NS_LITERAL_STRING("  ")
#define kEndTag NS_LITERAL_STRING("</")

class nsIAtom;
class nsINode;

class nsXMLContentSerializer : public nsIContentSerializer {
 public:
  nsXMLContentSerializer();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(uint32_t flags, uint32_t aWrapColumn,
                  const char* aCharSet, bool aIsCopying,
                  bool aRewriteEncodingDeclaration) override;

  NS_IMETHOD AppendText(nsIContent* aText, int32_t aStartOffset,
                        int32_t aEndOffset, nsAString& aStr) override;

  NS_IMETHOD AppendCDATASection(nsIContent* aCDATASection,
                                int32_t aStartOffset, int32_t aEndOffset,
                                nsAString& aStr) override;

  NS_IMETHOD AppendProcessingInstruction(nsIContent* aPI,
                                         int32_t aStartOffset,
                                         int32_t aEndOffset,
                                         nsAString& aStr) override;

  NS_IMETHOD AppendComment(nsIContent* aComment, int32_t aStartOffset,
                           int32_t aEndOffset, nsAString& aStr) override;
  
  NS_IMETHOD AppendDoctype(nsIContent *aDoctype,
                           nsAString& aStr) override;

  NS_IMETHOD AppendElementStart(mozilla::dom::Element* aElement,
                                mozilla::dom::Element* aOriginalElement,
                                nsAString& aStr) override;

  NS_IMETHOD AppendElementEnd(mozilla::dom::Element* aElement,
                              nsAString& aStr) override;

  NS_IMETHOD Flush(nsAString& aStr) override { return NS_OK; }

  NS_IMETHOD AppendDocumentStart(nsIDocument *aDocument,
                                 nsAString& aStr) override;

 protected:
  virtual ~nsXMLContentSerializer();

  


  MOZ_WARN_UNUSED_RESULT
  bool AppendToString(const char16_t aChar,
                      nsAString& aOutputStr);

  


  MOZ_WARN_UNUSED_RESULT
  bool AppendToString(const nsAString& aStr,
                      nsAString& aOutputStr);

  




  MOZ_WARN_UNUSED_RESULT
  bool AppendToStringConvertLF(const nsAString& aStr,
                               nsAString& aOutputStr);

  



  MOZ_WARN_UNUSED_RESULT
  bool AppendToStringWrapped(const nsASingleFragmentString& aStr,
                             nsAString& aOutputStr);

  



  MOZ_WARN_UNUSED_RESULT
  bool AppendToStringFormatedWrapped(const nsASingleFragmentString& aStr,
                                     nsAString& aOutputStr);

  
  MOZ_WARN_UNUSED_RESULT
  bool AppendWrapped_WhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          nsAString &aOutputStr);

  
  MOZ_WARN_UNUSED_RESULT
  bool AppendFormatedWrapped_WhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          bool &aMayIgnoreStartOfLineWhitespaceSequence,
          nsAString &aOutputStr);

  
  MOZ_WARN_UNUSED_RESULT
  bool AppendWrapped_NonWhitespaceSequence(
          nsASingleFragmentString::const_char_iterator &aPos,
          const nsASingleFragmentString::const_char_iterator aEnd,
          const nsASingleFragmentString::const_char_iterator aSequenceStart,
          bool &aMayIgnoreStartOfLineWhitespaceSequence,
          bool &aSequenceStartAfterAWhiteSpace,
          nsAString &aOutputStr);

  



  MOZ_WARN_UNUSED_RESULT
  bool AppendNewLineToString(nsAString& aOutputStr);


  



  MOZ_WARN_UNUSED_RESULT
  virtual bool AppendAndTranslateEntities(const nsAString& aStr,
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

  MOZ_WARN_UNUSED_RESULT
  virtual bool SerializeAttributes(nsIContent* aContent,
                                   nsIContent *aOriginalElement,
                                   nsAString& aTagPrefix,
                                   const nsAString& aTagNamespaceURI,
                                   nsIAtom* aTagName,
                                   nsAString& aStr,
                                   uint32_t aSkipAttr,
                                   bool aAddNSAttr);

  MOZ_WARN_UNUSED_RESULT
  bool SerializeAttr(const nsAString& aPrefix,
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
                                   nsAString& aStr,
                                   nsresult& aResult);

  



  MOZ_WARN_UNUSED_RESULT
  virtual bool AppendEndOfElementStart(nsIContent *aOriginalElement,
                                       nsIAtom * aName,
                                       int32_t aNamespaceID,
                                       nsAString& aStr);

  




  MOZ_WARN_UNUSED_RESULT
  virtual bool AfterElementStart(nsIContent* aContent,
                                 nsIContent* aOriginalElement,
                                 nsAString& aStr) { return true; };

  







  virtual bool CheckElementEnd(nsIContent * aContent,
                                 bool & aForceFormat,
                                 nsAString& aStr);

  




  virtual void AfterElementEnd(nsIContent * aContent,
                               nsAString& aStr) { };

  


  virtual bool LineBreakBeforeOpen(int32_t aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakAfterOpen(int32_t aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakBeforeClose(int32_t aNamespaceID, nsIAtom* aName);

  


  virtual bool LineBreakAfterClose(int32_t aNamespaceID, nsIAtom* aName);

  



  MOZ_WARN_UNUSED_RESULT
  bool AppendIndentation(nsAString& aStr);

  MOZ_WARN_UNUSED_RESULT
  bool IncrIndentation(nsIAtom* aName);
  void DecrIndentation(nsIAtom* aName);

  
  
  MOZ_WARN_UNUSED_RESULT
  bool MaybeAddNewlineForRootNode(nsAString& aStr);
  void MaybeFlagNewlineForRootNode(nsINode* aNode);

  
  virtual void MaybeEnterInPreContent(nsIContent* aNode);
  virtual void MaybeLeaveFromPreContent(nsIContent* aNode);

  bool ShouldMaintainPreLevel() const;
  int32_t PreLevel() const {
    MOZ_ASSERT(ShouldMaintainPreLevel());
    return mPreLevel;
  }
  int32_t& PreLevel() {
    MOZ_ASSERT(ShouldMaintainPreLevel());
    return mPreLevel;
  }

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

private:
  
  int32_t       mPreLevel;
};

nsresult
NS_NewXMLContentSerializer(nsIContentSerializer** aSerializer);

#endif 
