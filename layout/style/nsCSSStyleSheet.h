










































#ifndef nsCSSStyleSheet_h_
#define nsCSSStyleSheet_h_

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsCOMArray.h"

class nsICSSRule;
class nsXMLNameSpaceMap;
class nsCSSRuleProcessor;
class nsMediaList;
class nsICSSGroupRule;
class nsICSSImportRule;
class nsIPrincipal;
class nsIURI;
class nsMediaList;
class nsMediaQueryResultCacheKey;
class nsCSSStyleSheet;
class nsPresContext;
template<class E> class nsTArray;





class nsCSSStyleSheetInner {
public:
  friend class nsCSSStyleSheet;
  friend class nsCSSRuleProcessor;
  friend nsresult NS_NewCSSStyleSheet(nsCSSStyleSheet** aInstancePtrResult);
private:
  nsCSSStyleSheetInner(nsCSSStyleSheet* aPrimarySheet);
  nsCSSStyleSheetInner(nsCSSStyleSheetInner& aCopy,
                       nsCSSStyleSheet* aPrimarySheet);
  ~nsCSSStyleSheetInner();

  nsCSSStyleSheetInner* CloneFor(nsCSSStyleSheet* aPrimarySheet);
  void AddSheet(nsCSSStyleSheet* aSheet);
  void RemoveSheet(nsCSSStyleSheet* aSheet);

  void RebuildNameSpaces();

  
  nsresult CreateNamespaceMap();

  nsAutoTArray<nsCSSStyleSheet*, 8> mSheets;
  nsCOMPtr<nsIURI>       mSheetURI; 
  nsCOMPtr<nsIURI>       mOriginalSheetURI;  
  nsCOMPtr<nsIURI>       mBaseURI; 
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMArray<nsICSSRule> mOrderedRules;
  nsAutoPtr<nsXMLNameSpaceMap> mNameSpaceMap;
  
  
  
  
  
  nsRefPtr<nsCSSStyleSheet> mFirstChild;
  PRBool                 mComplete;

#ifdef DEBUG
  PRBool                 mPrincipalSet;
#endif
};






class CSSRuleListImpl;
struct ChildSheetListBuilder;



#define NS_CSS_STYLE_SHEET_IMPL_CID     \
{ 0x55f243d9, 0xd985, 0x490c, \
 { 0x9e, 0xea, 0x09, 0x5c, 0x7f, 0xa3, 0x5c, 0xf4 } }


