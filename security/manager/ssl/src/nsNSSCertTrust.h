





































#ifndef _NSNSSCERTTRUST_H_
#define _NSNSSCERTTRUST_H_

#include "certt.h"
#include "certdb.h"






class nsNSSCertTrust
{
public:
  nsNSSCertTrust();
  nsNSSCertTrust(unsigned int ssl, unsigned int email, unsigned int objsign);
  nsNSSCertTrust(CERTCertTrust *t);
  virtual ~nsNSSCertTrust();

  
  PRBool HasAnyCA();
  PRBool HasAnyUser();
  PRBool HasCA(PRBool checkSSL = PR_TRUE, 
               PRBool checkEmail = PR_TRUE,  
               PRBool checkObjSign = PR_TRUE);
  PRBool HasPeer(PRBool checkSSL = PR_TRUE, 
                 PRBool checkEmail = PR_TRUE,  
                 PRBool checkObjSign = PR_TRUE);
  PRBool HasUser(PRBool checkSSL = PR_TRUE, 
                 PRBool checkEmail = PR_TRUE,  
                 PRBool checkObjSign = PR_TRUE);
  PRBool HasTrustedCA(PRBool checkSSL = PR_TRUE, 
                      PRBool checkEmail = PR_TRUE,  
                      PRBool checkObjSign = PR_TRUE);
  PRBool HasTrustedPeer(PRBool checkSSL = PR_TRUE, 
                        PRBool checkEmail = PR_TRUE,  
                        PRBool checkObjSign = PR_TRUE);

  
  
  void SetValidCA();
  
  void SetTrustedServerCA();
  
  void SetTrustedCA();
  
  void SetValidServerPeer();
  
  void SetValidPeer();
  
  void SetTrustedPeer();
  
  void SetUser();

  
  
  void SetSSLTrust(PRBool peer, PRBool tPeer,
                   PRBool ca,   PRBool tCA, PRBool tClientCA,
                   PRBool user, PRBool warn); 

  void SetEmailTrust(PRBool peer, PRBool tPeer,
                     PRBool ca,   PRBool tCA, PRBool tClientCA,
                     PRBool user, PRBool warn);

  void SetObjSignTrust(PRBool peer, PRBool tPeer,
                       PRBool ca,   PRBool tCA, PRBool tClientCA,
                       PRBool user, PRBool warn);

  
  void AddCATrust(PRBool ssl, PRBool email, PRBool objSign);
  
  void AddPeerTrust(PRBool ssl, PRBool email, PRBool objSign);

  
  CERTCertTrust * GetTrust() { return &mTrust; }

private:
  void addTrust(unsigned int *t, unsigned int v);
  void removeTrust(unsigned int *t, unsigned int v);
  PRBool hasTrust(unsigned int t, unsigned int v);
  CERTCertTrust mTrust;
};

#endif
