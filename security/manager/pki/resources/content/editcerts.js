





































const nsIX509Cert = Components.interfaces.nsIX509Cert;
const nsX509CertDB = "@mozilla.org/security/x509certdb;1";
const nsIX509CertDB = Components.interfaces.nsIX509CertDB;
const nsIPKIParamBlock = Components.interfaces.nsIPKIParamBlock;

var certdb;
var cert;

function setWindowName()
{
  var dbkey = self.name;

  
  certdb = Components.classes[nsX509CertDB].getService(nsIX509CertDB);
  
  
  
  cert = certdb.findCertByDBKey(dbkey, null);

  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var windowReference = document.getElementById('editCaCert');

  var message1 = bundle.formatStringFromName("editTrustCA",
                                             [ cert.commonName ],
                                             1);
  setText("certmsg", message1);

  var ssl = document.getElementById("trustSSL");
  if (certdb.isCertTrusted(cert, nsIX509Cert.CA_CERT, 
                          nsIX509CertDB.TRUSTED_SSL)) {
    ssl.setAttribute("checked", "true");
  } else {
    ssl.setAttribute("checked", "false");
  }
  var email = document.getElementById("trustEmail");
  if (certdb.isCertTrusted(cert, nsIX509Cert.CA_CERT, 
                          nsIX509CertDB.TRUSTED_EMAIL)) {
    email.setAttribute("checked", "true");
  } else {
    email.setAttribute("checked", "false");
  }
  var objsign = document.getElementById("trustObjSign");
  if (certdb.isCertTrusted(cert, nsIX509Cert.CA_CERT,  
                          nsIX509CertDB.TRUSTED_OBJSIGN)) {
    objsign.setAttribute("checked", "true");
  } else {
    objsign.setAttribute("checked", "false");
  }
}

function doOK()
{
  var ssl = document.getElementById("trustSSL");
  var email = document.getElementById("trustEmail");
  var objsign = document.getElementById("trustObjSign");
  var trustssl = (ssl.checked) ? nsIX509CertDB.TRUSTED_SSL : 0;
  var trustemail = (email.checked) ? nsIX509CertDB.TRUSTED_EMAIL : 0;
  var trustobjsign = (objsign.checked) ? nsIX509CertDB.TRUSTED_OBJSIGN : 0;
  
  
  
  certdb.setCertTrust(cert, nsIX509Cert.CA_CERT, 
                      trustssl | trustemail | trustobjsign);
  return true;
}

function doLoadForSSLCert()
{
  var dbkey = self.name;

  
  certdb = Components.classes[nsX509CertDB].getService(nsIX509CertDB);
  cert = certdb.findCertByDBKey(dbkey, null);

  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var windowReference = document.getElementById('editWebsiteCert');

  var message1 = bundle.formatStringFromName("editTrustSSL",
                                             [ cert.commonName ],
                                             1);
  setText("certmsg", message1);

  setText("issuer", cert.issuerName);

  var cacert = getCaCertForEntityCert(cert);
  if(cacert == null)
  {
     setText("explainations",bundle.GetStringFromName("issuerNotKnown"));
  }
  else if(certdb.isCertTrusted(cacert, nsIX509Cert.CA_CERT,
                                                nsIX509CertDB.TRUSTED_SSL))
  {
     setText("explainations",bundle.GetStringFromName("issuerTrusted"));
  }
  else
  {
     setText("explainations",bundle.GetStringFromName("issuerNotTrusted"));
  }






  
  var trustssl = document.getElementById("trustSSLCert");
  var notrustssl = document.getElementById("dontTrustSSLCert");
  if (certdb.isCertTrusted(cert, nsIX509Cert.SERVER_CERT, 
                          nsIX509CertDB.TRUSTED_SSL)) {
    trustssl.radioGroup.selectedItem = trustssl;
  } else {
    trustssl.radioGroup.selectedItem = notrustssl;
  }
}

function doSSLOK()
{
  var ssl = document.getElementById("trustSSLCert");
  
  var trustssl = ssl.selected ? nsIX509CertDB.TRUSTED_SSL : 0;
  
  
  
  certdb.setCertTrust(cert, nsIX509Cert.SERVER_CERT, trustssl);
  return true;
}

function doLoadForEmailCert()
{
  var dbkey = self.name;

  
  certdb = Components.classes[nsX509CertDB].getService(nsIX509CertDB);
  cert = certdb.findCertByDBKey(dbkey, null);

  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var windowReference = document.getElementById('editEmailCert');

  var message1 = bundle.formatStringFromName("editTrustEmail",
                                             [ cert.commonName ],
                                             1);
  setText("certmsg", message1);

  setText("issuer", cert.issuerName);

  var cacert = getCaCertForEntityCert(cert);
  if(cacert == null)
  {
     setText("explainations",bundle.GetStringFromName("issuerNotKnown"));
  }
  else if(certdb.isCertTrusted(cacert, nsIX509Cert.CA_CERT,
                                                nsIX509CertDB.TRUSTED_EMAIL))
  {
     setText("explainations",bundle.GetStringFromName("issuerTrusted"));
  }
  else
  {
     setText("explainations",bundle.GetStringFromName("issuerNotTrusted"));
  }






  
  var trustemail = document.getElementById("trustEmailCert");
  var notrustemail = document.getElementById("dontTrustEmailCert");
  if (certdb.isCertTrusted(cert, nsIX509Cert.EMAIL_CERT, 
                          nsIX509CertDB.TRUSTED_EMAIL)) {
    trustemail.radioGroup.selectedItem = trustemail;
  } else {
    trustemail.radioGroup.selectedItem = notrustemail;
  }
}

function doEmailOK()
{
  var email = document.getElementById("trustEmailCert");
  
  var trustemail = email.selected ? nsIX509CertDB.TRUSTED_EMAIL : 0;
  
  
  
  certdb.setCertTrust(cert, nsIX509Cert.EMAIL_CERT, trustemail);
  return true;
}

function editCaTrust()
{
   var cacert = getCaCertForEntityCert(cert);
   if(cacert != null)
   {
      window.openDialog('chrome://pippki/content/editcacert.xul', cacert.dbKey,
                        'chrome,centerscreen,modal');
   }
   else
   {
      var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
      alert(bundle.GetStringFromName("issuerCertNotFound"));
   }
}

function getCaCertForEntityCert(cert)
{
   var i=1;
   var nextCertInChain;
   nextCertInChain = cert;
   var lastSubjectName="";
   while(true)
   {
     if(nextCertInChain == null)
     {
        return null;
     }
     if((nextCertInChain.type == nsIX509Cert.CA_CERT) || 
                                 (nextCertInChain.subjectName = lastSubjectName))
     {
        break;
     }

     lastSubjectName = nextCertInChain.subjectName;
     nextCertInChain = nextCertInChain.issuer;
   }

   return nextCertInChain;
}