class nsCSSStyleSheet : public nsIStyleSheet,
                        public nsIDOMCSSStyleSheet,
                        public nsICSSLoaderObserver
{
public:
  nsCSSStyleSheet();

  NS_DECL_ISUPPORTS

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_STYLE_SHEET_IMPL_CID)

  
  NS_IMETHOD GetSheetURI(nsIURI** aSheetURI) const;
  NS_IMETHOD GetBaseURI(nsIURI** aBaseURI) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD_(PRBool) HasRules() const;
  NS_IMETHOD GetApplicable(PRBool& aApplicable) const;
  NS_IMETHOD SetEnabled(PRBool aEnabled);
  NS_IMETHOD GetComplete(PRBool& aComplete) const;
  NS_IMETHOD SetComplete();
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const;  
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const;  
  NS_IMETHOD SetOwningDocument(nsIDocument* aDocument);
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif
  
  void AppendStyleSheet(nsCSSStyleSheet* aSheet);
  void InsertStyleSheetAt(nsCSSStyleSheet* aSheet, PRInt32 aIndex);
  void PrependStyleRule(nsICSSRule* aRule);
  void AppendStyleRule(nsICSSRule* aRule);
  void ReplaceStyleRule(nsICSSRule* aOld, nsICSSRule* aNew);
  PRInt32 StyleRuleCount() const;
  nsresult GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const;
  nsresult DeleteRuleFromGroup(nsICSSGroupRule* aGroup, PRUint32 aIndex);
  nsresult InsertRuleIntoGroup(const nsAString& aRule, nsICSSGroupRule* aGroup, PRUint32 aIndex, PRUint32* _retval);
  nsresult ReplaceRuleInGroup(nsICSSGroupRule* aGroup, nsICSSRule* aOld, nsICSSRule* aNew);
  PRInt32 StyleSheetCount() const;
  already_AddRefed<nsCSSStyleSheet> GetStyleSheetAt(PRInt32 aIndex) const;
  void SetURIs(nsIURI* aSheetURI, nsIURI* aOriginalSheetURI, nsIURI* aBaseURI);
  void SetPrincipal(nsIPrincipal* aPrincipal);
  nsIPrincipal* Principal() const;
  void SetTitle(const nsAString& aTitle) { mTitle = aTitle; }
  void SetMedia(nsMediaList* aMedia);
  void SetOwningNode(nsIDOMNode* aOwningNode) { mOwningNode = aOwningNode;  }
  void SetOwnerRule(nsICSSImportRule* aOwnerRule) { mOwnerRule = aOwnerRule;  }
  already_AddRefed<nsICSSImportRule> GetOwnerRule();
  nsXMLNameSpaceMap* GetNameSpaceMap() const;
  already_AddRefed<nsCSSStyleSheet> Clone(nsCSSStyleSheet* aCloneParent,
                                          nsICSSImportRule* aCloneOwnerRule,
                                          nsIDocument* aCloneDocument,
                                          nsIDOMNode* aCloneOwningNode) const;
  PRBool IsModified() const { return mDirty; }
  void SetModified(PRBool aModified) { mDirty = aModified; }
  nsresult AddRuleProcessor(nsCSSRuleProcessor* aProcessor);
  nsresult DropRuleProcessor(nsCSSRuleProcessor* aProcessor);
  nsresult InsertRuleInternal(const nsAString& aRule,
                              PRUint32 aIndex, PRUint32* aReturn);
  virtual nsIURI* GetOriginalURI() const;

  
  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus);

  enum EnsureUniqueInnerResult {
    
    eUniqueInner_AlreadyUnique,
    
    
    eUniqueInner_ClonedInner,
    
    eUniqueInner_CloneFailed
  };
  EnsureUniqueInnerResult EnsureUniqueInner();

  
  
  PRBool AppendAllChildSheets(nsTArray<nsCSSStyleSheet*>& aArray);

  PRBool UseForPresentation(nsPresContext* aPresContext,
                            nsMediaQueryResultCacheKey& aKey) const;

  
  NS_DECL_NSIDOMSTYLESHEET

  
  NS_DECL_NSIDOMCSSSTYLESHEET

  
  
  static PRBool RebuildChildList(nsICSSRule* aRule, void* aBuilder);

private:
  nsCSSStyleSheet(const nsCSSStyleSheet& aCopy,
                  nsCSSStyleSheet* aParentToUse,
                  nsICSSImportRule* aOwnerRuleToUse,
                  nsIDocument* aDocumentToUse,
                  nsIDOMNode* aOwningNodeToUse);

  
  nsCSSStyleSheet(const nsCSSStyleSheet& aCopy); 
  nsCSSStyleSheet& operator=(const nsCSSStyleSheet& aCopy); 

protected:
  virtual ~nsCSSStyleSheet();

  void ClearRuleCascades();

  nsresult WillDirty();
  void     DidDirty();

  
  
  
  nsresult SubjectSubsumesInnerPrincipal() const;

  
  nsresult RegisterNamespaceRule(nsICSSRule* aRule);

protected:
  nsString              mTitle;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<nsCSSStyleSheet> mNext;
  nsCSSStyleSheet*      mParent;    
  nsICSSImportRule*     mOwnerRule; 

  CSSRuleListImpl*      mRuleCollection;
  nsIDocument*          mDocument; 
  nsIDOMNode*           mOwningNode; 
  PRPackedBool          mDisabled;
  PRPackedBool          mDirty; 

  nsCSSStyleSheetInner* mInner;

  nsAutoTArray<nsCSSRuleProcessor*, 8>* mRuleProcessors;

  friend class nsMediaList;
  friend class nsCSSRuleProcessor;
  friend nsresult NS_NewCSSStyleSheet(nsCSSStyleSheet** aInstancePtrResult);
  friend struct ChildSheetListBuilder;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCSSStyleSheet, NS_CSS_STYLE_SHEET_IMPL_CID)

nsresult NS_NewCSSStyleSheet(nsCSSStyleSheet** aInstancePtrResult);

#endif 
