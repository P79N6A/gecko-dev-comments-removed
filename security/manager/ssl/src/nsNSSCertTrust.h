



#ifndef _NSNSSCERTTRUST_H_
#define _NSNSSCERTTRUST_H_

#include "certt.h"
#include "certdb.h"






class nsNSSCertTrust
{
public:
  nsNSSCertTrust();
  nsNSSCertTrust(unsigned int ssl, unsigned int email, unsigned int objsign);
  explicit nsNSSCertTrust(CERTCertTrust *t);
  virtual ~nsNSSCertTrust();

  
  bool HasAnyCA();
  bool HasAnyUser();
  bool HasCA(bool checkSSL = true, 
               bool checkEmail = true,  
               bool checkObjSign = true);
  bool HasPeer(bool checkSSL = true, 
                 bool checkEmail = true,  
                 bool checkObjSign = true);
  bool HasUser(bool checkSSL = true, 
                 bool checkEmail = true,  
                 bool checkObjSign = true);
  bool HasTrustedCA(bool checkSSL = true, 
                      bool checkEmail = true,  
                      bool checkObjSign = true);
  bool HasTrustedPeer(bool checkSSL = true, 
                        bool checkEmail = true,  
                        bool checkObjSign = true);

  
  
  void SetValidCA();
  
  void SetTrustedServerCA();
  
  void SetTrustedCA();
  
  void SetValidServerPeer();
  
  void SetValidPeer();
  
  void SetTrustedPeer();
  
  void SetUser();

  
  
  void SetSSLTrust(bool peer, bool tPeer,
                   bool ca,   bool tCA, bool tClientCA,
                   bool user, bool warn); 

  void SetEmailTrust(bool peer, bool tPeer,
                     bool ca,   bool tCA, bool tClientCA,
                     bool user, bool warn);

  void SetObjSignTrust(bool peer, bool tPeer,
                       bool ca,   bool tCA, bool tClientCA,
                       bool user, bool warn);

  
  void AddCATrust(bool ssl, bool email, bool objSign);
  
  void AddPeerTrust(bool ssl, bool email, bool objSign);

  
  CERTCertTrust * GetTrust() { return &mTrust; }

private:
  void addTrust(unsigned int *t, unsigned int v);
  void removeTrust(unsigned int *t, unsigned int v);
  bool hasTrust(unsigned int t, unsigned int v);
  CERTCertTrust mTrust;
};

#endif
