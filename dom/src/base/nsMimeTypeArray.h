




































#ifndef nsMimeTypeArray_h___
#define nsMimeTypeArray_h___

#include "nsIDOMMimeTypeArray.h"
#include "nsIDOMMimeType.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsIDOMNavigator;

class nsMimeTypeArray : public nsIDOMMimeTypeArray
{
public:
  nsMimeTypeArray(nsIDOMNavigator* navigator);
  virtual ~nsMimeTypeArray();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMIMETYPEARRAY

  nsresult Refresh();

private:
  nsresult GetMimeTypes();
  void     Clear();

protected:
  nsIDOMNavigator* mNavigator;
  PRUint32 mMimeTypeCount;
  nsIDOMMimeType** mMimeTypeArray;
};

class nsMimeType : public nsIDOMMimeType
{
public:
  nsMimeType(nsIDOMPlugin* aPlugin, nsIDOMMimeType* aMimeType);
  virtual ~nsMimeType();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMIMETYPE

protected:
  nsIDOMPlugin* mPlugin;
  nsCOMPtr<nsIDOMMimeType> mMimeType;
};

class nsHelperMimeType : public nsIDOMMimeType
{
public:
  nsHelperMimeType(const nsAString& aType)
    : mType(aType)
  {
  }

  virtual ~nsHelperMimeType()
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMIMETYPE 
  
private:
  nsString mType;
};

#endif 
