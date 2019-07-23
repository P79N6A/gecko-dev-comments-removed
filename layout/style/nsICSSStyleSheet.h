






































#ifndef nsICSSStyleSheet_h___
#define nsICSSStyleSheet_h___

#include "nsIStyleSheet.h"
#include "nsString.h"

class nsICSSRule;
class nsIDOMNode;
class nsXMLNameSpaceMap;
class nsCSSRuleProcessor;
class nsMediaList;
class nsICSSGroupRule;
class nsICSSImportRule;
class nsIPrincipal;



#define NS_ICSS_STYLE_SHEET_IID     \
{ 0x36541c18, 0xe735, 0x48ef, \
 { 0x86, 0x22, 0x3a, 0x48, 0x12, 0x75, 0xb7, 0x57 } }

class nsICSSStyleSheet : public nsIStyleSheet {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_STYLE_SHEET_IID)

  NS_IMETHOD  ContainsStyleSheet(nsIURI* aURL, PRBool& aContains, nsIStyleSheet** aTheChild=nsnull) = 0;

  NS_IMETHOD  AppendStyleSheet(nsICSSStyleSheet* aSheet) = 0;
  NS_IMETHOD  InsertStyleSheetAt(nsICSSStyleSheet* aSheet, PRInt32 aIndex) = 0;

  
  NS_IMETHOD  PrependStyleRule(nsICSSRule* aRule) = 0;
  NS_IMETHOD  AppendStyleRule(nsICSSRule* aRule) = 0;
  NS_IMETHOD  ReplaceStyleRule(nsICSSRule* aOld, nsICSSRule* aNew) = 0;

  NS_IMETHOD  StyleRuleCount(PRInt32& aCount) const = 0;
  NS_IMETHOD  GetStyleRuleAt(PRInt32 aIndex, nsICSSRule*& aRule) const = 0;

  NS_IMETHOD  DeleteRuleFromGroup(nsICSSGroupRule* aGroup, PRUint32 aIndex) = 0;
  NS_IMETHOD  InsertRuleIntoGroup(const nsAString & aRule, nsICSSGroupRule* aGroup, PRUint32 aIndex, PRUint32* _retval) = 0;
  NS_IMETHOD  ReplaceRuleInGroup(nsICSSGroupRule* aGroup, nsICSSRule* aOld, nsICSSRule* aNew) = 0;

  NS_IMETHOD  StyleSheetCount(PRInt32& aCount) const = 0;
  NS_IMETHOD  GetStyleSheetAt(PRInt32 aIndex, nsICSSStyleSheet*& aSheet) const = 0;

  




  NS_IMETHOD  SetURIs(nsIURI* aSheetURI, nsIURI* aBaseURI) = 0;

  




  virtual NS_HIDDEN_(void) SetPrincipal(nsIPrincipal* aPrincipal) = 0;

  
  virtual NS_HIDDEN_(nsIPrincipal*) Principal() const = 0;
  
  NS_IMETHOD  SetTitle(const nsAString& aTitle) = 0;
  NS_IMETHOD  SetMedia(nsMediaList* aMedia) = 0;
  NS_IMETHOD  SetOwningNode(nsIDOMNode* aOwningNode) = 0;

  NS_IMETHOD  SetOwnerRule(nsICSSImportRule* aOwnerRule) = 0;
  NS_IMETHOD  GetOwnerRule(nsICSSImportRule** aOwnerRule) = 0;
  
  
  virtual NS_HIDDEN_(nsXMLNameSpaceMap*) GetNameSpaceMap() const = 0;

  NS_IMETHOD  Clone(nsICSSStyleSheet* aCloneParent,
                    nsICSSImportRule* aCloneOwnerRule,
                    nsIDocument* aCloneDocument,
                    nsIDOMNode* aCloneOwningNode,
                    nsICSSStyleSheet** aClone) const = 0;

  NS_IMETHOD  IsModified(PRBool* aModified) const = 0; 
  NS_IMETHOD  SetModified(PRBool aModified) = 0;

  NS_IMETHOD  AddRuleProcessor(nsCSSRuleProcessor* aProcessor) = 0;
  NS_IMETHOD  DropRuleProcessor(nsCSSRuleProcessor* aProcessor) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSStyleSheet, NS_ICSS_STYLE_SHEET_IID)

nsresult
NS_NewCSSStyleSheet(nsICSSStyleSheet** aInstancePtrResult);

#endif 
