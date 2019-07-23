









































#ifndef nsIStyleSheet_h___
#define nsIStyleSheet_h___

#include <stdio.h>
#include "nsISupports.h"

class nsIAtom;
class nsString;
class nsIURI;
class nsIStyleRule;
class nsPresContext;
class nsIContent;
class nsIDocument;
class nsIStyleRuleProcessor;



#define NS_ISTYLE_SHEET_IID     \
{ 0x7b2d31da, 0xc3fb, 0x4537,   \
 { 0xbd, 0x97, 0x33, 0x72, 0x72, 0xb8, 0x35, 0x68 } }









class nsIStyleSheet : public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISTYLE_SHEET_IID)

  
  NS_IMETHOD GetSheetURI(nsIURI** aSheetURI) const = 0;
  NS_IMETHOD GetBaseURI(nsIURI** aBaseURI) const = 0;
  NS_IMETHOD GetTitle(nsString& aTitle) const = 0;
  NS_IMETHOD GetType(nsString& aType) const = 0;
  NS_IMETHOD_(PRBool) UseForMedium(nsPresContext* aPresContext) const = 0;
  NS_IMETHOD_(PRBool) HasRules() const = 0;

  






  NS_IMETHOD GetApplicable(PRBool& aApplicable) const = 0;

  








  NS_IMETHOD SetEnabled(PRBool aEnabled) = 0;

  


  NS_IMETHOD GetComplete(PRBool& aComplete) const = 0;
  NS_IMETHOD SetComplete() = 0;

  
  NS_IMETHOD GetParentSheet(nsIStyleSheet*& aParent) const = 0;  
  NS_IMETHOD GetOwningDocument(nsIDocument*& aDocument) const = 0; 
  NS_IMETHOD SetOwningDocument(nsIDocument* aDocument) = 0;

#ifdef DEBUG
  virtual void List(FILE* out = stdout, PRInt32 aIndent = 0) const = 0;
#endif
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIStyleSheet, NS_ISTYLE_SHEET_IID)

#endif 
