




































#ifndef NS_SMILCOMPOSITOR_H_
#define NS_SMILCOMPOSITOR_H_

#include "nsIContent.h"
#include "nsTHashtable.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsSMILAnimationFunction.h"
#include "nsSMILCompositorTable.h"
#include "pldhash.h"










struct nsSMILCompositorKey
{
  PRBool    Equals(const nsSMILCompositorKey &aOther) const;

  nsRefPtr<nsIContent> mElement;
  nsRefPtr<nsIAtom>    mAttributeName; 
  PRPackedBool         mIsCSS;
};








class nsSMILCompositor : public PLDHashEntryHdr
{
public:
  typedef const nsSMILCompositorKey& KeyType;
  typedef const nsSMILCompositorKey* KeyTypePointer;

  nsSMILCompositor(KeyTypePointer aKey) : mKey(*aKey) { }
  nsSMILCompositor(const nsSMILCompositor& toCopy)
    : mKey(toCopy.mKey),
      mAnimationFunctions(toCopy.mAnimationFunctions)
  { }
  ~nsSMILCompositor() { }

  
  KeyType GetKey() const { return mKey; }
  PRBool KeyEquals(KeyTypePointer aKey) const;
  static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey);
  enum { ALLOW_MEMMOVE = PR_FALSE };

  
  void AddAnimationFunction(nsSMILAnimationFunction* aFunc);
  
  
  
  
  void ComposeAttribute();

  
  void ClearAnimationEffects();

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);

 private:
  
  
  nsISMILAttr* CreateSMILAttr();

  
  PR_STATIC_CALLBACK(PLDHashOperator) DoComposeAttribute(
      nsSMILCompositor* aCompositor, void *aData);

  
  nsSMILCompositorKey  mKey;

  
  
  
  nsTArray<nsSMILAnimationFunction*> mAnimationFunctions;
};

#endif 
