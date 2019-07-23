#if 0





































#endif




function checkCert(channel) {
  if (!channel.originalURI.schemeIs("https"))  
    return;

  const Ci = Components.interfaces;  
  var cert =
      channel.securityInfo.QueryInterface(Ci.nsISSLStatusProvider).
      SSLStatus.QueryInterface(Ci.nsISSLStatus).serverCert;

  var issuer = cert.issuer;
  while (issuer && !cert.equals(issuer)) {
    cert = issuer;
    issuer = cert.issuer;
  }

  if (!issuer || issuer.tokenName != "Builtin Object Token")
    throw "cert issuer is not built-in";
}






function BadCertHandler() {
}
BadCertHandler.prototype = {

  
  confirmUnknownIssuer: function(socketInfo, cert, certAddType) {
    LOG("EM BadCertHandler: Unknown issuer");
    return false;
  },

  confirmMismatchDomain: function(socketInfo, targetURL, cert) {
    LOG("EM BadCertHandler: Mismatched domain");
    return false;
  },

  confirmCertExpired: function(socketInfo, cert) {
    LOG("EM BadCertHandler: Expired certificate");
    return false;
  },

  notifyCrlNextupdate: function(socketInfo, targetURL, cert) {
  },

  
  onChannelRedirect: function(oldChannel, newChannel, flags) {
    
    
    checkCert(oldChannel);
  },

  
  getInterface: function(iid) {
    return this.QueryInterface(iid);
  },

  
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.nsIBadCertListener) &&
        !iid.equals(Components.interfaces.nsIChannelEventSink) &&
        !iid.equals(Components.interfaces.nsIInterfaceRequestor) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
