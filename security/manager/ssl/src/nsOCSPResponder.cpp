





































#include "nsOCSPResponder.h"

#include "nsCOMPtr.h"
#include "nsIDateTimeFormat.h"
#include "nsDateTimeFormatCID.h"
#include "nsComponentManagerUtils.h"
#include "nsReadableUtils.h"

#include "certdb.h"


NS_IMPL_ISUPPORTS1(nsOCSPResponder, nsIOCSPResponder)

nsOCSPResponder::nsOCSPResponder()
{
  
}

nsOCSPResponder::nsOCSPResponder(const PRUnichar * aCA, const PRUnichar * aURL)
{
  mCA.Assign(aCA);
  mURL.Assign(aURL);
}

nsOCSPResponder::~nsOCSPResponder()
{
  
}


NS_IMETHODIMP nsOCSPResponder::GetResponseSigner(PRUnichar** aCA)
{
  NS_ENSURE_ARG(aCA);
  *aCA = ToNewUnicode(mCA);
  return NS_OK;
}


NS_IMETHODIMP nsOCSPResponder::GetServiceURL(PRUnichar** aURL)
{
  NS_ENSURE_ARG(aURL);
  *aURL = ToNewUnicode(mURL);
  return NS_OK;
}

PRBool nsOCSPResponder::IncludeCert(CERTCertificate *aCert)
{
  CERTCertTrust *trust;
  char *nickname;

  trust = aCert->trust;
  nickname = aCert->nickname;

  PR_ASSERT(trust != nsnull);

  
  if (trust == nsnull) {
    return PR_FALSE;
  }

  if ( ( ( trust->sslFlags & CERTDB_INVISIBLE_CA ) ||
         (trust->emailFlags & CERTDB_INVISIBLE_CA ) ||
         (trust->objectSigningFlags & CERTDB_INVISIBLE_CA ) ) ||
       nickname == NULL) {
      return PR_FALSE;
  }
  if ((trust->sslFlags & CERTDB_VALID_CA) ||
      (trust->emailFlags & CERTDB_VALID_CA) ||
      (trust->objectSigningFlags & CERTDB_VALID_CA)) {
      return PR_TRUE;
  }
  return PR_FALSE;
}





PRInt32 nsOCSPResponder::CmpCAName(nsIOCSPResponder *a, nsIOCSPResponder *b)
{
  PRInt32 cmp1;
  nsXPIDLString aTok, bTok;
  a->GetResponseSigner(getter_Copies(aTok));
  b->GetResponseSigner(getter_Copies(bTok));
  if (aTok != nsnull && bTok != nsnull) {
    cmp1 = Compare(aTok, bTok);
  } else {
    cmp1 = (aTok == nsnull) ? 1 : -1;
  }
  return cmp1;
}





PRInt32 nsOCSPResponder::CompareEntries(nsIOCSPResponder *a, nsIOCSPResponder *b)
{
  nsXPIDLString aURL, bURL;
  nsAutoString aURLAuto, bURLAuto;

  a->GetServiceURL(getter_Copies(aURL));
  aURLAuto.Assign(aURL);
  b->GetServiceURL(getter_Copies(bURL));
  bURLAuto.Assign(bURL);

  if (!aURLAuto.IsEmpty()) {
    if (!bURLAuto.IsEmpty()) {
      return nsOCSPResponder::CmpCAName(a, b);
    } else {
      return -1;
    }
  } else {
    if (!bURLAuto.IsEmpty()) {
      return 1;
    } else {
      return nsOCSPResponder::CmpCAName(a, b);
    }
  }
}

