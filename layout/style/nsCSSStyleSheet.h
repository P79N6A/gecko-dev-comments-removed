










































#ifndef nsCSSStyleSheet_h_
#define nsCSSStyleSheet_h_

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsCOMArray.h"

class nsXMLNameSpaceMap;
class nsCSSRuleProcessor;
class nsMediaList;
class nsIPrincipal;
class nsIURI;
class nsMediaList;
class nsMediaQueryResultCacheKey;
class nsCSSStyleSheet;
class nsPresContext;
template<class E, class A> class nsTArray;

namespace mozilla {
namespace css {
class Rule;
class GroupRule;
class ImportRule;
}
}





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
  nsCOMArray<mozilla::css::Rule> mOrderedRules;
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
{ 0xca926f30, 0x2a7e, 0x477e, \
 { 0x84, 0x67, 0x80, 0x3f, 0xb3, 0x2a, 0xf2, 0x0a } }


class NS_FINAL_CLASS nsCSSStyleSheet : public nsIStyleSheet,
                                       public nsIDOMCSSStyleSheet,
                                       public nsICSSLoaderObserver
{
public:
  nsCSSStyleSheet();

  NS_DECL_ISUPPORTS

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_STYLE_SHEET_IMPL_CID)

  
  virtual nsIURI* GetSheetURI() const;
  virtual nsIURI* GetBaseURI() const;
  virtual void GetTitle(nsString& aTitle) const;
  virtual void GetType(nsString& aType) const;
  virtual PRBool HasRules() const;
  virtual PRBool IsApplicable() const;
  virtual void SetEnabled(PRBool aEnabled);
  virtual PRBool IsComplete() const;
  virtual void SetComplete();
  virtual nsIStyleSheet* GetParentSheet() const;  
  virtual nsIDocument* GetOwningDocument() const;  
  virtual void SetOwningDocument(nsIDocument* aDocument);

  
  virtual PRUint64 FindOwningWindowID() const;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const;
#endif

  void AppendStyleSheet(nsCSSStyleSheet* aSheet);
  void InsertStyleSheetAt(nsCSSStyleSheet* aSheet, PRInt32 aIndex);

  
  void PrependStyleRule(mozilla::css::Rule* aRule);
  void AppendStyleRule(mozilla::css::Rule* aRule);
  void ReplaceStyleRule(mozilla::css::Rule* aOld, mozilla::css::Rule* aNew);

  PRInt32 StyleRuleCount() const;
  nsresult GetStyleRuleAt(PRInt32 aIndex, mozilla::css::Rule*& aRule) const;

  nsresult DeleteRuleFromGroup(mozilla::css::GroupRule* aGroup, PRUint32 aIndex);
  nsresult InsertRuleIntoGroup(const nsAString& aRule, mozilla::css::GroupRule* aGroup, PRUint32 aIndex, PRUint32* _retval);
  nsresult ReplaceRuleInGroup(mozilla::css::GroupRule* aGroup, mozilla::css::Rule* aOld, mozilla::css::Rule* aNew);

  PRInt32 StyleSheetCount() const;

  




  void SetURIs(nsIURI* aSheetURI, nsIURI* aOriginalSheetURI, nsIURI* aBaseURI);

  




  void SetPrincipal(nsIPrincipal* aPrincipal);

  
  nsIPrincipal* Principal() const { return mInner->mPrincipal; }

  void SetTitle(const nsAString& aTitle) { mTitle = aTitle; }
  void SetMedia(nsMediaList* aMedia);
  void SetOwningNode(nsIDOMNode* aOwningNode) { mOwningNode = aOwningNode;  }

  void SetOwnerRule(mozilla::css::ImportRule* aOwnerRule) { mOwnerRule = aOwnerRule;  }
  mozilla::css::ImportRule* GetOwnerRule() const { return mOwnerRule; }

  nsXMLNameSpaceMap* GetNameSpaceMap() const { return mInner->mNameSpaceMap; }

  already_AddRefed<nsCSSStyleSheet> Clone(nsCSSStyleSheet* aCloneParent,
                                          mozilla::css::ImportRule* aCloneOwnerRule,
                                          nsIDocument* aCloneDocument,
                                          nsIDOMNode* aCloneOwningNode) const;

  PRBool IsModified() const { return mDirty; }

  void SetModifiedByChildRule() {
    NS_ASSERTION(mDirty,
                 "sheet must be marked dirty before handing out child rules");
    DidDirty();
  }

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

  
  
  static PRBool RebuildChildList(mozilla::css::Rule* aRule, void* aBuilder);

private:
  nsCSSStyleSheet(const nsCSSStyleSheet& aCopy,
                  nsCSSStyleSheet* aParentToUse,
                  mozilla::css::ImportRule* aOwnerRuleToUse,
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

  
  nsresult RegisterNamespaceRule(mozilla::css::Rule* aRule);

protected:
  nsString              mTitle;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<nsCSSStyleSheet> mNext;
  nsCSSStyleSheet*      mParent;    
  mozilla::css::ImportRule* mOwnerRule; 

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
