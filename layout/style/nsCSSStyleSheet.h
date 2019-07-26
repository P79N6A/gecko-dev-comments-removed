







#ifndef nsCSSStyleSheet_h_
#define nsCSSStyleSheet_h_

#include "mozilla/Attributes.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsString.h"
#include "mozilla/CORSMode.h"

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
private:
  nsCSSStyleSheetInner(nsCSSStyleSheet* aPrimarySheet,
                       mozilla::CORSMode aCORSMode);
  nsCSSStyleSheetInner(nsCSSStyleSheetInner& aCopy,
                       nsCSSStyleSheet* aPrimarySheet);
  ~nsCSSStyleSheetInner();

  nsCSSStyleSheetInner* CloneFor(nsCSSStyleSheet* aPrimarySheet);
  void AddSheet(nsCSSStyleSheet* aSheet);
  void RemoveSheet(nsCSSStyleSheet* aSheet);

  void RebuildNameSpaces();

  
  nsresult CreateNamespaceMap();

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  nsAutoTArray<nsCSSStyleSheet*, 8> mSheets;
  nsCOMPtr<nsIURI>       mSheetURI; 
  nsCOMPtr<nsIURI>       mOriginalSheetURI;  
  nsCOMPtr<nsIURI>       mBaseURI; 
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMArray<mozilla::css::Rule> mOrderedRules;
  nsAutoPtr<nsXMLNameSpaceMap> mNameSpaceMap;
  
  
  
  
  
  nsRefPtr<nsCSSStyleSheet> mFirstChild;
  mozilla::CORSMode      mCORSMode;
  bool                   mComplete;

#ifdef DEBUG
  bool                   mPrincipalSet;
#endif
};






class CSSRuleListImpl;
struct ChildSheetListBuilder;



#define NS_CSS_STYLE_SHEET_IMPL_CID     \
{ 0xca926f30, 0x2a7e, 0x477e, \
 { 0x84, 0x67, 0x80, 0x3f, 0xb3, 0x2a, 0xf2, 0x0a } }


class nsCSSStyleSheet MOZ_FINAL : public nsIStyleSheet,
                                  public nsIDOMCSSStyleSheet,
                                  public nsICSSLoaderObserver
{
public:
  nsCSSStyleSheet(mozilla::CORSMode aCORSMode);

  NS_DECL_ISUPPORTS

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CSS_STYLE_SHEET_IMPL_CID)

  
  virtual nsIURI* GetSheetURI() const;
  virtual nsIURI* GetBaseURI() const MOZ_OVERRIDE;
  virtual void GetTitle(nsString& aTitle) const MOZ_OVERRIDE;
  virtual void GetType(nsString& aType) const MOZ_OVERRIDE;
  virtual bool HasRules() const MOZ_OVERRIDE;
  virtual bool IsApplicable() const MOZ_OVERRIDE;
  virtual void SetEnabled(bool aEnabled) MOZ_OVERRIDE;
  virtual bool IsComplete() const MOZ_OVERRIDE;
  virtual void SetComplete() MOZ_OVERRIDE;
  virtual nsIStyleSheet* GetParentSheet() const MOZ_OVERRIDE;  
  virtual nsIDocument* GetOwningDocument() const MOZ_OVERRIDE;  
  virtual void SetOwningDocument(nsIDocument* aDocument) MOZ_OVERRIDE;

  
  uint64_t FindOwningWindowInnerID() const;
#ifdef DEBUG
  virtual void List(FILE* out = stdout, int32_t aIndent = 0) const MOZ_OVERRIDE;
#endif

  void AppendStyleSheet(nsCSSStyleSheet* aSheet);
  void InsertStyleSheetAt(nsCSSStyleSheet* aSheet, int32_t aIndex);

  
  void PrependStyleRule(mozilla::css::Rule* aRule);
  void AppendStyleRule(mozilla::css::Rule* aRule);
  void ReplaceStyleRule(mozilla::css::Rule* aOld, mozilla::css::Rule* aNew);

  int32_t StyleRuleCount() const;
  nsresult GetStyleRuleAt(int32_t aIndex, mozilla::css::Rule*& aRule) const;

  nsresult DeleteRuleFromGroup(mozilla::css::GroupRule* aGroup, uint32_t aIndex);
  nsresult InsertRuleIntoGroup(const nsAString& aRule, mozilla::css::GroupRule* aGroup, uint32_t aIndex, uint32_t* _retval);
  nsresult ReplaceRuleInGroup(mozilla::css::GroupRule* aGroup, mozilla::css::Rule* aOld, mozilla::css::Rule* aNew);

  int32_t StyleSheetCount() const;

  




  void SetURIs(nsIURI* aSheetURI, nsIURI* aOriginalSheetURI, nsIURI* aBaseURI);

  




  void SetPrincipal(nsIPrincipal* aPrincipal);

  
  nsIPrincipal* Principal() const { return mInner->mPrincipal; }

  
  nsIDocument* GetDocument() const { return mDocument; }

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

  
  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet* aSheet, bool aWasAlternate,
                              nsresult aStatus);

  enum EnsureUniqueInnerResult {
    
    eUniqueInner_AlreadyUnique,
    
    
    eUniqueInner_ClonedInner,
    
    eUniqueInner_CloneFailed
  };
  EnsureUniqueInnerResult EnsureUniqueInner();

  
  
  bool AppendAllChildSheets(nsTArray<nsCSSStyleSheet*>& aArray);

  bool UseForPresentation(nsPresContext* aPresContext,
                            nsMediaQueryResultCacheKey& aKey) const;

  nsresult ParseSheet(const nsAString& aInput);

  
  NS_DECL_NSIDOMSTYLESHEET

  
  NS_DECL_NSIDOMCSSSTYLESHEET

  
  
  static bool RebuildChildList(mozilla::css::Rule* aRule, void* aBuilder);

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

  
  mozilla::CORSMode GetCORSMode() const { return mInner->mCORSMode; }

private:
  nsCSSStyleSheet(const nsCSSStyleSheet& aCopy,
                  nsCSSStyleSheet* aParentToUse,
                  mozilla::css::ImportRule* aOwnerRuleToUse,
                  nsIDocument* aDocumentToUse,
                  nsIDOMNode* aOwningNodeToUse);

  nsCSSStyleSheet(const nsCSSStyleSheet& aCopy) MOZ_DELETE;
  nsCSSStyleSheet& operator=(const nsCSSStyleSheet& aCopy) MOZ_DELETE;

protected:
  virtual ~nsCSSStyleSheet();

  void ClearRuleCascades();

  nsresult WillDirty();
  void     DidDirty();

  
  
  
  
  nsresult SubjectSubsumesInnerPrincipal();

  
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
  bool                  mDisabled;
  bool                  mDirty; 

  nsCSSStyleSheetInner* mInner;

  nsAutoTArray<nsCSSRuleProcessor*, 8>* mRuleProcessors;

  friend class nsMediaList;
  friend class nsCSSRuleProcessor;
  friend struct ChildSheetListBuilder;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCSSStyleSheet, NS_CSS_STYLE_SHEET_IMPL_CID)

#endif 
