






#ifndef mozilla_CSSVariableDeclarations_h
#define mozilla_CSSVariableDeclarations_h

#include "nsDataHashtable.h"

namespace mozilla {

class CSSVariableDeclarations
{
public:
  CSSVariableDeclarations();
  CSSVariableDeclarations(const CSSVariableDeclarations& aOther);
#ifdef DEBUG
  ~CSSVariableDeclarations();
#endif
  CSSVariableDeclarations& operator=(const CSSVariableDeclarations& aOther);

  






  bool Has(const nsAString& aName) const;

  


  enum Type {
    eTokenStream,  
    eInitial,      
    eInherit       
  };

  












  bool Get(const nsAString& aName, Type& aType, nsString& aValue) const;

  






  void PutInitial(const nsAString& aName);

  






  void PutInherit(const nsAString& aName);

  







  void PutTokenStream(const nsAString& aName, const nsString& aTokenStream);

  





  void Remove(const nsAString& aName);

  


  uint32_t Count() const { return mVariables.Count(); }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
  


  void CopyVariablesFrom(const CSSVariableDeclarations& aOther);
  static PLDHashOperator EnumerateVariableForCopy(const nsAString& aName,
                                                  nsString aValue,
                                                  void* aData);

  nsDataHashtable<nsStringHashKey, nsString> mVariables;
};

} 

#endif
