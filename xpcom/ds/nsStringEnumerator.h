





































#include "nsIStringEnumerator.h"
#include "nsVoidArray.h"




































NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsCStringArray* aArray);

NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsStringArray* aArray);










NS_COM nsresult
NS_NewAdoptingStringEnumerator(nsIStringEnumerator** aResult,
                               nsStringArray* aArray);

NS_COM nsresult
NS_NewAdoptingUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                                   nsCStringArray* aArray);














NS_COM nsresult
NS_NewStringEnumerator(nsIStringEnumerator** aResult,
                       const nsStringArray* aArray,
                       nsISupports* aOwner);
NS_COM nsresult
NS_NewUTF8StringEnumerator(nsIUTF8StringEnumerator** aResult,
                           const nsCStringArray* aArray,
                           nsISupports* aOwner);
