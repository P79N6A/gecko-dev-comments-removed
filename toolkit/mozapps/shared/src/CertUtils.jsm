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

  var errorstring = "cert issuer is not built-in";
  if (!issuer)
    throw errorstring;

  issuer = issuer.QueryInterface(Ci.nsIX509Cert3);
  var tokenNames = issuer.getAllTokenNames({});

  if (!tokenNames.some(isBuiltinToken))
    throw errorstring;
}

function isBuiltinToken(tokenName) {
  return tokenName == "Builtin Object Token";
}






function BadCertHandler() {
}
BadCertHandler.prototype = {

  
  onChannelRedirect: function(oldChannel, newChannel, flags) {
    
    
    checkCert(oldChannel);
  },

  
  notifyCertProblem: function(socketInfo, status, targetSite) {
    return true;
  },

  
  notifySSLError: function(socketInfo, error, targetSite) {
    return true;
  },

  
  getInterface: function(iid) {
    return this.QueryInterface(iid);
  },

  
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.nsIChannelEventSink) &&
        !iid.equals(Components.interfaces.nsIBadCertListener2) &&
        !iid.equals(Components.interfaces.nsISSLErrorListener) &&
        !iid.equals(Components.interfaces.nsIInterfaceRequestor) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
