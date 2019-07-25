



































const EXPORTED_SYMBOLS = ['CryptoWrapper', 'CryptoMeta', 'CryptoMetas'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/keys.js");
Cu.import("resource://services-sync/base_records/wbo.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");

function CryptoWrapper(uri) {
  this.cleartext = {};
  WBORecord.call(this, uri);
  this.encryption = "";
  this.ciphertext = null;
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoWrapper",

  encrypt: function CryptoWrapper_encrypt(passphrase) {
    let pubkey = PubKeys.getDefaultKey();
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = meta.getKey(privkey, passphrase);

    this.IV = Svc.Crypto.generateRandomIV();
    this.ciphertext = Svc.Crypto.encrypt(JSON.stringify(this.cleartext),
                                         symkey, this.IV);
    this.hmac = Utils.sha256HMAC(this.ciphertext, symkey.hmacKey);
    this.cleartext = null;
  },

  decrypt: function CryptoWrapper_decrypt(passphrase) {
    let pubkey = PubKeys.getDefaultKey();
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = meta.getKey(privkey, passphrase);

    
    if (Utils.sha256HMAC(this.ciphertext, symkey.hmacKey) != this.hmac)
      throw "Record SHA256 HMAC mismatch: " + this.hmac;

    this.cleartext = JSON.parse(Svc.Crypto.decrypt(this.ciphertext, symkey,
                                                   this.IV));
    this.ciphertext = null;

    
    if (this.cleartext.id != this.id)
      throw "Record id mismatch: " + [this.cleartext.id, this.id];

    return this.cleartext;
  },

  toString: function CryptoWrap_toString() "{ " + [
      "id: " + this.id,
      "index: " + this.sortindex,
      "modified: " + this.modified,
      "payload: " + (this.deleted ? "DELETED" : JSON.stringify(this.cleartext))
    ].join("\n  ") + " }",

  
  get id() WBORecord.prototype.__lookupGetter__("id").call(this),

  
  set id(val) {
    WBORecord.prototype.__lookupSetter__("id").call(this, val);
    return this.cleartext.id = val;
  },
};

Utils.deferGetSet(CryptoWrapper, "payload", ["ciphertext", "encryption", "IV",
                                             "hmac"]);
Utils.deferGetSet(CryptoWrapper, "cleartext", "deleted");

function CryptoMeta(uri) {
  WBORecord.call(this, uri);
  this.keyring = {};
}
CryptoMeta.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoMeta",

  getWrappedKey: function _getWrappedKey(privkey) {
    
    let pubkeyUri = privkey.publicKeyUri.spec;

    
    for (let relUri in this.keyring) {
      if (pubkeyUri == this.baseUri.resolve(relUri))
        return this.keyring[relUri];
    }
    return null;
  },

  getKey: function CryptoMeta_getKey(privkey, passphrase) {
    let wrapped_key = this.getWrappedKey(privkey);
    if (!wrapped_key)
      throw "keyring doesn't contain a key for " + privkey.publicKeyUri.spec;

    
    let localHMAC = Utils.sha256HMAC(wrapped_key.wrapped, this.hmacKey);
    if (localHMAC != wrapped_key.hmac)
      throw "Key SHA256 HMAC mismatch: " + wrapped_key.hmac;

    
    let unwrappedKey = new String(
      Svc.Crypto.unwrapSymmetricKey(
        wrapped_key.wrapped,
        privkey.keyData,
        passphrase.passwordUTF8,
        privkey.salt,
        privkey.iv
      )
    );

    unwrappedKey.hmacKey = Svc.KeyFactory.keyFromString(Ci.nsIKeyObject.HMAC,
      unwrappedKey);

    
    return (this.getKey = function() unwrappedKey)();
  },

  addKey: function CryptoMeta_addKey(new_pubkey, privkey, passphrase) {
    let symkey = this.getKey(privkey, passphrase);
    this.addUnwrappedKey(new_pubkey, symkey);
  },

  addUnwrappedKey: function CryptoMeta_addUnwrappedKey(new_pubkey, symkey) {
    
    if (typeof new_pubkey == "string")
      new_pubkey = PubKeys.get(new_pubkey);

    
    
    for (let relUri in this.keyring) {
      if (new_pubkey.uri.spec == this.uri.resolve(relUri))
        delete this.keyring[relUri];
    }

    
    let wrapped = Svc.Crypto.wrapSymmetricKey(symkey, new_pubkey.keyData);
    this.keyring[new_pubkey.uri.spec] = {
      wrapped: wrapped,
      hmac: Utils.sha256HMAC(wrapped, this.hmacKey)
    };
  },

  get hmacKey() {
    let passphrase = ID.get("WeaveCryptoID").passwordUTF8;
    return Svc.KeyFactory.keyFromString(Ci.nsIKeyObject.HMAC, passphrase);
  }
};

Utils.deferGetSet(CryptoMeta, "payload", "keyring");

Utils.lazy(this, 'CryptoMetas', CryptoRecordManager);

function CryptoRecordManager() {
  RecordManager.call(this);
}
CryptoRecordManager.prototype = {
  __proto__: RecordManager.prototype,
  _recordType: CryptoMeta
};
