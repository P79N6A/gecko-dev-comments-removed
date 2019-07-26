const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/StoreTrustAnchor.jsm");


TrustedRootCertificate.index = Ci.nsIX509CertDB.AppXPCShellRoot;

sendAsyncMessage("addCertCompleted");
