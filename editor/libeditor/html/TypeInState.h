




































#ifndef TypeInState_h__
#define TypeInState_h__

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsISelectionListener.h"
#include "nsEditProperty.h"
#include "nsString.h"
#include "nsVoidArray.h"

struct PropItem
{
  nsIAtom *tag;
  nsString attr;
  nsString value;
  
  PropItem() : tag(nsnull), attr(), value() {};
  PropItem(nsIAtom *aTag, const nsAString &aAttr, const nsAString &aValue);
  ~PropItem();
};

class TypeInState : public nsISelectionListener
{
public:

  NS_DECL_ISUPPORTS

  TypeInState();
  void Reset();
  virtual ~TypeInState();

  nsresult UpdateSelState(nsISelection *aSelection);

  
  NS_DECL_NSISELECTIONLISTENER

  nsresult SetProp(nsIAtom *aProp);
  nsresult SetProp(nsIAtom *aProp, const nsString &aAttr);
  nsresult SetProp(nsIAtom *aProp, const nsString &aAttr, const nsString &aValue);

  nsresult ClearAllProps();
  nsresult ClearProp(nsIAtom *aProp);
  nsresult ClearProp(nsIAtom *aProp, const nsString &aAttr);
  
  
  
  
  nsresult TakeClearProperty(PropItem **outPropItem);

  
  
  
  nsresult TakeSetProperty(PropItem **outPropItem);

  
  
  
  nsresult TakeRelativeFontSize(PRInt32 *outRelSize);

  nsresult GetTypingState(PRBool &isSet, PRBool &theSetting, nsIAtom *aProp);
  nsresult GetTypingState(PRBool &isSet, PRBool &theSetting, nsIAtom *aProp, 
                          const nsString &aAttr);
  nsresult GetTypingState(PRBool &isSet, PRBool &theSetting, nsIAtom *aProp, 
                          const nsString &aAttr, nsString* outValue);

  static   PRBool FindPropInList(nsIAtom *aProp, const nsAString &aAttr, nsAString *outValue, nsVoidArray &aList, PRInt32 &outIndex);

protected:

  nsresult RemovePropFromSetList(nsIAtom *aProp, const nsString &aAttr);
  nsresult RemovePropFromClearedList(nsIAtom *aProp, const nsString &aAttr);
  PRBool IsPropSet(nsIAtom *aProp, const nsString &aAttr, nsString* outValue);
  PRBool IsPropSet(nsIAtom *aProp, const nsString &aAttr, nsString* outValue, PRInt32 &outIndex);
  PRBool IsPropCleared(nsIAtom *aProp, const nsString &aAttr);
  PRBool IsPropCleared(nsIAtom *aProp, const nsString &aAttr, PRInt32 &outIndex);

  nsVoidArray mSetArray;
  nsVoidArray mClearedArray;
  PRInt32 mRelativeFontSize;
  nsCOMPtr<nsIDOMNode> mLastSelectionContainer;
  PRInt32 mLastSelectionOffset;
  
  friend class nsHTMLEditRules;
};



#endif  

