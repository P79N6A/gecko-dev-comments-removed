



#ifndef nsHostObjectURI_h
#define nsHostObjectURI_h

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIClassInfo.h"
#include "nsIPrincipal.h"
#include "nsISerializable.h"
#include "nsIURIWithPrincipal.h"
#include "nsSimpleURI.h"






class nsHostObjectURI : public nsSimpleURI,
                        public nsIURIWithPrincipal
{
public:
  nsHostObjectURI(nsIPrincipal* aPrincipal) :
      nsSimpleURI(), mPrincipal(aPrincipal)
  {}

  
  nsHostObjectURI() : nsSimpleURI() {}

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIURIWITHPRINCIPAL
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  
  virtual nsresult CloneInternal(RefHandlingEnum aRefHandlingMode,
                                 nsIURI** aClone) MOZ_OVERRIDE;
  virtual nsresult EqualsInternal(nsIURI* aOther,
                                  RefHandlingEnum aRefHandlingMode,
                                  bool* aResult) MOZ_OVERRIDE;

  
  virtual nsSimpleURI* StartClone(RefHandlingEnum ) MOZ_OVERRIDE
  { return new nsHostObjectURI(); }

  nsCOMPtr<nsIPrincipal> mPrincipal;

protected:
  virtual ~nsHostObjectURI() {}
};

#define NS_HOSTOBJECTURI_CID \
{ 0xf5475c51, 0x59a7, 0x4757, \
  { 0xb3, 0xd9, 0xe2, 0x11, 0xa9, 0x41, 0x08, 0x72 } }

#endif 
