





































#include "nsIStringEnumerator.h"
#include "nsString.h"
#include "nsTArray.h"




































NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                       const nsTArray<nsString>* aArray,
                       nsISupports* aOwner);
NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                           const nsTArray<nsCString>* aArray);

NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                       const nsTArray<nsString>* aArray);










NS_COM nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                               nsTArray<nsString>* aArray);

NS_COM nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                                   nsTArray<nsCString>* aArray);














NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                           const nsTArray<nsCString>* aArray,
                           nsISupports* aOwner);
