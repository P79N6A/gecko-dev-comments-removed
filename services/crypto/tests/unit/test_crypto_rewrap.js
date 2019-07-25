let cryptoSvc;
try {
  Components.utils.import("resource://services-crypto/threaded.js");
  cryptoSvc = new ThreadedCrypto();
} catch (ex) {
  
  cryptoSvc = Cc["@labs.mozilla.com/Weave/Crypto;1"]
                .getService(Ci.IWeaveCrypto);
}

function run_test() {
  var salt = cryptoSvc.generateRandomBytes(16);
  var iv = cryptoSvc.generateRandomIV();
  var symKey = cryptoSvc.generateRandomKey();

  
  do_check_eq(cryptoSvc.keypairBits, 2048)
  var pubOut = {};
  var privOut = {};
  cryptoSvc.generateKeypair("old passphrase", salt, iv, pubOut, privOut);
  var pubKey = pubOut.value;
  var privKey = privOut.value;

  
  var wrappedKey = cryptoSvc.wrapSymmetricKey(symKey, pubKey);
  var unwrappedKey = cryptoSvc.unwrapSymmetricKey(wrappedKey, privKey,
                                                  "old passphrase", salt, iv);

  
  do_check_eq(unwrappedKey, symKey);

  
  var newPrivKey = cryptoSvc.rewrapPrivateKey(privKey, "old passphrase",
                                              salt, iv, "new passphrase");
  
  
  var newUnwrappedKey = cryptoSvc.unwrapSymmetricKey(wrappedKey, newPrivKey,
                                                     "new passphrase", salt, iv);
  
  
  do_check_eq(newUnwrappedKey, unwrappedKey);
}
