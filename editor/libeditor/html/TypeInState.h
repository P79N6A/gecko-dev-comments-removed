




































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

  void SetProp(nsIAtom* aProp, const nsAString& aAttr, const nsAString& aValue);

  void ClearAllProps();
  void ClearProp(nsIAtom* aProp, const nsAString& aAttr);
  
  
  
  
  PropItem* TakeClearProperty();

  
  
  
  PropItem* TakeSetProperty();

  
  
  
  PRInt32 TakeRelativeFontSize();

  nsresult GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp);
  nsresult GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp, 
                          const nsString &aAttr, nsString* outValue);

  static   bool FindPropInList(nsIAtom *aProp, const nsAString &aAttr, nsAString *outValue, nsTArray<PropItem*> &aList, PRInt32 &outIndex);

protected:

  nsresult RemovePropFromSetList(nsIAtom* aProp, const nsAString& aAttr);
  nsresult RemovePropFromClearedList(nsIAtom* aProp, const nsAString& aAttr);
  bool IsPropSet(nsIAtom* aProp, const nsAString& aAttr, nsAString* outValue);
  bool IsPropSet(nsIAtom* aProp, const nsAString& aAttr, nsAString* outValue, PRInt32& outIndex);
  bool IsPropCleared(nsIAtom* aProp, const nsAString& aAttr);
  bool IsPropCleared(nsIAtom* aProp, const nsAString& aAttr, PRInt32& outIndex);

  nsTArray<PropItem*> mSetArray;
  nsTArray<PropItem*> mClearedArray;
  PRInt32 mRelativeFontSize;
  nsCOMPtr<nsIDOMNode> mLastSelectionContainer;
  PRInt32 mLastSelectionOffset;
  
  friend class nsHTMLEditRules;
};



#endif  

