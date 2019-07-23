










































#ifndef nsNodeInfo_h___
#define nsNodeInfo_h___

#include "nsINodeInfo.h"
#include "nsNodeInfoManager.h"
#include "plhash.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"

class nsFixedSizeAllocator;

class nsNodeInfo : public nsINodeInfo
{
public:
  NS_DECL_ISUPPORTS

  
  virtual void GetQualifiedName(nsAString &aQualifiedName) const;
  virtual void GetLocalName(nsAString& aLocalName) const;
  virtual nsresult GetNamespaceURI(nsAString& aNameSpaceURI) const;
  virtual PRBool Equals(const nsAString& aName) const;
  virtual PRBool Equals(const nsAString& aName,
                        const nsAString& aPrefix) const;
  virtual PRBool Equals(const nsAString& aName, PRInt32 aNamespaceID) const;
  virtual PRBool Equals(const nsAString& aName, const nsAString& aPrefix,
                        PRInt32 aNamespaceID) const;
  virtual PRBool NamespaceEquals(const nsAString& aNamespaceURI) const;
  virtual PRBool
    QualifiedNameEqualsInternal(const nsACString& aQualifiedName) const;

  
  
public:
  static nsNodeInfo *Create();
private:
  nsNodeInfo();
protected:
  virtual ~nsNodeInfo();

public:
  






  nsresult Init(nsIAtom *aName, nsIAtom *aPrefix, PRInt32 aNamespaceID,
                nsNodeInfoManager *aOwnerManager);

  


  static void ClearCache();

private:
  static nsFixedSizeAllocator* sNodeInfoPool;

  




   void LastRelease();
};

#endif 
