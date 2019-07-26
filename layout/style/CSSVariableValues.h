






#ifndef mozilla_CSSVariableValues_h
#define mozilla_CSSVariableValues_h

#include "nsCSSScanner.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"

namespace mozilla {

class CSSVariableValues
{
public:
  CSSVariableValues();
  CSSVariableValues(const CSSVariableValues& aOther);
#ifdef DEBUG
  ~CSSVariableValues();
#endif
  CSSVariableValues& operator=(const CSSVariableValues& aOther);

  









  bool Get(const nsAString& aName, nsString& aValue) const;

  













  bool Get(const nsAString& aName,
           nsString& aValue,
           nsCSSTokenSerializationType& aFirstToken,
           nsCSSTokenSerializationType& aLastToken) const;

  








  void Put(const nsAString& aName,
           nsString aValue,
           nsCSSTokenSerializationType aFirstToken,
           nsCSSTokenSerializationType aLastToken);

private:
  struct Variable
  {
    Variable()
      : mFirstToken(eCSSTokenSerialization_Nothing)
      , mLastToken(eCSSTokenSerialization_Nothing)
    {}

    Variable(const nsAString& aVariableName,
             nsString aValue,
             nsCSSTokenSerializationType aFirstToken,
             nsCSSTokenSerializationType aLastToken)
      : mVariableName(aVariableName)
      , mValue(aValue)
      , mFirstToken(aFirstToken)
      , mLastToken(aLastToken)
    {}

    nsString mVariableName;
    nsString mValue;
    nsCSSTokenSerializationType mFirstToken;
    nsCSSTokenSerializationType mLastToken;
  };

  


  void CopyVariablesFrom(const CSSVariableValues& aOther);

  



  nsDataHashtable<nsStringHashKey, size_t> mVariableIDs;

  


  nsTArray<Variable> mVariables;
};

} 

#endif
