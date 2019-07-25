






































#ifndef nsICSSStyleSheet_h___
#define nsICSSStyleSheet_h___

#include "nsIStyleSheet.h"
#include "nsCOMPtr.h"

class nsICSSRule;
class nsIDOMNode;
class nsXMLNameSpaceMap;
class nsCSSRuleProcessor;
class nsMediaList;
class nsICSSGroupRule;
class nsICSSImportRule;
class nsIPrincipal;
class nsAString;



#define NS_ICSS_STYLE_SHEET_IID     \
{ 0x94d4d747, 0xf690, 0x4eb6, \
 { 0x96, 0xc0, 0x19, 0x6a, 0x1b, 0x36, 0x59, 0xdc } }

class nsICSSStyleSheet : public nsIStyleSheet {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_STYLE_SHEET_IID)

  virtual void AppendStyleSheet(nsICSSStyleSheet* aSheet) = 0;
  virtual void InsertStyleSheetAt(nsICSSStyleSheet* aSheet, PRInt32 aIndex) = 0;

  
  virtual void PrependStyleRule(nsICSSRule* aRule) = 0;
  virtual void AppendStyleRule(nsICSSRule* aRule) = 0;
  virtual void ReplaceStyleRule(nsICSSRule* aOld, nsICSSRule* aNew) = 0;

  virtual PRInt32 StyleRuleCount() const = 0;
  virtual nsresult GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const = 0;

  virtual nsresult DeleteRuleFromGroup(nsICSSGroupRule* aGroup, PRUint32 aIndex) = 0;
  virtual nsresult InsertRuleIntoGroup(const nsAString & aRule, nsICSSGroupRule* aGroup,
                                       PRUint32 aIndex, PRUint32* _retval) = 0;
  virtual nsresult ReplaceRuleInGroup(nsICSSGroupRule* aGroup, nsICSSRule* aOld,
                                      nsICSSRule* aNew) = 0;

  virtual PRInt32 StyleSheetCount() const = 0;
  virtual already_AddRefed<nsICSSStyleSheet> GetStyleSheetAt(PRInt32 aIndex) const = 0;

  




  virtual void SetURIs(nsIURI* aSheetURI, nsIURI* aOriginalSheetURI,
                       nsIURI* aBaseURI) = 0;

  




  virtual void SetPrincipal(nsIPrincipal* aPrincipal) = 0;

  
  virtual nsIPrincipal* Principal() const = 0;

  virtual void SetTitle(const nsAString& aTitle) = 0;
  virtual void SetMedia(nsMediaList* aMedia) = 0;
  virtual void SetOwningNode(nsIDOMNode* aOwningNode) = 0;

  virtual void SetOwnerRule(nsICSSImportRule* aOwnerRule) = 0;
  virtual already_AddRefed<nsICSSImportRule> GetOwnerRule() = 0;
  
  
  virtual nsXMLNameSpaceMap* GetNameSpaceMap() const = 0;

  virtual already_AddRefed<nsICSSStyleSheet> Clone(nsICSSStyleSheet* aCloneParent,
                                                   nsICSSImportRule* aCloneOwnerRule,
                                                   nsIDocument* aCloneDocument,
                                                   nsIDOMNode* aCloneOwningNode) const = 0;

  virtual PRBool IsModified() const = 0; 
  virtual void SetModified(PRBool aModified) = 0;

  virtual nsresult AddRuleProcessor(nsCSSRuleProcessor* aProcessor) = 0;
  virtual nsresult DropRuleProcessor(nsCSSRuleProcessor* aProcessor) = 0;

  


  virtual nsresult InsertRuleInternal(const nsAString& aRule,
                                PRUint32 aIndex, PRUint32* aReturn) = 0;

  

  virtual nsIURI* GetOriginalURI() const = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSStyleSheet, NS_ICSS_STYLE_SHEET_IID)

nsresult
NS_NewCSSStyleSheet(nsICSSStyleSheet** aInstancePtrResult);

#endif 
