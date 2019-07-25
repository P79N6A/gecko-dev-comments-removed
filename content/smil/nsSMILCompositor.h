




































#ifndef NS_SMILCOMPOSITOR_H_
#define NS_SMILCOMPOSITOR_H_

#include "nsTHashtable.h"
#include "nsString.h"
#include "nsSMILAnimationFunction.h"
#include "nsSMILTargetIdentifier.h"
#include "nsSMILCompositorTable.h"
#include "pldhash.h"








class nsSMILCompositor : public PLDHashEntryHdr
{
public:
  typedef nsSMILTargetIdentifier KeyType;
  typedef const KeyType& KeyTypeRef;
  typedef const KeyType* KeyTypePointer;

  explicit nsSMILCompositor(KeyTypePointer aKey)
   : mKey(*aKey),
     mForceCompositing(false)
  { }
  nsSMILCompositor(const nsSMILCompositor& toCopy)
    : mKey(toCopy.mKey),
      mAnimationFunctions(toCopy.mAnimationFunctions),
      mForceCompositing(false)
  { }
  ~nsSMILCompositor() { }

  
  KeyTypeRef GetKey() const { return mKey; }
  bool KeyEquals(KeyTypePointer aKey) const;
  static KeyTypePointer KeyToPointer(KeyTypeRef aKey) { return &aKey; }
  static PLDHashNumber HashKey(KeyTypePointer aKey);
  enum { ALLOW_MEMMOVE = false };

  
  void AddAnimationFunction(nsSMILAnimationFunction* aFunc);

  
  
  
  void ComposeAttribute();

  
  void ClearAnimationEffects();

  
  void Traverse(nsCycleCollectionTraversalCallback* aCallback);

  
  
  void ToggleForceCompositing() { mForceCompositing = true; }

  
  void StealCachedBaseValue(nsSMILCompositor* aOther) {
    mCachedBaseValue = aOther->mCachedBaseValue;
  }

 private:
  
  
  nsISMILAttr* CreateSMILAttr();
  
  
  
  
  PRUint32 GetFirstFuncToAffectSandwich();

  
  
  void UpdateCachedBaseValue(const nsSMILValue& aBaseValue);

  
  PR_STATIC_CALLBACK(PLDHashOperator) DoComposeAttribute(
      nsSMILCompositor* aCompositor, void *aData);

  
  KeyType mKey;

  
  nsTArray<nsSMILAnimationFunction*> mAnimationFunctions;

  
  
  
  
  bool mForceCompositing;

  
  
  
  nsAutoPtr<nsSMILValue> mCachedBaseValue;
};

#endif 
