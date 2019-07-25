Cu.import("resource://services-crypto/WeaveCrypto.js");
Cu.import("resource://services-sync/util.js");

let cryptoSvc = new WeaveCrypto();

function run_test() {
  if (this.gczeal) {
    _("Running deriveKey tests with gczeal(2).");
    gczeal(2);
  } else {
    _("Running deriveKey tests with default gczeal.");
  }

  var iv = cryptoSvc.generateRandomIV();
  var der_passphrase = "secret phrase";
  var der_salt = "RE5YUHpQcGl3bg==";   
  
  _("Testing deriveKeyFromPassphrase. Input is \"" + der_passphrase + "\", \"" + der_salt + "\" (base64-encoded).");
  
  
  do_check_eq("abcdefghijk8mn9pqrstuvwxyz234567",
              Utils.base32ToFriendly("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"));
  do_check_eq("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567",
              Utils.base32FromFriendly(
                Utils.base32ToFriendly("ABCDEFGHIJKLMNOPQRSTUVWXYZ234567")));
  
  
  do_check_false(Utils.isPassphrase("o-5wmnu-o5tqc-7lz2h-amkbw-izqzi"));  
  do_check_false(Utils.isPassphrase("O-5WMNU-O5TQC-7LZ2H-AMKBW-IZQZI"));  
  do_check_true(Utils.isPassphrase("9-5wmnu-95tqc-78z2h-amkbw-izqzi"));
  do_check_true(Utils.isPassphrase("9-5WMNU-95TQC-78Z2H-AMKBW-IZQZI"));   
  do_check_true(Utils.isPassphrase(
      Utils.normalizePassphrase("9-5WMNU-95TQC-78Z2H-AMKBW-IZQZI")));
    
  
  var der_key = Utils.deriveEncodedKeyFromPassphrase(der_passphrase, der_salt);
  _("Derived key in base64: " + der_key);
  do_check_eq(cryptoSvc.decrypt(cryptoSvc.encrypt("bacon", der_key, iv), der_key, iv), "bacon");
  
  
  var der_key = Utils.deriveEncodedKeyFromPassphrase(der_passphrase, der_salt, 16);
  _("Derived key in base64: " + der_key);
  do_check_eq("d2zG0d2cBfXnRwMUGyMwyg==", der_key);
  do_check_eq(cryptoSvc.decrypt(cryptoSvc.encrypt("bacon", der_key, iv), der_key, iv), "bacon");

  
  var b32key = Utils.derivePresentableKeyFromPassphrase(der_passphrase, der_salt, 16);
  var hyphenated = Utils.hyphenatePassphrase(b32key);
  do_check_true(Utils.isPassphrase(b32key));
  
  _("Derived key in base32: " + b32key);
  do_check_eq(b32key.length, 26);
  do_check_eq(hyphenated.length, 31);  
  do_check_eq(hyphenated, "9-5wmnu-95tqc-78z2h-amkbw-izqzi");

  if (this.gczeal)
    gczeal(0);

  
  
  
  
  do_check_eq(
      Utils.deriveEncodedKeyFromPassphrase(der_passphrase, der_salt, 16, false),
      Utils.deriveEncodedKeyFromPassphrase(der_passphrase, der_salt, 16, true));
}
