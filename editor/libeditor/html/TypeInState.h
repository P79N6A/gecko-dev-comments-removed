




































#ifndef TypeInState_h__
#define TypeInState_h__

#include "nsCOMPtr.h"
#include "nsIDOMNode.h"
#include "nsISelection.h"
#include "nsISelectionListener.h"
#include "nsEditProperty.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsCycleCollectionParticipant.h"

struct PropItem
{
  nsIAtom *tag;
  nsString attr;
  nsString value;
  
  PropItem();
  PropItem(nsIAtom *aTag, const nsAString &aAttr, const nsAString &aValue);
  ~PropItem();
};

class TypeInState : public nsISelectionListener
{
public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(TypeInState)

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

  nsresult GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp);
  nsresult GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp, 
                          const nsString &aAttr);
  nsresult GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp, 
                          const nsString &aAttr, nsString* outValue);

  static   bool FindPropInList(nsIAtom *aProp, const nsAString &aAttr, nsAString *outValue, nsTArray<PropItem*> &aList, PRInt32 &outIndex);

protected:

  nsresult RemovePropFromSetList(nsIAtom *aProp, const nsString &aAttr);
  nsresult RemovePropFromClearedList(nsIAtom *aProp, const nsString &aAttr);
  bool IsPropSet(nsIAtom *aProp, const nsString &aAttr, nsString* outValue);
  bool IsPropSet(nsIAtom *aProp, const nsString &aAttr, nsString* outValue, PRInt32 &outIndex);
  bool IsPropCleared(nsIAtom *aProp, const nsString &aAttr);
  bool IsPropCleared(nsIAtom *aProp, const nsString &aAttr, PRInt32 &outIndex);

  nsTArray<PropItem*> mSetArray;
  nsTArray<PropItem*> mClearedArray;
  PRInt32 mRelativeFontSize;
  nsCOMPtr<nsIDOMNode> mLastSelectionContainer;
  PRInt32 mLastSelectionOffset;
  
  friend class nsHTMLEditRules;
};



#endif  

