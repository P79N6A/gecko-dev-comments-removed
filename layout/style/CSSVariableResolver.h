








#ifndef mozilla_CSSVariableResolver_h
#define mozilla_CSSVariableResolver_h

#include "mozilla/DebugOnly.h"
#include "nsCSSParser.h"
#include "nsCSSScanner.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"

namespace mozilla {

class CSSVariableDeclarations;
class CSSVariableValues;
class EnumerateVariableReferencesData;

class CSSVariableResolver
{
  friend class CSSVariableDeclarations;
  friend class CSSVariableValues;
  friend class EnumerateVariableReferencesData;
public:
  



  explicit CSSVariableResolver(CSSVariableValues* aOutput)
    : mOutput(aOutput)
#ifdef DEBUG
    , mResolved(false)
#endif
  {
    MOZ_ASSERT(aOutput);
  }

  




  void Resolve(const CSSVariableValues* aInherited,
               const CSSVariableDeclarations* aSpecified);

private:
  struct Variable
  {
    Variable(const nsAString& aVariableName,
             nsString aValue,
             nsCSSTokenSerializationType aFirstToken,
             nsCSSTokenSerializationType aLastToken,
             bool aWasInherited)
      : mVariableName(aVariableName)
      , mValue(aValue)
      , mFirstToken(aFirstToken)
      , mLastToken(aLastToken)
      , mWasInherited(aWasInherited)
      , mResolved(false)
      , mReferencesNonExistentVariable(false)
      , mInStack(false)
      , mIndex(0)
      , mLowLink(0) { }

    nsString mVariableName;
    nsString mValue;
    nsCSSTokenSerializationType mFirstToken;
    nsCSSTokenSerializationType mLastToken;

    
    bool mWasInherited;

    
    bool mResolved;

    
    bool mReferencesNonExistentVariable;

    
    bool mInStack;
    size_t mIndex;
    size_t mLowLink;
  };

  













  void Put(const nsAString& aVariableName,
           nsString aValue,
           nsCSSTokenSerializationType aFirstToken,
           nsCSSTokenSerializationType aLastToken,
           bool aWasInherited);

  
  void RemoveCycles(size_t aID);
  void ResolveVariable(size_t aID);

  
  
  nsDataHashtable<nsStringHashKey, size_t> mVariableIDs;

  
  nsTArray<Variable> mVariables;

  
  nsTArray<nsTArray<size_t> > mReferences;

  
  
  size_t mNextIndex;

  
  
  
  nsTArray<size_t> mStack;

  
  nsCSSParser mParser;

  
  CSSVariableValues* mOutput;

#ifdef DEBUG
  
  bool mResolved;
#endif
};

}

#endif
