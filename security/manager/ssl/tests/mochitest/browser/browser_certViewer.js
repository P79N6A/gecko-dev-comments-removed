



let gBugWindow;

function onLoad() {
  gBugWindow.removeEventListener("load", onLoad);
  gBugWindow.addEventListener("unload", onUnload);
  gBugWindow.close();
}

function onUnload() {
  gBugWindow.removeEventListener("unload", onUnload);
  window.focus();
  finish();
}



function test() {
  waitForExplicitFinish();
  let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);
  
  
  
  let cert = certdb.findCertByNickname(null, "pgoca");
  ok(cert, "found a certificate to look at");
  let arg = {
    QueryInterface: function() this,
    getISupportAtIndex: function() this.cert,
    cert: cert
  };
  gBugWindow = window.openDialog("chrome://pippki/content/certViewer.xul",
                                 "", "", arg);
  gBugWindow.addEventListener("load", onLoad);
}
