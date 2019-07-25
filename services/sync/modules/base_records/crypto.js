



































const EXPORTED_SYMBOLS = ['CryptoWrapper', 'CryptoMeta', 'CryptoMetas'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");

function CryptoWrapper(uri) {
  this._CryptoWrap_init(uri);
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoWrapper",

  _CryptoWrap_init: function CryptoWrap_init(uri) {
    
    
    this._WBORec_init(uri);
    this.data.payload = {
      encryption: "",
      ciphertext: null
    };
  },

  
  
  cleartext: null,

  encrypt: function CryptoWrapper_encrypt(passphrase) {
    
    if (this.deleted)
      return;

    let pubkey = PubKeys.getDefaultKey();
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    let meta = CryptoMetas.get(this.encryption);
    let symkey = meta.getKey(privkey, passphrase);

    this.ciphertext = Svc.Crypto.encrypt(JSON.stringify([this.cleartext]),
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
							symkey, meta.bulkIV))[0];
    this.ciphertext = null;

    return this.cleartext;
  },

  toString: function CryptoWrap_toString() "{ " + [
      "id: " + this.id,
      "parent: " + this.parentid,
      "index: " + this.sortindex,
      "modified: " + this.modified,
      "payload: " + (this.deleted ? "DELETED" : JSON.stringify(this.cleartext))
    ].join("\n  ") + " }",
};

Utils.deferGetSet(CryptoWrapper, "payload", ["encryption", "ciphertext"]);

function CryptoMeta(uri) {
  this._CryptoMeta_init(uri);
}
CryptoMeta.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoMeta",

  _CryptoMeta_init: function CryptoMeta_init(uri) {
    this._WBORec_init(uri);
    this.data.payload = {
      bulkIV: null,
      keyring: {}
    };
  },

  generateIV: function CryptoMeta_generateIV() {
    this.bulkIV = Svc.Crypto.generateRandomIV();
  },

  getKey: function CryptoMeta_getKey(privkey, passphrase) {
    
    if (this._unwrappedKey)
      return this._unwrappedKey;

    
    let pubkeyUri = privkey.publicKeyUri.spec;

    
    let wrapped_key;
    for (let relUri in this.payload.keyring) {
      if (pubkeyUri == this.baseUri.resolve(relUri))
        wrapped_key = this.payload.keyring[relUri];
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

    
    
    for (let relUri in this.payload.keyring) {
      if (pubkeyUri == this.uri.resolve(relUri))
        delete this.payload.keyring[relUri];
    }

    this.payload.keyring[new_pubkey.uri.spec] =
      Svc.Crypto.wrapSymmetricKey(symkey, new_pubkey.keyData);
  }
};

Utils.deferGetSet(CryptoMeta, "data.payload", "bulkIV");

Utils.lazy(this, 'CryptoMetas', CryptoRecordManager);

function CryptoRecordManager() { this._init(); }
CryptoRecordManager.prototype = {
  __proto__: RecordManager.prototype,
  _recordType: CryptoMeta
};
