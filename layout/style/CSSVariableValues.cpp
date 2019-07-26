






#include "CSSVariableValues.h"

namespace mozilla {

CSSVariableValues::CSSVariableValues()
{
  MOZ_COUNT_CTOR(CSSVariableValues);
}

CSSVariableValues::CSSVariableValues(const CSSVariableValues& aOther)
{
  MOZ_COUNT_CTOR(CSSVariableValues);
  CopyVariablesFrom(aOther);
}

#ifdef DEBUG
CSSVariableValues::~CSSVariableValues()
{
  MOZ_COUNT_DTOR(CSSVariableValues);
}
#endif

CSSVariableValues&
CSSVariableValues::operator=(const CSSVariableValues& aOther)
{
  if (this == &aOther) {
    return *this;
  }

  mVariableIDs.Clear();
  mVariables.Clear();
  CopyVariablesFrom(aOther);
  return *this;
}

bool
CSSVariableValues::Get(const nsAString& aName, nsString& aValue) const
{
  size_t id;
  if (!mVariableIDs.Get(aName, &id)) {
    return false;
  }
  aValue = mVariables[id].mValue;
  return true;
}

bool
CSSVariableValues::Get(const nsAString& aName,
                       nsString& aValue,
                       nsCSSTokenSerializationType& aFirstToken,
                       nsCSSTokenSerializationType& aLastToken) const
{
  size_t id;
  if (!mVariableIDs.Get(aName, &id)) {
    return false;
  }
  aValue = mVariables[id].mValue;
  aFirstToken = mVariables[id].mFirstToken;
  aLastToken = mVariables[id].mLastToken;
  return true;
}

void
CSSVariableValues::Put(const nsAString& aName,
                       nsString aValue,
                       nsCSSTokenSerializationType aFirstToken,
                       nsCSSTokenSerializationType aLastToken)
{
  size_t id;
  if (mVariableIDs.Get(aName, &id)) {
    mVariables[id].mValue = aValue;
    mVariables[id].mFirstToken = aFirstToken;
    mVariables[id].mLastToken = aLastToken;
  } else {
    id = mVariables.Length();
    mVariableIDs.Put(aName, id);
    mVariables.AppendElement(Variable(aName, aValue, aFirstToken, aLastToken));
  }
}

void
CSSVariableValues::CopyVariablesFrom(const CSSVariableValues& aOther)
{
  for (size_t i = 0, n = aOther.mVariables.Length(); i < n; i++) {
    Put(aOther.mVariables[i].mVariableName,
        aOther.mVariables[i].mValue,
        aOther.mVariables[i].mFirstToken,
        aOther.mVariables[i].mLastToken);
  }
}

} 
