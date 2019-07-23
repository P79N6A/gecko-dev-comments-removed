




































#ifndef nsGREResProperties_h__
#define nsGREResProperties_h__

#include "nsString.h"
#include "nsIPersistentProperties2.h"
#include "nsCOMPtr.h"





class nsGREResProperties {
public:
  


  nsGREResProperties(const nsACString& aFile);

  


  PRBool DidLoad() const;
  nsresult Get(const nsAString& aKey, nsAString& value);

private:
  nsCOMPtr<nsIPersistentProperties> mProps;
};

#endif 
