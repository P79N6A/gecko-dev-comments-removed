




































#ifndef nsIDOMGCParticipant_h_
#define nsIDOMGCParticipant_h_

#include "nsISupports.h"

template<class E> class nsCOMArray;


#define NS_IDOMGCPARTICIPANT_IID \
{ 0x0e2a5a8d, 0x28fd, 0x4a5c, \
  {0x8b, 0xf1, 0x5b, 0x00, 0x67, 0xff, 0x32, 0x86} }













class nsIDOMGCParticipant : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDOMGCPARTICIPANT_IID)

  










  virtual nsIDOMGCParticipant* GetSCCIndex() = 0;

  









  virtual void AppendReachableList(nsCOMArray<nsIDOMGCParticipant>& aArray) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDOMGCParticipant, NS_IDOMGCPARTICIPANT_IID)

#endif 
