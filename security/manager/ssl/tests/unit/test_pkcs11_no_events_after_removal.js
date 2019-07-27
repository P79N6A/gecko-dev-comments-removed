











do_get_profile();
Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

function run_test() {
  let pkcs11 = Cc["@mozilla.org/security/pkcs11;1"].getService(Ci.nsIPKCS11);
  let libraryName = ctypes.libraryName("pkcs11testmodule");
  let libraryFile = Services.dirsvc.get("CurWorkD", Ci.nsILocalFile);
  libraryFile.append("pkcs11testmodule");
  libraryFile.append(libraryName);
  ok(libraryFile.exists());
  pkcs11.addModule("PKCS11 Test Module", libraryFile.path, 0, 0);
  pkcs11.deleteModule("PKCS11 Test Module");
  Services.obs.addObserver(function() { do_check_true(false); },
                           "smartcard-insert", false);
  Services.obs.addObserver(function() { do_check_true(false); },
                           "smartcard-remove", false);
  do_timeout(500, do_test_finished);
  do_test_pending();
}
