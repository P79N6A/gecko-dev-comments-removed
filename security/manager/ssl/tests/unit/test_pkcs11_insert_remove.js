











do_get_profile();
Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

const gExpectedTokenLabel = "Test PKCS11 TokeÃ± Label";

function SmartcardObserver(type) {
  this.type = type;
  do_test_pending();
}

SmartcardObserver.prototype = {
  observe: function(subject, topic, data) {
    do_check_eq(topic, this.type);
    do_check_eq(gExpectedTokenLabel, data);
    Services.obs.removeObserver(this, this.type);
    do_test_finished();
  }
};

function run_test() {
  Services.obs.addObserver(new SmartcardObserver("smartcard-insert"),
                           "smartcard-insert", false);
  Services.obs.addObserver(new SmartcardObserver("smartcard-remove"),
                           "smartcard-remove", false);

  let pkcs11 = Cc["@mozilla.org/security/pkcs11;1"].getService(Ci.nsIPKCS11);
  do_register_cleanup(function() {
    pkcs11.deleteModule("PKCS11 Test Module");
  });
  let libraryName = ctypes.libraryName("pkcs11testmodule");
  let libraryFile = Services.dirsvc.get("CurWorkD", Ci.nsILocalFile);
  libraryFile.append("pkcs11testmodule");
  libraryFile.append(libraryName);
  ok(libraryFile.exists());
  pkcs11.addModule("PKCS11 Test Module", libraryFile.path, 0, 0);
}
