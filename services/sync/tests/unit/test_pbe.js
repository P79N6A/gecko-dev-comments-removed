function run_test() {
  
  let ch = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);

  let pbe = Cc["@labs.mozilla.com/Weave/Crypto;1"].getService(Ci.IWeaveCrypto);

  pbe.algorithm = pbe.DES_EDE3_CBC;
  let cipherTxt = pbe.encrypt("passphrase", "my very secret message!");

  do_check_true(cipherTxt != "my very secret message!");

  let clearTxt = pbe.decrypt("passphrase", cipherTxt);
  do_check_true(clearTxt == "my very secret message!");

  
  
  let failure = false;
  try {
    pbe.decrypt("wrongpassphrase", cipherTxt);
  } catch (e) {
    failure = true;
  }
  do_check_true(failure);
}
