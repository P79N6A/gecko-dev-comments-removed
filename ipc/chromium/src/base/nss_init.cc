



#include "base/nss_init.h"

#include <nss.h>
#include <plarena.h>
#include <prerror.h>
#include <prinit.h>



#define Lock FOO_NSS_Lock
#include <secmod.h>
#include <ssl.h>
#undef Lock

#include "base/file_util.h"
#include "base/logging.h"
#include "base/singleton.h"

namespace {


SECMODModule *InitDefaultRootCerts() {
  const char* kModulePath = "libnssckbi.so";
  char modparams[1024];
  snprintf(modparams, sizeof(modparams),
          "name=\"Root Certs\" library=\"%s\"", kModulePath);
  SECMODModule *root = SECMOD_LoadUserModule(modparams, NULL, PR_FALSE);
  if (root)
    return root;

  
  
  NOTREACHED();
  return NULL;
}

class NSSInitSingleton {
 public:
  NSSInitSingleton() {
    
    SECStatus status = NSS_NoDB_Init(".");
    if (status != SECSuccess) {
      char buffer[513] = "Couldn't retrieve error";
      PRInt32 err_length = PR_GetErrorTextLength();
      if (err_length > 0 && static_cast<size_t>(err_length) < sizeof(buffer))
        PR_GetErrorText(buffer);

      NOTREACHED() << "Error calling NSS_NoDB_Init: " << buffer;
    }

    root_ = InitDefaultRootCerts();

    NSS_SetDomesticPolicy();

    
    for (int i = 0; i < SSL_NumImplementedCiphers; i++) {
      SSLCipherSuiteInfo info;
      if (SSL_GetCipherSuiteInfo(SSL_ImplementedCiphers[i], &info,
                                 sizeof(info)) == SECSuccess) {
        SSL_CipherPrefSetDefault(SSL_ImplementedCiphers[i],
                                 (info.effectiveKeyBits >= 80));
      }
    }

    
    SSL_OptionSetDefault(SSL_SECURITY, PR_TRUE);

    
  }

  ~NSSInitSingleton() {
    if (root_) {
      SECMOD_UnloadUserModule(root_);
      SECMOD_DestroyModule(root_);
      root_ = NULL;
    }

    
    SSL_ClearSessionCache();

    SECStatus status = NSS_Shutdown();
    if (status != SECSuccess)
      LOG(ERROR) << "NSS_Shutdown failed, leak?  See "
                    "http://code.google.com/p/chromium/issues/detail?id=4609";

    PL_ArenaFinish();

    PRStatus prstatus = PR_Cleanup();
    if (prstatus != PR_SUCCESS)
      LOG(ERROR) << "PR_Cleanup failed?";
  }

 private:
  SECMODModule *root_;
};

}  

namespace base {

void EnsureNSSInit() {
  Singleton<NSSInitSingleton>::get();
}

}  
