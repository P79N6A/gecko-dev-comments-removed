





































#include "nsIStringEnumerator.h"
#include "nsString.h"
#include "nsTArray.h"




































nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                       const nsTArray<nsString>* aArray,
                       nsISupports* aOwner);
nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                           const nsTArray<nsCString>* aArray);

nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                       const nsTArray<nsString>* aArray);










nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                               nsTArray<nsString>* aArray);

nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                                   nsTArray<nsCString>* aArray);














nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                           const nsTArray<nsCString>* aArray,
                           nsISupports* aOwner);
