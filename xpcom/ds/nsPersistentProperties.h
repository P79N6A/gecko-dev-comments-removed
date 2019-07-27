





#ifndef nsPersistentProperties_h___
#define nsPersistentProperties_h___

#include "nsIPersistentProperties2.h"
#include "pldhash.h"
#include "plarena.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsIUnicharInputStream;

class nsPersistentProperties final : public nsIPersistentProperties
{
public:
  nsPersistentProperties();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_NSIPERSISTENTPROPERTIES

  static nsresult Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

private:
  ~nsPersistentProperties();

protected:
  nsCOMPtr<nsIUnicharInputStream> mIn;

  PLDHashTable mTable;
  PLArenaPool mArena;
};

class nsPropertyElement final : public nsIPropertyElement
{
public:
  nsPropertyElement()
  {
  }

  nsPropertyElement(const nsACString& aKey, const nsAString& aValue)
    : mKey(aKey)
    , mValue(aValue)
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTYELEMENT

  static NS_METHOD Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

private:
  ~nsPropertyElement() {}

protected:
  nsCString mKey;
  nsString mValue;
};

#endif 
