








#include "CSSVariableResolver.h"

#include "CSSVariableDeclarations.h"
#include "CSSVariableValues.h"
#include "mozilla/PodOperations.h"
#include <algorithm>

namespace mozilla {





class EnumerateVariableReferencesData
{
public:
  explicit EnumerateVariableReferencesData(CSSVariableResolver& aResolver)
    : mResolver(aResolver)
    , mReferences(new bool[aResolver.mVariables.Length()])
  {
  }

  



  void Reset()
  {
    PodZero(mReferences.get(), mResolver.mVariables.Length());
    mReferencesNonExistentVariable = false;
  }

  void RecordVariableReference(const nsAString& aVariableName)
  {
    size_t id;
    if (mResolver.mVariableIDs.Get(aVariableName, &id)) {
      mReferences[id] = true;
    } else {
      mReferencesNonExistentVariable = true;
    }
  }

  bool HasReferenceToVariable(size_t aID) const
  {
    return mReferences[aID];
  }

  bool ReferencesNonExistentVariable() const
  {
   return mReferencesNonExistentVariable;
  }

private:
  CSSVariableResolver& mResolver;

  
  
  
  
  nsAutoArrayPtr<bool> mReferences;

  
  
  bool mReferencesNonExistentVariable;
};

static void
RecordVariableReference(const nsAString& aVariableName,
                        void* aData)
{
  static_cast<EnumerateVariableReferencesData*>(aData)->
    RecordVariableReference(aVariableName);
}

void
CSSVariableResolver::RemoveCycles(size_t v)
{
  mVariables[v].mIndex = mNextIndex;
  mVariables[v].mLowLink = mNextIndex;
  mVariables[v].mInStack = true;
  mStack.AppendElement(v);
  mNextIndex++;

  for (size_t i = 0, n = mReferences[v].Length(); i < n; i++) {
    size_t w = mReferences[v][i];
    if (!mVariables[w].mIndex) {
      RemoveCycles(w);
      mVariables[v].mLowLink = std::min(mVariables[v].mLowLink,
                                        mVariables[w].mLowLink);
    } else if (mVariables[w].mInStack) {
      mVariables[v].mLowLink = std::min(mVariables[v].mLowLink,
                                        mVariables[w].mIndex);
    }
  }

  if (mVariables[v].mLowLink == mVariables[v].mIndex) {
    if (mStack.LastElement() == v) {
      
      
      
      mVariables[mStack.LastElement()].mInStack = false;
      mStack.TruncateLength(mStack.Length() - 1);
    } else {
      size_t w;
      do {
        w = mStack.LastElement();
        mVariables[w].mValue.Truncate(0);
        mVariables[w].mInStack = false;
        mStack.TruncateLength(mStack.Length() - 1);
      } while (w != v);
    }
  }
}

void
CSSVariableResolver::ResolveVariable(size_t aID)
{
  if (mVariables[aID].mValue.IsEmpty() || mVariables[aID].mWasInherited) {
    
    
    mOutput->Put(mVariables[aID].mVariableName,
                 mVariables[aID].mValue,
                 mVariables[aID].mFirstToken,
                 mVariables[aID].mLastToken);
  } else {
    
    
    
    
    
    
    
    
    for (size_t i = 0, n = mReferences[aID].Length(); i < n; i++) {
      size_t j = mReferences[aID][i];
      if (aID != j && !mVariables[j].mResolved) {
        ResolveVariable(j);
      }
    }
    nsString resolvedValue;
    nsCSSTokenSerializationType firstToken, lastToken;
    if (!mParser.ResolveVariableValue(mVariables[aID].mValue, mOutput,
                                      resolvedValue, firstToken, lastToken)) {
      resolvedValue.Truncate(0);
    }
    mOutput->Put(mVariables[aID].mVariableName, resolvedValue,
                 firstToken, lastToken);
  }
  mVariables[aID].mResolved = true;
}

void
CSSVariableResolver::Resolve(const CSSVariableValues* aInherited,
                             const CSSVariableDeclarations* aSpecified)
{
  MOZ_ASSERT(!mResolved);

  
  
  
  
  MOZ_ASSERT(aInherited);
  MOZ_ASSERT(aSpecified);

  aInherited->AddVariablesToResolver(this);
  aSpecified->AddVariablesToResolver(this);

  
  
  size_t n = mVariables.Length();
  mReferences.SetLength(n);
  EnumerateVariableReferencesData data(*this);
  for (size_t id = 0; id < n; id++) {
    data.Reset();
    if (!mVariables[id].mWasInherited &&
        !mVariables[id].mValue.IsEmpty()) {
      if (mParser.EnumerateVariableReferences(mVariables[id].mValue,
                                              RecordVariableReference,
                                              &data)) {
        
        
        for (size_t i = 0; i < n; i++) {
          if (data.HasReferenceToVariable(i)) {
            mReferences[id].AppendElement(i);
          }
        }
        
        
        
        if (data.HasReferenceToVariable(id)) {
          mVariables[id].mValue.Truncate();
        }
        
        
        
        mVariables[id].mReferencesNonExistentVariable =
          data.ReferencesNonExistentVariable();
      } else {
        MOZ_ASSERT(false, "EnumerateVariableReferences should not have failed "
                          "if we previously parsed the specified value");
        mVariables[id].mValue.Truncate(0);
      }
    }
  }

  
  
  
  mNextIndex = 1;
  for (size_t id = 0; id < n; id++) {
    if (!mVariables[id].mIndex) {
      RemoveCycles(id);
      MOZ_ASSERT(mStack.IsEmpty());
    }
  }

  
  
  for (size_t id = 0; id < n; id++) {
    if (!mVariables[id].mResolved) {
      ResolveVariable(id);
    }
  }

#ifdef DEBUG
  mResolved = true;
#endif
}

void
CSSVariableResolver::Put(const nsAString& aVariableName,
                         nsString aValue,
                         nsCSSTokenSerializationType aFirstToken,
                         nsCSSTokenSerializationType aLastToken,
                         bool aWasInherited)
{
  MOZ_ASSERT(!mResolved);

  size_t id;
  if (mVariableIDs.Get(aVariableName, &id)) {
    MOZ_ASSERT(mVariables[id].mWasInherited && !aWasInherited,
               "should only overwrite inherited variables with specified "
               "variables");
    mVariables[id].mValue = aValue;
    mVariables[id].mFirstToken = aFirstToken;
    mVariables[id].mLastToken = aLastToken;
    mVariables[id].mWasInherited = aWasInherited;
  } else {
    id = mVariables.Length();
    mVariableIDs.Put(aVariableName, id);
    mVariables.AppendElement(Variable(aVariableName, aValue,
                                      aFirstToken, aLastToken, aWasInherited));
  }
}

}
