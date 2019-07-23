




































#include "nsSMILCompositor.h"
#include "nsHashKeys.h"


inline PRBool
nsSMILCompositorKey::Equals(const nsSMILCompositorKey &aOther) const
{
  return (aOther.mElement       == mElement &&
          aOther.mAttributeName == mAttributeName &&
          aOther.mIsCSS         == mIsCSS);
}


PRBool
nsSMILCompositor::KeyEquals(KeyTypePointer aKey) const
{
  return aKey && aKey->Equals(mKey);
}

 PLDHashNumber
nsSMILCompositor::HashKey(KeyTypePointer aKey)
{
  
  const char *attrName = nsnull;
  aKey->mAttributeName->GetUTF8String(&attrName);
  return NS_PTR_TO_UINT32(aKey->mElement.get()) +
    HashString(attrName) +
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

  
  
  nsAutoPtr<nsISMILAttr> smilAttr;
  if (mKey.mIsCSS) {
    
    
    
  } else {
    smilAttr = mKey.mElement->GetAnimatedAttr(mKey.mAttributeName);
  }

  if (!smilAttr) {
    
    return;
  }

  
  nsSMILAnimationFunction::Comparator comparator;
  mAnimationFunctions.Sort(comparator);

  
  
  
                                 
  PRUint32 length = mAnimationFunctions.Length();
  PRUint32 i;
  for (i = length; i > 0; --i) {
    nsSMILAnimationFunction* curAnimFunc = mAnimationFunctions[i-1];
    
    
    
    
    
    
    
    




    
    if (curAnimFunc->WillReplace()) {
      --i;
      break;
    }
  }
  
  

  
  

  
  nsSMILValue resultValue = smilAttr->GetBaseValue();
  if (resultValue.IsNull()) {
    NS_WARNING("nsISMILAttr::GetBaseValue failed");
    return;
  }
  for (; i < length; ++i) {
    nsSMILAnimationFunction* curAnimFunc = mAnimationFunctions[i];
    if (curAnimFunc) {
      curAnimFunc->ComposeResult(*smilAttr, resultValue);
    }
  }

  
  nsresult rv = smilAttr->SetAnimValue(resultValue);
  if (NS_FAILED(rv)) {
    NS_WARNING("nsISMILAttr::SetAnimValue failed");
  } 
}

 void
nsSMILCompositor::ComposeAttributes(nsSMILCompositorTable& aCompositorTable)
{
  aCompositorTable.EnumerateEntries(DoComposeAttribute, nsnull);
}

 PR_CALLBACK PLDHashOperator
nsSMILCompositor::DoComposeAttribute(nsSMILCompositor* aCompositor,
                                     void* )
{ 
  aCompositor->ComposeAttribute();
  return PL_DHASH_NEXT;
}
