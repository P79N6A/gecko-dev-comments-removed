




#include "nsSMILCompositor.h"
#include "nsSMILCSSProperty.h"
#include "nsCSSProps.h"
#include "nsHashKeys.h"


bool
nsSMILCompositor::KeyEquals(KeyTypePointer aKey) const
{
  return aKey && aKey->Equals(mKey);
}

 PLDHashNumber
nsSMILCompositor::HashKey(KeyTypePointer aKey)
{
  
  
  
  
  return (NS_PTR_TO_UINT32(aKey->mElement.get()) >> 2) +
    NS_PTR_TO_UINT32(aKey->mAttributeName.get()) +
    (aKey->mIsCSS ? 1 : 0);
}


void
nsSMILCompositor::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  if (!mKey.mElement)
    return;

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback, "Compositor mKey.mElement");
  aCallback->NoteXPCOMChild(mKey.mElement);
}


void
nsSMILCompositor::AddAnimationFunction(nsSMILAnimationFunction* aFunc)
{
  if (aFunc) {
    mAnimationFunctions.AppendElement(aFunc);
  }
}

void
nsSMILCompositor::ComposeAttribute()
{
  if (!mKey.mElement)
    return;

  
  
  nsAutoPtr<nsISMILAttr> smilAttr(CreateSMILAttr());
  if (!smilAttr) {
    
    return;
  }
  if (mAnimationFunctions.IsEmpty()) {
    
    
    smilAttr->ClearAnimValue();
    return;
  }

  
  nsSMILAnimationFunction::Comparator comparator;
  mAnimationFunctions.Sort(comparator);

  
  
  uint32_t firstFuncToCompose = GetFirstFuncToAffectSandwich();

  
  nsSMILValue sandwichResultValue;
  if (!mAnimationFunctions[firstFuncToCompose]->WillReplace()) {
    sandwichResultValue = smilAttr->GetBaseValue();
  }
  UpdateCachedBaseValue(sandwichResultValue);

  if (!mForceCompositing) {
    return;
  }

  
  uint32_t length = mAnimationFunctions.Length();
  for (uint32_t i = firstFuncToCompose; i < length; ++i) {
    mAnimationFunctions[i]->ComposeResult(*smilAttr, sandwichResultValue);
  }
  if (sandwichResultValue.IsNull()) {
    smilAttr->ClearAnimValue();
    return;
  }

  
  nsresult rv = smilAttr->SetAnimValue(sandwichResultValue);
  if (NS_FAILED(rv)) {
    NS_WARNING("nsISMILAttr::SetAnimValue failed");
  }
}

void
nsSMILCompositor::ClearAnimationEffects()
{
  if (!mKey.mElement || !mKey.mAttributeName)
    return;

  nsAutoPtr<nsISMILAttr> smilAttr(CreateSMILAttr());
  if (!smilAttr) {
    
    return;
  }
  smilAttr->ClearAnimValue();
}



nsISMILAttr*
nsSMILCompositor::CreateSMILAttr()
{
  if (mKey.mIsCSS) {
    nsCSSProperty propId =
      nsCSSProps::LookupProperty(nsDependentAtomString(mKey.mAttributeName),
                                 nsCSSProps::eEnabled);
    if (nsSMILCSSProperty::IsPropertyAnimatable(propId)) {
      return new nsSMILCSSProperty(propId, mKey.mElement.get());
    }
  } else {
    return mKey.mElement->GetAnimatedAttr(mKey.mAttributeNamespaceID,
                                          mKey.mAttributeName);
  }
  return nullptr;
}

uint32_t
nsSMILCompositor::GetFirstFuncToAffectSandwich()
{
  uint32_t i;
  for (i = mAnimationFunctions.Length(); i > 0; --i) {
    nsSMILAnimationFunction* curAnimFunc = mAnimationFunctions[i-1];
    
    
    
    
    
    mForceCompositing |=
      curAnimFunc->UpdateCachedTarget(mKey) ||
      curAnimFunc->HasChanged() ||
      curAnimFunc->WasSkippedInPrevSample();

    if (curAnimFunc->WillReplace()) {
      --i;
      break;
    }
  }
  
  
  
  
  
  if (mForceCompositing) {
    for (uint32_t j = i; j > 0; --j) {
      mAnimationFunctions[j-1]->SetWasSkipped();
    }
  }
  return i;
}

void
nsSMILCompositor::UpdateCachedBaseValue(const nsSMILValue& aBaseValue)
{
  if (!mCachedBaseValue) {
    
    mCachedBaseValue = new nsSMILValue(aBaseValue);
    NS_WARN_IF_FALSE(mCachedBaseValue, "failed to cache base value (OOM?)");
    mForceCompositing = true;
  } else if (*mCachedBaseValue != aBaseValue) {
    
    *mCachedBaseValue = aBaseValue;
    mForceCompositing = true;
  }
}
