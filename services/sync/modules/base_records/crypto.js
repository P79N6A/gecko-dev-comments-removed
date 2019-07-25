



































const EXPORTED_SYMBOLS = ['CryptoWrapper', 'CryptoMeta', 'CryptoMetas'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");

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
    
    if (this.deleted)
      return;

    let pubkey = PubKeys.getDefaultKey();
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = meta.getKey(privkey, passphrase);

    this.ciphertext = Svc.Crypto.encrypt(JSON.stringify(this.cleartext),
					 symkey, meta.bulkIV);
    this.cleartext = null;
  },

  decrypt: function CryptoWrapper_decrypt(passphrase) {
    
    if (this.deleted)
      return;

    let pubkey = PubKeys.getDefaultKey();
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = meta.getKey(privkey, passphrase);

    this.cleartext = JSON.parse(Svc.Crypto.decrypt(this.ciphertext,
							symkey, meta.bulkIV));
    this.ciphertext = null;

    
    if (this.cleartext.id != this.id)
      throw "Server attack?! Id mismatch: " + [this.cleartext.id, this.id];

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

Utils.deferGetSet(CryptoWrapper, "payload", ["encryption", "ciphertext"]);

function CryptoMeta(uri) {
  WBORecord.call(this, uri);
  this.bulkIV = null;
  this.keyring = {};
}
CryptoMeta.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoMeta",

  generateIV: function CryptoMeta_generateIV() {
    this.bulkIV = Svc.Crypto.generateRandomIV();
  },

  getKey: function CryptoMeta_getKey(privkey, passphrase) {
    
    if (this._unwrappedKey)
      return this._unwrappedKey;

    
    let pubkeyUri = privkey.publicKeyUri.spec;

    
    let wrapped_key;
    for (let relUri in this.keyring) {
      if (pubkeyUri == this.baseUri.resolve(relUri))
        wrapped_key = this.keyring[relUri];
    }
    if (!wrapped_key)
      throw "keyring doesn't contain a key for " + pubkeyUri;

    return this._unwrappedKey = Svc.Crypto.unwrapSymmetricKey(wrapped_key,
      privkey.keyData, passphrase.password, privkey.salt, privkey.iv);
  },

  addKey: function CryptoMeta_addKey(new_pubkey, privkey, passphrase) {
    let symkey = this.getKey(privkey, passphrase);
    this.addUnwrappedKey(new_pubkey, symkey);
  },

  addUnwrappedKey: function CryptoMeta_addUnwrappedKey(new_pubkey, symkey) {
    
    if (typeof new_pubkey == "string")
      new_pubkey = PubKeys.get(new_pubkey);

    
    
    for (let relUri in this.keyring) {
      if (pubkeyUri == this.uri.resolve(relUri))
        delete this.keyring[relUri];
    }

    this.keyring[new_pubkey.uri.spec] =
      Svc.Crypto.wrapSymmetricKey(symkey, new_pubkey.keyData);
  }
};

Utils.deferGetSet(CryptoMeta, "payload", ["bulkIV", "keyring"]);

Utils.lazy(this, 'CryptoMetas', CryptoRecordManager);

function CryptoRecordManager() {
  RecordManager.call(this);
}
CryptoRecordManager.prototype = {
  __proto__: RecordManager.prototype,
  _recordType: CryptoMeta
};
