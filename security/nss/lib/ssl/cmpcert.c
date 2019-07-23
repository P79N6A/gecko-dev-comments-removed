







































#include <stdio.h>
#include <string.h>
#include "prerror.h"
#include "secitem.h"
#include "prnetdb.h"
#include "cert.h"
#include "nspr.h"
#include "secder.h"
#include "key.h"
#include "nss.h"






SECStatus
NSS_CmpCertChainWCANames(CERTCertificate *cert, CERTDistNames *caNames)
{
  SECItem *         caname;
  CERTCertificate * curcert;
  CERTCertificate * oldcert;
  PRInt32           contentlen;
  int               j;
  int               headerlen;
  int               depth;
  SECStatus         rv;
  SECItem           issuerName;
  SECItem           compatIssuerName;

  if (!cert || !caNames || !caNames->nnames || !caNames->names ||
      !caNames->names->data)
    return SECFailure;
  depth=0;
  curcert = CERT_DupCertificate(cert);
  
  while( curcert ) {
    issuerName = curcert->derIssuer;
    
    



    rv = DER_Lengths(&issuerName, &headerlen, (PRUint32 *)&contentlen);
    if ( rv == SECSuccess ) {
      compatIssuerName.data = &issuerName.data[headerlen];
      compatIssuerName.len = issuerName.len - headerlen;
    } else {
      compatIssuerName.data = NULL;
      compatIssuerName.len = 0;
    }
    
    for (j = 0; j < caNames->nnames; j++) {
      caname = &caNames->names[j];
      if (SECITEM_CompareItem(&issuerName, caname) == SECEqual) {
	rv = SECSuccess;
	CERT_DestroyCertificate(curcert);
	goto done;
      } else if (SECITEM_CompareItem(&compatIssuerName, caname) == SECEqual) {
	rv = SECSuccess;
	CERT_DestroyCertificate(curcert);
	goto done;
      }
    }
    if ( ( depth <= 20 ) &&
	 ( SECITEM_CompareItem(&curcert->derIssuer, &curcert->derSubject)
	   != SECEqual ) ) {
      oldcert = curcert;
      curcert = CERT_FindCertByName(curcert->dbhandle,
				    &curcert->derIssuer);
      CERT_DestroyCertificate(oldcert);
      depth++;
    } else {
      CERT_DestroyCertificate(curcert);
      curcert = NULL;
    }
  }
  rv = SECFailure;
  
done:
  return rv;
}

