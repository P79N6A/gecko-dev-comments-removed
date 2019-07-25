




































#ifndef nsPersistentProperties_h___
#define nsPersistentProperties_h___

#include "nsIPersistentProperties2.h"
#include "pldhash.h"
#include "plarena.h"
#include "nsString.h"
#include "nsCOMPtr.h"

#include "nsIUnicharInputStream.h"


class nsPersistentProperties : public nsIPersistentProperties
{
public:
  nsPersistentProperties();
  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_NSIPERSISTENTPROPERTIES

  static nsresult
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
  ~nsPersistentProperties();

protected:
  nsCOMPtr<nsIUnicharInputStream> mIn;

  nsIPersistentProperties* mSubclass;
  struct PLDHashTable mTable;
  PLArenaPool mArena;
};

class nsPropertyElement : public nsIPropertyElement
{
public:
  nsPropertyElement()
  {
  }

  nsPropertyElement(const nsACString& aKey, const nsAString& aValue)
    : mKey(aKey), mValue(aValue)
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTYELEMENT

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
  ~nsPropertyElement() {}

protected:
  nsCString mKey;
  nsString mValue;
};

#endif 
