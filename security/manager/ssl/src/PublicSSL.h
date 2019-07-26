





#ifndef mozilla_SSL_h
#define mozilla_SSL_h

namespace mozilla {

void ClearPrivateSSLState();

namespace psm {

void InitializeSSLServerCertVerificationThreads();
void StopSSLServerCertVerificationThreads();
void ConfigureMD5(bool enabled);
nsresult InitializeCipherSuite();

} 
} 

#endif

