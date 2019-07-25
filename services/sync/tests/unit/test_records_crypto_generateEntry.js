let atob = Cu.import("resource://services-sync/util.js").atob;
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/base_records/crypto.js");





function run_test() {
  
  
  let bundle = new SyncKeyBundle(PWDMGR_PASSPHRASE_REALM, "st3fan", "q7ynpwq7vsc9m34hankbyi3s3i");
  
  
  let e = "14b8c09fa84e92729ee695160af6e0385f8f6215a25d14906e1747bdaa2de426";
  let h = "370e3566245d79fe602a3adb5137e42439cd2a571235197e0469d7d541b07875";
  
  
  let realE = Utils.bytesAsHex(atob(bundle.encryptionKey));
  let realH = Utils.bytesAsHex(bundle.hmacKey);
  
  _("Real E: " + realE);
  _("Real H: " + realH);
  do_check_eq(realH, h);
  do_check_eq(realE, e);
}
