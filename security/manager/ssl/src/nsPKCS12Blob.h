




#ifndef _NS_PKCS12BLOB_H_
#define _NS_PKCS12BLOB_H_

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIPK11TokenDB.h"
#include "nsNSSHelper.h"
#include "nsIPK11Token.h"
#include "nsIMutableArray.h"

#include "nss.h"

#include "pkcs12.h"
#include "p12plcy.h"

class nsIX509Cert;






class nsPKCS12Blob
{
public:
  nsPKCS12Blob();
  virtual ~nsPKCS12Blob();

  
  nsresult SetToken(nsIPK11Token *token);

  
  nsresult ImportFromFile(nsIFile *file);

  
#if 0
  
  nsresult LoadCerts(nsIX509Cert **certs, int numCerts);
#endif
  nsresult ExportToFile(nsIFile *file, nsIX509Cert **certs, int numCerts);

private:

  nsCOMPtr<nsIPK11Token>          mToken;
  nsCOMPtr<nsIMutableArray>       mCertArray;
  nsCOMPtr<nsIInterfaceRequestor> mUIContext;

  
  nsresult getPKCS12FilePassword(SECItem *);
  nsresult newPKCS12FilePassword(SECItem *);
  nsresult inputToDecoder(SEC_PKCS12DecoderContext *, nsIFile *);
  void unicodeToItem(const PRUnichar *, SECItem *);
  void handleError(int myerr = 0);

  
  
  
  
  
  
  
  
  
  
  
  
  enum RetryReason { rr_do_not_retry, rr_bad_password, rr_auto_retry_empty_password_flavors };
  enum ImportMode { im_standard_prompt, im_try_zero_length_secitem };
  
  nsresult ImportFromFileHelper(nsIFile *file, ImportMode aImportMode, RetryReason &aWantRetry);

  
  PRFileDesc *mTmpFile;

  
  nsCString                 *mDigest;
  nsCString::const_iterator *mDigestIterator;

  bool        mTokenSet;

  
  static SECStatus digest_open(void *, PRBool);
  static SECStatus digest_close(void *, PRBool);
  static int       digest_read(void *, unsigned char *, unsigned long);
  static int       digest_write(void *, unsigned char *, unsigned long);
  static SECItem * nickname_collision(SECItem *, PRBool *, void *);
  static void write_export_file(void *arg, const char *buf, unsigned long len);

};

#endif 
