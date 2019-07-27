







#ifndef mozilla_CSSStyleSheet_h
#define mozilla_CSSStyleSheet_h

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Element.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsCOMArray.h"
#include "nsTArrayForwardDeclare.h"
#include "nsString.h"
#include "mozilla/CORSMode.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/net/ReferrerPolicy.h"

class CSSRuleListImpl;
class nsCSSRuleProcessor;
class nsIPrincipal;
class nsIURI;
class nsMediaList;
class nsMediaQueryResultCacheKey;
class nsPresContext;
class nsXMLNameSpaceMap;

namespace mozilla {
struct ChildSheetListBuilder;
class CSSStyleSheet;

namespace css {
class Rule;
class GroupRule;
class ImportRule;
}
namespace dom {
class CSSRuleList;
}





class CSSStyleSheetInner
{
public:
  friend class mozilla::CSSStyleSheet;
  friend class ::nsCSSRuleProcessor;
  typedef net::ReferrerPolicy ReferrerPolicy;

private:
  CSSStyleSheetInner(CSSStyleSheet* aPrimarySheet,
                     CORSMode aCORSMode,
                     ReferrerPolicy aReferrerPolicy);
  CSSStyleSheetInner(CSSStyleSheetInner& aCopy,
                     CSSStyleSheet* aPrimarySheet);
  ~CSSStyleSheetInner();

  CSSStyleSheetInner* CloneFor(CSSStyleSheet* aPrimarySheet);
  void AddSheet(CSSStyleSheet* aSheet);
  void RemoveSheet(CSSStyleSheet* aSheet);

  void RebuildNameSpaces();

  
  nsresult CreateNamespaceMap();

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  nsAutoTArray<CSSStyleSheet*, 8> mSheets;
  nsCOMPtr<nsIURI>       mSheetURI; 
  nsCOMPtr<nsIURI>       mOriginalSheetURI;  
  nsCOMPtr<nsIURI>       mBaseURI; 
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMArray<css::Rule>  mOrderedRules;
  nsAutoPtr<nsXMLNameSpaceMap> mNameSpaceMap;
  
  
  
  
  
  nsRefPtr<CSSStyleSheet> mFirstChild;
  CORSMode               mCORSMode;
  
  
  ReferrerPolicy         mReferrerPolicy;
  bool                   mComplete;

#ifdef DEBUG
  bool                   mPrincipalSet;
#endif
};








#define NS_CSS_STYLE_SHEET_IMPL_CID     \
{ 0xca926f30, 0x2a7e, 0x477e, \
 { 0x84, 0x67, 0x80, 0x3f, 0xb3, 0x2a, 0xf2, 0x0a } }


class CSSStyleSheet final : public nsIStyleSheet,
                            public nsIDOMCSSStyleSheet,
                            public nsICSSLoaderObserver,
                            public nsWrapperCache
{
public:
  typedef net::ReferrerPolicy ReferrerPolicy;
  CSSStyleSheet(CORSMode aCORSMode, ReferrerPolicy aReferrerPolicy);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(CSSStyleSheet,
                                                         nsIStyleSheet)

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_STYLE_SHEET_IMPL_CID)

  
  virtual nsIURI* GetSheetURI() const override;
  virtual nsIURI* GetBaseURI() const override;
  virtual void GetTitle(nsString& aTitle) const override;
  virtual void GetType(nsString& aType) const override;
  virtual bool HasRules() const override;
  virtual bool IsApplicable() const override;
  virtual void SetEnabled(bool aEnabled) override;
  virtual bool IsComplete() const override;
  virtual void SetComplete() override;
  virtual nsIStyleSheet* GetParentSheet() const override;  
  virtual nsIDocument* GetOwningDocument() const override;  
  virtual void SetOwningDocument(nsIDocument* aDocument) override;

  
  uint64_t FindOwningWindowInnerID() const;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const override;
#endif

  void AppendStyleSheet(CSSStyleSheet* aSheet);
  void InsertStyleSheetAt(CSSStyleSheet* aSheet, int32_t aIndex);

  
  void PrependStyleRule(css::Rule* aRule);
  void AppendStyleRule(css::Rule* aRule);
  void ReplaceStyleRule(css::Rule* aOld, css::Rule* aNew);

  int32_t StyleRuleCount() const;
  css::Rule* GetStyleRuleAt(int32_t aIndex) const;

  nsresult DeleteRuleFromGroup(css::GroupRule* aGroup, uint32_t aIndex);
  nsresult InsertRuleIntoGroup(const nsAString& aRule, css::GroupRule* aGroup, uint32_t aIndex, uint32_t* _retval);
  nsresult ReplaceRuleInGroup(css::GroupRule* aGroup, css::Rule* aOld, css::Rule* aNew);

  int32_t StyleSheetCount() const;

  




  void SetURIs(nsIURI* aSheetURI, nsIURI* aOriginalSheetURI, nsIURI* aBaseURI);

  




  void SetPrincipal(nsIPrincipal* aPrincipal);

  
  nsIPrincipal* Principal() const { return mInner->mPrincipal; }

  
  nsIDocument* GetDocument() const { return mDocument; }

  void SetTitle(const nsAString& aTitle) { mTitle = aTitle; }
  void SetMedia(nsMediaList* aMedia);
  void SetOwningNode(nsINode* aOwningNode) { mOwningNode = aOwningNode;  }

  void SetOwnerRule(css::ImportRule* aOwnerRule) { mOwnerRule = aOwnerRule;  }
  css::ImportRule* GetOwnerRule() const { return mOwnerRule; }

