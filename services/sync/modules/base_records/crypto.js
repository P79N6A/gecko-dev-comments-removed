




































const EXPORTED_SYMBOLS = ["CryptoWrapper", "CollectionKeys", "BulkKeyBundle", "SyncKeyBundle"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/base_records/wbo.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/log4moz.js");

function CryptoWrapper(collection, id) {
  this.cleartext = {};
  WBORecord.call(this, collection, id);
  this.ciphertext = null;
  this.id = id;
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Record.CryptoWrapper",

  ciphertextHMAC: function ciphertextHMAC(keyBundle) {
    let hmacKey = keyBundle.hmacKeyObject;
    if (!hmacKey)
      throw "Cannot compute HMAC with null key.";
    
    return Utils.sha256HMAC(this.ciphertext, hmacKey);
  },

  








  encrypt: function encrypt(keyBundle) {

    keyBundle = keyBundle || CollectionKeys.keyForCollection(this.collection);
    if (!keyBundle)
      throw new Error("Key bundle is null for " + this.uri.spec);

    this.IV = Svc.Crypto.generateRandomIV();
    this.ciphertext = Svc.Crypto.encrypt(JSON.stringify(this.cleartext),
                                         keyBundle.encryptionKey, this.IV);
    this.hmac = this.ciphertextHMAC(keyBundle);
    this.cleartext = null;
  },

  
  decrypt: function decrypt(keyBundle) {
    
    if (!this.ciphertext) {
      throw "No ciphertext: nothing to decrypt?";
    }

    keyBundle = keyBundle || CollectionKeys.keyForCollection(this.collection);
    if (!keyBundle)
      throw new Error("Key bundle is null for " + this.collection + "/" + this.id);

    
    let computedHMAC = this.ciphertextHMAC(keyBundle);

    if (computedHMAC != this.hmac) {
      throw "Record SHA256 HMAC mismatch: " + this.hmac + ", not " + computedHMAC;
    }

    
    let json_result = JSON.parse(Svc.Crypto.decrypt(this.ciphertext,
                                                    keyBundle.encryptionKey, this.IV));
    
    if (json_result && (json_result instanceof Object)) {
      this.cleartext = json_result;
    this.ciphertext = null;
    }
    else {
      throw "Decryption failed: result is <" + json_result + ">, not an object.";
    }

    
    if (this.cleartext.id != this.id)
      throw "Record id mismatch: " + [this.cleartext.id, this.id];

    return this.cleartext;
  },

  toString: function CryptoWrap_toString() "{ " + [
      "id: " + this.id,
      "index: " + this.sortindex,
      "modified: " + this.modified,
      "payload: " + (this.deleted ? "DELETED" : JSON.stringify(this.cleartext)),
      "collection: " + (this.collection || "undefined")
    ].join("\n  ") + " }",

  
  get id() WBORecord.prototype.__lookupGetter__("id").call(this),

  
  set id(val) {
    WBORecord.prototype.__lookupSetter__("id").call(this, val);
    return this.cleartext.id = val;
  },
};

Utils.deferGetSet(CryptoWrapper, "payload", ["ciphertext", "IV", "hmac"]);
Utils.deferGetSet(CryptoWrapper, "cleartext", "deleted");

Utils.lazy(this, "CollectionKeys", CollectionKeyManager);









function CollectionKeyManager() {
  this._lastModified = 0;
  this._collections = {};
  this._default = null;
  
  this._log = Log4Moz.repository.getLogger("CollectionKeys");
}



CollectionKeyManager.prototype = {
  
  clear: function clear() {
    this._log.info("Clearing CollectionKeys...");
    this._lastModified = 0;
    this._collections = {};
    this._default = null;
  },
  
  keyForCollection: function(collection) {
                      
    
    this._log.trace("keyForCollection: " + collection + ". Default is " + (this._default ? "not null." : "null."));
    
    if (collection && this._collections[collection])
      return this._collections[collection];
    
    return this._default;
  },

  



  generateNewKeys: function(collections) {
    let newDefaultKey = new BulkKeyBundle(null, DEFAULT_KEYBUNDLE_NAME);
    newDefaultKey.generateRandom();
    
    let newColls = {};
    if (collections) {
      collections.forEach(function (c) {
        let b = new BulkKeyBundle(null, c);
        b.generateRandom();
        newColls[c] = b;
      });
    }
    this._default = newDefaultKey;
    this._collections = newColls;
    this._lastModified = (Math.round(Date.now()/10)/100);
  },

  asWBO: function(collection, id) {
    let wbo = new CryptoWrapper(collection || "crypto", id || "keys");
    let c = {};
    for (let k in this._collections) {
      c[k] = this._collections[k].keyPair;
    }
    wbo.cleartext = {
      "default": this._default ? this._default.keyPair : null,
      "collections": c,
      "id": id,
      "collection": collection
    };
    wbo.modified = this._lastModified;
    return wbo;
  },

  
  
  updateNeeded: function(info_collections) {

    this._log.info("Testing for updateNeeded. Last modified: " + this._lastModified);

    
    if (!this._lastModified)
      return true;

    
    
    if (!("crypto" in info_collections))
      return true;

    
    return (info_collections["crypto"] > this._lastModified);
  },

  setContents: function setContents(payload, modified) {
    if ("collections" in payload) {
      let out_coll = {};
      let colls = payload["collections"];
      for (let k in colls) {
        let v = colls[k];
        if (v) {
          let keyObj = new BulkKeyBundle(null, k);
          keyObj.keyPair = v;
          if (keyObj) {
            out_coll[k] = keyObj;
          }
        }
      }
      this._collections = out_coll;
    }
    if ("default" in payload) {
      if (payload.default) {
        let b = new BulkKeyBundle(null, DEFAULT_KEYBUNDLE_NAME);
        b.keyPair = payload.default;
        this._default = b;
      }
      else {
        this._default = null;
      }
    }
    
    
    
    
    
    
    this._lastModified = modified || (Math.round(Date.now()/10)/100);
    return payload;
  },

  updateContents: function updateContents(syncKeyBundle, storage_keys) {
    let log = this._log;
    log.info("Updating collection keys...");
    
    
    
    
    
    let payload;
    try {
      payload = storage_keys.decrypt(syncKeyBundle);
    } catch (ex) {
      log.warn("Got exception \"" + ex + "\" decrypting storage keys with sync key.");
      log.info("Aborting updateContents. Rethrowing.");
      throw ex;
    }

    let r = this.setContents(payload, storage_keys.modified);
    log.info("Collection keys updated.");
    return r;
    }
}




















function KeyBundle(realm, collectionName, keyStr) {
  let realm = realm || PWDMGR_KEYBUNDLE_REALM;
  
  if (keyStr && !keyStr.charAt)
    
    throw "KeyBundle given non-string key.";
  
  Identity.call(this, realm, collectionName, keyStr);
  this._hmac    = null;
  this._encrypt = null;
  
  
  this._hmacObj = null;
}

KeyBundle.prototype = {
  __proto__: Identity.prototype,
  
  


  get encryptionKey() {
    return this._encrypt;
  },
  
  set encryptionKey(value) {
    this._encrypt = value;
  },

  get hmacKey() {
    return this._hmac;
  },
  
  set hmacKey(value) {
    this._hmac = value;
    this._hmacObj = value ? Utils.makeHMACKey(value) : null;
  },
  
  get hmacKeyObject() {
    return this._hmacObj;
  },
}

function BulkKeyBundle(realm, collectionName) {
  let log = Log4Moz.repository.getLogger("BulkKeyBundle");
  log.info("BulkKeyBundle being created for " + collectionName);
  KeyBundle.call(this, realm, collectionName);
}

BulkKeyBundle.prototype = {
  __proto__: KeyBundle.prototype,
   
  generateRandom: function generateRandom() {
    let generatedHMAC = Svc.Crypto.generateRandomKey();
    let generatedEncr = Svc.Crypto.generateRandomKey();
    this.keyPair = [generatedEncr, generatedHMAC];
  },
  
  get keyPair() {
    return [this._encrypt, btoa(this._hmac)];
  },
  
  



  set keyPair(value) {
    if (value.length && (value.length == 2)) {
      let json = JSON.stringify(value);
      let en = value[0];
      let hm = value[1];
      
      this.password = json;
      this.hmacKey  = Utils.safeAtoB(hm);
      this._encrypt = en;          
    }
    else {
      throw "Invalid keypair";
  }
  },
};

function SyncKeyBundle(realm, collectionName, syncKey) {
  let log = Log4Moz.repository.getLogger("SyncKeyBundle");
  log.info("SyncKeyBundle being created for " + collectionName);
  KeyBundle.call(this, realm, collectionName, syncKey);
  if (syncKey)
    this.keyStr = syncKey;      
} 

SyncKeyBundle.prototype = {
  __proto__: KeyBundle.prototype,

  



  get keyStr() {
    return this.password;
  },

  set keyStr(value) {
    this.password = value;
    this._hmac    = null;
    this._hmacObj = null;
    this._encrypt = null;
    this.generateEntry();
  },
  
  






  get encryptionKey() {
    if (!this._encrypt)
      this.generateEntry();
    return this._encrypt;
  },
  
  get hmacKey() {
    if (!this._hmac)
      this.generateEntry();
    return this._hmac;
  },
  
  get hmacKeyObject() {
    if (!this._hmacObj)
      this.generateEntry();
    return this._hmacObj;
  },
  
  


  generateEntry: function generateEntry() {
    let m = this.keyStr;
    if (m) {
      
      m = Utils.decodeKeyBase32(m);
      
      
      let h = Utils.makeHMACHasher();
      
      
      let u = this.username; 
      let k1 = Utils.makeHMACKey("" + HMAC_INPUT + u + "\x01");
      let enc = Utils.sha256HMACBytes(m, k1, h);
      
      
      let k2 = Utils.makeHMACKey(enc + HMAC_INPUT + u + "\x02");
      let hmac = Utils.sha256HMACBytes(m, k2, h);
      
      
      this._encrypt = btoa(enc);
      
      
      this._hmac = hmac;
      this._hmacObj = Utils.makeHMACKey(hmac);
    }
  }
};
