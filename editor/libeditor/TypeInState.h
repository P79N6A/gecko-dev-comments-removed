




#ifndef TypeInState_h__
#define TypeInState_h__

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISelectionListener.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nscore.h"

class nsIAtom;
class nsIDOMNode;
class nsISelection;

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

  nsresult UpdateSelState(nsISelection *aSelection);

  
  NS_DECL_NSISELECTIONLISTENER

  void SetProp(nsIAtom* aProp, const nsAString& aAttr, const nsAString& aValue);

  void ClearAllProps();
  void ClearProp(nsIAtom* aProp, const nsAString& aAttr);
  
  
  
  
  PropItem* TakeClearProperty();

  
  
  
  PropItem* TakeSetProperty();

  
  
  
  int32_t TakeRelativeFontSize();

  void GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp);
  void GetTypingState(bool &isSet, bool &theSetting, nsIAtom *aProp,
                      const nsString &aAttr, nsString* outValue);

  static   bool FindPropInList(nsIAtom *aProp, const nsAString &aAttr, nsAString *outValue, nsTArray<PropItem*> &aList, int32_t &outIndex);

protected:
  virtual ~TypeInState();

  void RemovePropFromSetList(nsIAtom* aProp, const nsAString& aAttr);
  void RemovePropFromClearedList(nsIAtom* aProp, const nsAString& aAttr);
  bool IsPropSet(nsIAtom* aProp, const nsAString& aAttr, nsAString* outValue);
  bool IsPropSet(nsIAtom* aProp, const nsAString& aAttr, nsAString* outValue, int32_t& outIndex);
  bool IsPropCleared(nsIAtom* aProp, const nsAString& aAttr);
  bool IsPropCleared(nsIAtom* aProp, const nsAString& aAttr, int32_t& outIndex);

  nsTArray<PropItem*> mSetArray;
  nsTArray<PropItem*> mClearedArray;
  int32_t mRelativeFontSize;
  nsCOMPtr<nsIDOMNode> mLastSelectionContainer;
  int32_t mLastSelectionOffset;
  
  friend class nsHTMLEditRules;
};



#endif  