  nsXMLNameSpaceMap* GetNameSpaceMap() const { return mInner->mNameSpaceMap; }

  already_AddRefed<CSSStyleSheet> Clone(CSSStyleSheet* aCloneParent,
                                        css::ImportRule* aCloneOwnerRule,
                                        nsIDocument* aCloneDocument,
                                        nsINode* aCloneOwningNode) const;

  bool IsModified() const { return mDirty; }

  void SetModifiedByChildRule() {
    NS_ASSERTION(mDirty,
                 "sheet must be marked dirty before handing out child rules");
    DidDirty();
  }

  nsresult AddRuleProcessor(nsCSSRuleProcessor* aProcessor);
  nsresult DropRuleProcessor(nsCSSRuleProcessor* aProcessor);

  


  nsresult InsertRuleInternal(const nsAString& aRule,
                              uint32_t aIndex, uint32_t* aReturn);

  

  virtual nsIURI* GetOriginalURI() const;

  
  NS_IMETHOD StyleSheetLoaded(CSSStyleSheet* aSheet, bool aWasAlternate,
                              nsresult aStatus) override;

  enum EnsureUniqueInnerResult {
    
    eUniqueInner_AlreadyUnique,
    
    
    eUniqueInner_ClonedInner
  };
  EnsureUniqueInnerResult EnsureUniqueInner();

  
  void AppendAllChildSheets(nsTArray<CSSStyleSheet*>& aArray);

  bool UseForPresentation(nsPresContext* aPresContext,
                            nsMediaQueryResultCacheKey& aKey) const;

  nsresult ParseSheet(const nsAString& aInput);

  
  NS_DECL_NSIDOMSTYLESHEET

  
  NS_DECL_NSIDOMCSSSTYLESHEET

  
  
  static bool RebuildChildList(css::Rule* aRule, void* aBuilder);

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const override;

  
  CORSMode GetCORSMode() const { return mInner->mCORSMode; }

  
  ReferrerPolicy GetReferrerPolicy() const { return mInner->mReferrerPolicy; }

  dom::Element* GetScopeElement() const { return mScopeElement; }
  void SetScopeElement(dom::Element* aScopeElement)
  {
    mScopeElement = aScopeElement;
  }

  
  
  
  void GetType(nsString& aType) {
    const_cast<const CSSStyleSheet*>(this)->GetType(aType);
  }
  
  nsINode* GetOwnerNode() const { return mOwningNode; }
  CSSStyleSheet* GetParentStyleSheet() const { return mParent; }
  
  
  void GetTitle(nsString& aTitle) {
    const_cast<const CSSStyleSheet*>(this)->GetTitle(aTitle);
  }
  nsMediaList* Media();
  bool Disabled() const { return mDisabled; }
  

  
  
  
  
  nsIDOMCSSRule* GetDOMOwnerRule() const;
  dom::CSSRuleList* GetCssRules(ErrorResult& aRv);
  uint32_t InsertRule(const nsAString& aRule, uint32_t aIndex,
                      ErrorResult& aRv) {
    uint32_t retval;
    aRv = InsertRule(aRule, aIndex, &retval);
    return retval;
  }
  void DeleteRule(uint32_t aIndex, ErrorResult& aRv) {
    aRv = DeleteRule(aIndex);
  }

  
  dom::ParentObject GetParentObject() const {
    if (mOwningNode) {
      return dom::ParentObject(mOwningNode);
    }

    return dom::ParentObject(static_cast<nsIStyleSheet*>(mParent), mParent);
  }
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  CSSStyleSheet(const CSSStyleSheet& aCopy,
                CSSStyleSheet* aParentToUse,
                css::ImportRule* aOwnerRuleToUse,
                nsIDocument* aDocumentToUse,
                nsINode* aOwningNodeToUse);

  CSSStyleSheet(const CSSStyleSheet& aCopy) = delete;
  CSSStyleSheet& operator=(const CSSStyleSheet& aCopy) = delete;

protected:
  virtual ~CSSStyleSheet();

  void ClearRuleCascades();

  void     WillDirty();
  void     DidDirty();

  
  
  
  
  nsresult SubjectSubsumesInnerPrincipal();

  
  nsresult RegisterNamespaceRule(css::Rule* aRule);

  
  void DropRuleCollection();

  
  void DropMedia();

  
  void UnlinkInner();
  
  void TraverseInner(nsCycleCollectionTraversalCallback &);

protected:
  nsString              mTitle;
  nsRefPtr<nsMediaList> mMedia;
  nsRefPtr<CSSStyleSheet> mNext;
  CSSStyleSheet*        mParent;    
  css::ImportRule*      mOwnerRule; 

  nsRefPtr<CSSRuleListImpl> mRuleCollection;
  nsIDocument*          mDocument; 
  nsINode*              mOwningNode; 
  bool                  mDisabled;
  bool                  mDirty; 
  nsRefPtr<dom::Element> mScopeElement;

  CSSStyleSheetInner*   mInner;

  nsAutoTArray<nsCSSRuleProcessor*, 8>* mRuleProcessors;

  friend class ::nsMediaList;
  friend class ::nsCSSRuleProcessor;
  friend struct mozilla::ChildSheetListBuilder;
};

NS_DEFINE_STATIC_IID_ACCESSOR(CSSStyleSheet, NS_CSS_STYLE_SHEET_IMPL_CID)

} 

#endif 
