var btoa;

function test_derive(cryptoSvc) {
  
  let pp = "secret phrase";
  let salt = "RE5YUHpQcGl3bg==";   
  
  
  let k = cryptoSvc.deriveKeyFromPassphrase(pp, salt, 16);
  do_check_eq(16, k.length);
  do_check_eq(btoa(k), "d2zG0d2cBfXnRwMUGyMwyg==");
  
  
  k = cryptoSvc.deriveKeyFromPassphrase(pp, salt, 32);
  do_check_eq(32, k.length);
  let encKey = btoa(k);
  
  
  let iv = cryptoSvc.generateRandomIV();
  do_check_eq(cryptoSvc.decrypt(cryptoSvc.encrypt("bacon", encKey, iv), encKey, iv), "bacon");
  
  
  k = cryptoSvc.deriveKeyFromPassphrase(pp, salt, null);
  do_check_eq(32, k.length);
  do_check_eq(encKey, btoa(k));
}

function run_test() {
  let cryptoSvc;
  try {
    let backstagePass = Components.utils.import("resource://services-crypto/WeaveCrypto.js");
    btoa = backstagePass.btoa;
  } catch (ex) {
    _("Aborting test: no WeaveCrypto.js.");
    return;
  }
  test_derive(new WeaveCrypto());
}
