






































#ifndef _NS_NSSCERTIFICATE_H_
#define _NS_NSSCERTIFICATE_H_

#include "nsIX509Cert.h"
#include "nsIX509Cert2.h"
#include "nsIX509Cert3.h"
#include "nsIX509CertDB.h"
#include "nsIX509CertList.h"
#include "nsIASN1Object.h"
#include "nsISMimeCert.h"
#include "nsNSSShutDown.h"
#include "nsISimpleEnumerator.h"

#include "nsNSSCertHeader.h"

class nsINSSComponent;
class nsIASN1Sequence;


class nsNSSCertificate : public nsIX509Cert,
                         public nsIX509Cert2,
                         public nsIX509Cert3,
                         public nsISMimeCert,
                         public nsNSSShutDownObject
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERT
  NS_DECL_NSIX509CERT2
  NS_DECL_NSIX509CERT3
  NS_DECL_NSISMIMECERT

  nsNSSCertificate(CERTCertificate *cert);
  
  virtual ~nsNSSCertificate();
  nsresult FormatUIStrings(const nsAutoString &nickname, nsAutoString &nickWithSerial, nsAutoString &details);
  static nsNSSCertificate* ConstructFromDER(char *certDER, int derLen);

  static char* defaultServerNickname(CERTCertificate* cert);

private:
  CERTCertificate *mCert;
  PRBool           mPermDelete;
  PRUint32         mCertType;
  nsCOMPtr<nsIASN1Object> mASN1Structure;
  nsresult CreateASN1Struct();
  nsresult CreateTBSCertificateASN1Struct(nsIASN1Sequence **retSequence,
                                          nsINSSComponent *nssComponent);
  nsresult GetSortableDate(PRTime aTime, nsAString &_aSortableDate);
  virtual void virtualDestroyNSSReference();
  void destructorSafeDestroyNSSReference();
};

class nsNSSCertList: public nsIX509CertList
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIX509CERTLIST

  nsNSSCertList(CERTCertList *certList = nsnull, PRBool adopt = PR_FALSE);
  virtual ~nsNSSCertList();

  static CERTCertList *DupCertList(CERTCertList *aCertList);
private:
  CERTCertList *mCertList;
};

class nsNSSCertListEnumerator: public nsISimpleEnumerator
{
public:
   NS_DECL_ISUPPORTS
   NS_DECL_NSISIMPLEENUMERATOR

   nsNSSCertListEnumerator(CERTCertList *certList);
   virtual ~nsNSSCertListEnumerator();
private:
   CERTCertList *mCertList;
};


#define NS_NSS_LONG 4
#define NS_NSS_GET_LONG(x) ((((unsigned long)((x)[0])) << 24) | \
                            (((unsigned long)((x)[1])) << 16) | \
                            (((unsigned long)((x)[2])) <<  8) | \
                             ((unsigned long)((x)[3])) )
#define NS_NSS_PUT_LONG(src,dest) (dest)[0] = (((src) >> 24) & 0xff); \
                                  (dest)[1] = (((src) >> 16) & 0xff); \
                                  (dest)[2] = (((src) >>  8) & 0xff); \
                                  (dest)[3] = ((src) & 0xff); 




#endif 
