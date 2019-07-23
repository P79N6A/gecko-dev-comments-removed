




































#ifndef nsPersistentProperties_h___
#define nsPersistentProperties_h___

#include "nsIPersistentProperties2.h"
#include "pldhash.h"
#include "plarena.h"
#include "nsString.h"

class nsIUnicharInputStream;


class nsPersistentProperties : public nsIPersistentProperties
{
public:
  nsPersistentProperties();
  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIPROPERTIES
  NS_DECL_NSIPERSISTENTPROPERTIES


  
  PRInt32 Read();
  PRInt32 SkipLine(PRInt32 c);
  PRInt32 SkipWhiteSpace(PRInt32 c);

  static NS_METHOD
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

private:
  ~nsPersistentProperties();

protected:
  nsIUnicharInputStream* mIn;
  PRUint32 mBufferPos;
  PRUint32 mBufferLength;
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
