






#include "CSSVariableDeclarations.h"

#include "CSSVariableResolver.h"
#include "nsCSSScanner.h"
#include "nsRuleData.h"




#define INITIAL_VALUE "!"
#define INHERIT_VALUE ";"
#define UNSET_VALUE   ")"

namespace mozilla {

CSSVariableDeclarations::CSSVariableDeclarations()
{
  MOZ_COUNT_CTOR(CSSVariableDeclarations);
}

CSSVariableDeclarations::CSSVariableDeclarations(const CSSVariableDeclarations& aOther)
{
  MOZ_COUNT_CTOR(CSSVariableDeclarations);
  CopyVariablesFrom(aOther);
}

#ifdef DEBUG
CSSVariableDeclarations::~CSSVariableDeclarations()
{
  MOZ_COUNT_DTOR(CSSVariableDeclarations);
}
#endif

CSSVariableDeclarations&
CSSVariableDeclarations::operator=(const CSSVariableDeclarations& aOther)
{
  if (this == &aOther) {
    return *this;
  }

  mVariables.Clear();
  CopyVariablesFrom(aOther);
  return *this;
}

 PLDHashOperator
CSSVariableDeclarations::EnumerateVariableForCopy(const nsAString& aName,
                                                  nsString aValue,
                                                  void* aData)
{
  CSSVariableDeclarations* variables = static_cast<CSSVariableDeclarations*>(aData);
  variables->mVariables.Put(aName, aValue);
  return PL_DHASH_NEXT;
}

void
CSSVariableDeclarations::CopyVariablesFrom(const CSSVariableDeclarations& aOther)
{
  aOther.mVariables.EnumerateRead(EnumerateVariableForCopy, this);
}

bool
CSSVariableDeclarations::Has(const nsAString& aName) const
{
  nsString value;
  return mVariables.Get(aName, &value);
}

bool
CSSVariableDeclarations::Get(const nsAString& aName,
                             Type& aType,
                             nsString& aTokenStream) const
{
  nsString value;
  if (!mVariables.Get(aName, &value)) {
    return false;
  }
  if (value.EqualsLiteral(INITIAL_VALUE)) {
    aType = eInitial;
    aTokenStream.Truncate();
  } else if (value.EqualsLiteral(INHERIT_VALUE)) {
    aType = eInitial;
    aTokenStream.Truncate();
  } else if (value.EqualsLiteral(UNSET_VALUE)) {
    aType = eUnset;
    aTokenStream.Truncate();
  } else {
    aType = eTokenStream;
    aTokenStream = value;
  }
  return true;
}

void
CSSVariableDeclarations::PutTokenStream(const nsAString& aName,
                                        const nsString& aTokenStream)
{
  MOZ_ASSERT(!aTokenStream.EqualsLiteral(INITIAL_VALUE) &&
             !aTokenStream.EqualsLiteral(INHERIT_VALUE) &&
             !aTokenStream.EqualsLiteral(UNSET_VALUE));
  mVariables.Put(aName, aTokenStream);
}

void
CSSVariableDeclarations::PutInitial(const nsAString& aName)
{
  mVariables.Put(aName, NS_LITERAL_STRING(INITIAL_VALUE));
}

void
CSSVariableDeclarations::PutInherit(const nsAString& aName)
{
  mVariables.Put(aName, NS_LITERAL_STRING(INHERIT_VALUE));
}

void
CSSVariableDeclarations::PutUnset(const nsAString& aName)
{
  mVariables.Put(aName, NS_LITERAL_STRING(UNSET_VALUE));
}

void
CSSVariableDeclarations::Remove(const nsAString& aName)
{
  mVariables.Remove(aName);
}

 PLDHashOperator
CSSVariableDeclarations::EnumerateVariableForMapRuleInfoInto(
                                                         const nsAString& aName,
                                                         nsString aValue,
                                                         void* aData)
{
  nsDataHashtable<nsStringHashKey, nsString>* variables =
    static_cast<nsDataHashtable<nsStringHashKey, nsString>*>(aData);
  if (!variables->Contains(aName)) {
    variables->Put(aName, aValue);
  }
  return PL_DHASH_NEXT;
}

void
CSSVariableDeclarations::MapRuleInfoInto(nsRuleData* aRuleData)
{
  if (!(aRuleData->mSIDs & NS_STYLE_INHERIT_BIT(Variables))) {
    return;
  }

  if (!aRuleData->mVariables) {
    aRuleData->mVariables = new CSSVariableDeclarations(*this);
  } else {
    mVariables.EnumerateRead(EnumerateVariableForMapRuleInfoInto,
                             aRuleData->mVariables.get());
  }
}

 PLDHashOperator
CSSVariableDeclarations::EnumerateVariableForAddVariablesToResolver(
                                                         const nsAString& aName,
                                                         nsString aValue,
                                                         void* aData)
{
  CSSVariableResolver* resolver = static_cast<CSSVariableResolver*>(aData);
  if (aValue.EqualsLiteral(INITIAL_VALUE)) {
    
    
    resolver->Put(aName, EmptyString(),
                  eCSSTokenSerialization_Nothing,
                  eCSSTokenSerialization_Nothing,
                  false);
  } else if (aValue.EqualsLiteral(INHERIT_VALUE) ||
             aValue.EqualsLiteral(UNSET_VALUE)) {
    
    
    
    
    
    
    
  } else {
    
    
    
    resolver->Put(aName, aValue,
                  eCSSTokenSerialization_Nothing,
                  eCSSTokenSerialization_Nothing,
                  false);
  }
  return PL_DHASH_NEXT;
}

void
CSSVariableDeclarations::AddVariablesToResolver(
                                           CSSVariableResolver* aResolver) const
{
  mVariables.EnumerateRead(EnumerateVariableForAddVariablesToResolver,
                           aResolver);
}

static size_t
SizeOfTableEntry(const nsAString& aKey,
                 const nsString& aValue,
                 MallocSizeOf aMallocSizeOf,
                 void* aUserArg)
{
  size_t n = 0;
  n += aKey.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
  n += aValue.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
  return n;
}

size_t
CSSVariableDeclarations::SizeOfIncludingThis(
                                      mozilla::MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += mVariables.SizeOfExcludingThis(SizeOfTableEntry, aMallocSizeOf);
  return n;
}

} 
