



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
  let certList = certdb.getCerts();
  let enumerator = certList.getEnumerator();
  ok(enumerator.hasMoreElements(), "we have at least one certificate");
  let cert = enumerator.getNext().QueryInterface(Ci.nsIX509Cert);
  ok(cert, "found a certificate to look at");
  info("looking at certificate with nickname " + cert.nickname);
  let arg = {
    QueryInterface: function() this,
    getISupportAtIndex: function() this.cert,
    cert: cert
  };
  gBugWindow = window.openDialog("chrome://pippki/content/certViewer.xul",
                                 "", "", arg);
  gBugWindow.addEventListener("load", onLoad);
}
