





#ifndef nsIGlobalObject_h__
#define nsIGlobalObject_h__

#include "nsISupports.h"
#include "nsTArray.h"
#include "js/TypeDecls.h"

#define NS_IGLOBALOBJECT_IID \
{ 0x11afa8be, 0xd997, 0x4e07, \
{ 0xa6, 0xa3, 0x6f, 0x87, 0x2e, 0xc3, 0xee, 0x7f } }

class nsACString;
class nsCString;
class nsCycleCollectionTraversalCallback;
class nsIPrincipal;

class nsIGlobalObject : public nsISupports
{
  nsTArray<nsCString> mHostObjectURIs;
  bool mIsDying;

protected:
  nsIGlobalObject()
   : mIsDying(false)
  {}

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IGLOBALOBJECT_IID)

  













  bool
  IsDying() const
  {
    return mIsDying;
  }

  virtual JSObject* GetGlobalJSObject() = 0;

  
  nsIPrincipal* PrincipalOrNull();

  void RegisterHostObjectURI(const nsACString& aURI);

  void UnregisterHostObjectURI(const nsACString& aURI);

  
  
  void UnlinkHostObjectURIs();
  void TraverseHostObjectURIs(nsCycleCollectionTraversalCallback &aCb);

protected:
  virtual ~nsIGlobalObject();

  void
  StartDying()
  {
    mIsDying = true;
  }
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIGlobalObject,
                              NS_IGLOBALOBJECT_IID)

#endif 
