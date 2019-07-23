










































#ifndef nsCSSStyleSheet_h_
#define nsCSSStyleSheet_h_

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsICSSStyleSheet.h"
#include "nsIDOMCSSStyleSheet.h"
#include "nsICSSLoaderObserver.h"
#include "nsVoidArray.h"
#include "nsCOMArray.h"

class nsIURI;
class nsMediaList;





class nsCSSStyleSheetInner {
public:
  nsCSSStyleSheetInner(nsICSSStyleSheet* aParentSheet);
  nsCSSStyleSheetInner(nsCSSStyleSheetInner& aCopy, nsICSSStyleSheet* aParentSheet);
  virtual ~nsCSSStyleSheetInner();

  virtual nsCSSStyleSheetInner* CloneFor(nsICSSStyleSheet* aParentSheet);
  virtual void AddSheet(nsICSSStyleSheet* aParentSheet);
  virtual void RemoveSheet(nsICSSStyleSheet* aParentSheet);

  virtual void RebuildNameSpaces();

  nsAutoVoidArray        mSheets;
  nsCOMPtr<nsIURI>       mSheetURI; 
  nsCOMPtr<nsIURI>       mBaseURI; 
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMArray<nsICSSRule> mOrderedRules;
  nsAutoPtr<nsXMLNameSpaceMap> mNameSpaceMap;
  PRBool                 mComplete;

#ifdef DEBUG
  PRBool                 mPrincipalSet;
#endif
};






class CSSImportsCollectionImpl;
class CSSRuleListImpl;
static PRBool CascadeSheetRulesInto(nsICSSStyleSheet* aSheet, void* aData);

class nsCSSStyleSheet : public nsICSSStyleSheet, 
                        public nsIDOMCSSStyleSheet,
                        public nsICSSLoaderObserver
{
public:
  nsCSSStyleSheet();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD GetSheetURI(nsIURI** aSheetURI) const;
  NS_IMETHOD GetBaseURI(nsIURI** aBaseURI) const;
  NS_IMETHOD GetTitle(nsString& aTitle) const;
  NS_IMETHOD GetType(nsString& aType) const;
  NS_IMETHOD_(PRBool) UseForMedium(nsPresContext* aPresContext) const;
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
  
  
  NS_IMETHOD ContainsStyleSheet(nsIURI* aURL, PRBool& aContains,
                                nsIStyleSheet** aTheChild=nsnull);
  NS_IMETHOD AppendStyleSheet(nsICSSStyleSheet* aSheet);
  NS_IMETHOD InsertStyleSheetAt(nsICSSStyleSheet* aSheet, PRInt32 aIndex);
  NS_IMETHOD PrependStyleRule(nsICSSRule* aRule);
  NS_IMETHOD AppendStyleRule(nsICSSRule* aRule);
  NS_IMETHOD ReplaceStyleRule(nsICSSRule* aOld, nsICSSRule* aNew);
  NS_IMETHOD StyleRuleCount(PRInt32& aCount) const;
  NS_IMETHOD GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const;
  NS_IMETHOD DeleteRuleFromGroup(nsICSSGroupRule* aGroup, PRUint32 aIndex);
  NS_IMETHOD InsertRuleIntoGroup(const nsAString& aRule, nsICSSGroupRule* aGroup, PRUint32 aIndex, PRUint32* _retval);
  NS_IMETHOD ReplaceRuleInGroup(nsICSSGroupRule* aGroup, nsICSSRule* aOld, nsICSSRule* aNew);
  NS_IMETHOD StyleSheetCount(PRInt32& aCount) const;
  NS_IMETHOD GetStyleSheetAt(PRInt32 aIndex, nsICSSStyleSheet*& aSheet) const;
  NS_IMETHOD SetURIs(nsIURI* aSheetURI, nsIURI* aBaseURI);
  virtual NS_HIDDEN_(void) SetPrincipal(nsIPrincipal* aPrincipal);
  virtual NS_HIDDEN_(nsIPrincipal*) Principal() const;
  NS_IMETHOD SetTitle(const nsAString& aTitle);
  NS_IMETHOD SetMedia(nsMediaList* aMedia);
  NS_IMETHOD SetOwningNode(nsIDOMNode* aOwningNode);
  NS_IMETHOD SetOwnerRule(nsICSSImportRule* aOwnerRule);
  NS_IMETHOD GetOwnerRule(nsICSSImportRule** aOwnerRule);
  virtual NS_HIDDEN_(nsXMLNameSpaceMap*) GetNameSpaceMap() const;
  NS_IMETHOD Clone(nsICSSStyleSheet* aCloneParent,
                   nsICSSImportRule* aCloneOwnerRule,
                   nsIDocument* aCloneDocument,
                   nsIDOMNode* aCloneOwningNode,
                   nsICSSStyleSheet** aClone) const;
  NS_IMETHOD IsModified(PRBool* aSheetModified) const;
  NS_IMETHOD SetModified(PRBool aModified);
  NS_IMETHOD AddRuleProcessor(nsCSSRuleProcessor* aProcessor);
  NS_IMETHOD DropRuleProcessor(nsCSSRuleProcessor* aProcessor);

  
  NS_IMETHOD StyleSheetLoaded(nsICSSStyleSheet* aSheet, PRBool aWasAlternate,
                              nsresult aStatus);
  
  nsresult EnsureUniqueInner();

  
  NS_DECL_NSIDOMSTYLESHEET

  
  NS_DECL_NSIDOMCSSSTYLESHEET

private:
  nsCSSStyleSheet(const nsCSSStyleSheet& aCopy,
                  nsICSSStyleSheet* aParentToUse,
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

protected:
  nsString              mTitle;
  nsCOMPtr<nsMediaList> mMedia;
  nsCSSStyleSheet*      mFirstChild;
  nsCSSStyleSheet*      mNext;
  nsICSSStyleSheet*     mParent;    
  nsICSSImportRule*     mOwnerRule; 

  CSSImportsCollectionImpl* mImportsCollection;
  CSSRuleListImpl*      mRuleCollection;
  nsIDocument*          mDocument; 
  nsIDOMNode*           mOwningNode; 
  PRPackedBool          mDisabled;
  PRPackedBool          mDirty; 

  nsCSSStyleSheetInner* mInner;

  nsAutoVoidArray*      mRuleProcessors;

  friend class nsMediaList;
  friend PRBool CascadeSheetRulesInto(nsICSSStyleSheet* aSheet, void* aData);
  friend nsresult NS_NewCSSStyleSheet(nsICSSStyleSheet** aInstancePtrResult);
};

#endif 
