





































#include "nsIStringEnumerator.h"
#include "nsVoidArray.h"




































NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                           const nsCStringArray* aArray);

NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                       const nsStringArray* aArray);










NS_COM nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                               nsStringArray* aArray);

NS_COM nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                                   nsCStringArray* aArray);














NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult NS_OUTPARAM,
                       const nsStringArray* aArray,
                       nsISupports* aOwner);
NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult NS_OUTPARAM,
                           const nsCStringArray* aArray,
                           nsISupports* aOwner);
