




#ifndef nsSAXLocator_h__
#define nsSAXLocator_h__

#include "nsISAXLocator.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

#define NS_SAXLOCATOR_CONTRACTID "@mozilla.org/saxparser/locator;1"
#define NS_SAXLOCATOR_CID  \
{/* {c1cd4045-846b-43bb-a95e-745a3d7b40e0}*/ \
0xc1cd4045, 0x846b, 0x43bb, \
{ 0xa9, 0x5e, 0x74, 0x5a, 0x3d, 0x7b, 0x40, 0xe0} }

class nsSAXLocator MOZ_FINAL : public nsISAXLocator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISAXLOCATOR

  nsSAXLocator(nsString& aPublicId,
               nsString& aSystemId,
               int32_t aLineNumber,
               int32_t aColumnNumber);

private:
  nsString mPublicId;
  nsString mSystemId;
  int32_t mLineNumber;
  int32_t mColumnNumber;
};

#endif 
