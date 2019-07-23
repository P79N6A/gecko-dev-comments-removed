





































#ifndef __NSNSSCERTIFICATEDB_H__
#define __NSNSSCERTIFICATEDB_H__

#include "nsIX509CertDB.h"
#include "nsIX509CertDB2.h"
#include "nsNSSCertHeader.h"

class nsIArray;

class nsNSSCertificateDB : public nsIX509CertDB, public nsIX509CertDB2
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERTDB
  NS_DECL_NSIX509CERTDB2

  nsNSSCertificateDB(); 
  virtual ~nsNSSCertificateDB();

  
  
  static char *
  default_nickname(CERTCertificate *cert, nsIInterfaceRequestor* ctx);

  static nsresult 
  ImportValidCACerts(int numCACerts, SECItem *CACerts, nsIInterfaceRequestor *ctx);

private:

  static nsresult
  ImportValidCACertsInList(CERTCertList *certList, nsIInterfaceRequestor *ctx);

  static void DisplayCertificateAlert(nsIInterfaceRequestor *ctx, 
                                      const char *stringID, nsIX509Cert *certToShow);

  void getCertNames(CERTCertList *certList,
                    PRUint32      type, 
                    PRUint32     *_count,
                    PRUnichar  ***_certNameList);

  CERTDERCerts *getCertsFromPackage(PRArenaPool *arena, PRUint8 *data, 
                                    PRUint32 length);
  nsresult handleCACertDownload(nsIArray *x509Certs, 
                                nsIInterfaceRequestor *ctx);
};

#define NS_X509CERTDB_CID { /* fb0bbc5c-452e-4783-b32c-80124693d871 */ \
    0xfb0bbc5c,                                                        \
    0x452e,                                                            \
    0x4783,                                                            \
    {0xb3, 0x2c, 0x80, 0x12, 0x46, 0x93, 0xd8, 0x71}                   \
  }

#endif
