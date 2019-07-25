let cryptoSvc;
try {
  Components.utils.import("resource://services-crypto/WeaveCrypto.js");
  cryptoSvc = new WeaveCrypto();
} catch (ex) {
  
  cryptoSvc = Cc["@labs.mozilla.com/Weave/Crypto;1"]
                .getService(Ci.IWeaveCrypto);
}

function run_test() {
  var salt = cryptoSvc.generateRandomBytes(16);
  do_check_eq(salt.length, 24);

  var iv = cryptoSvc.generateRandomIV();
  do_check_eq(iv.length, 24);

  var symKey = cryptoSvc.generateRandomKey();
  do_check_eq(symKey.length, 44);


  
  do_check_eq(cryptoSvc.keypairBits, 2048)

  var pubOut = {};
  var privOut = {};
  cryptoSvc.generateKeypair("my passphrase", salt, iv, pubOut, privOut);
  var pubKey = pubOut.value;
  var privKey = privOut.value;
  do_check_true(!!pubKey);
  do_check_true(!!privKey);
  do_check_eq(pubKey.length, 392);
  do_check_true(privKey.length == 1624 || privKey.length == 1644);

  
  var wrappedKey = cryptoSvc.wrapSymmetricKey(symKey, pubKey);
  do_check_eq(wrappedKey.length, 344);

  var unwrappedKey = cryptoSvc.unwrapSymmetricKey(wrappedKey, privKey,
                                                  "my passphrase", salt, iv);
  do_check_eq(unwrappedKey.length, 44);

  
  do_check_eq(unwrappedKey, symKey);


  
  cryptoSvc.keypairBits = 1024;
  do_check_eq(cryptoSvc.keypairBits, 1024)

  cryptoSvc.generateKeypair("my passphrase", salt, iv, pubOut, privOut);
  var pubKey = pubOut.value;
  var privKey = privOut.value;
  do_check_true(!!pubKey);
  do_check_true(!!privKey);
  do_check_eq(pubKey.length, 216);
  do_check_eq(privKey.length, 856);

  
  wrappedKey = cryptoSvc.wrapSymmetricKey(symKey, pubKey);
  do_check_eq(wrappedKey.length, 172);
  unwrappedKey = cryptoSvc.unwrapSymmetricKey(wrappedKey, privKey,
                                                  "my passphrase", salt, iv);
  do_check_eq(unwrappedKey.length, 44);

  
  do_check_eq(unwrappedKey, symKey);


}
