



this.EXPORTED_SYMBOLS = [
  "WBORecord",
  "RecordManager",
  "CryptoWrapper",
  "CollectionKeyManager",
  "Collection",
];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const CRYPTO_COLLECTION = "crypto";
const KEYS_WBO = "keys";

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/keys.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/util.js");

this.WBORecord = function WBORecord(collection, id) {
  this.data = {};
  this.payload = {};
  this.collection = collection;      
  this.id = id;                      
}
WBORecord.prototype = {
  _logName: "Sync.Record.WBO",

  get sortindex() {
    if (this.data.sortindex)
      return this.data.sortindex;
    return 0;
  },

  
  
  fetch: function fetch(resource) {
    if (!resource instanceof Resource) {
      throw new Error("First argument must be a Resource instance.");
    }

    let r = resource.get();
    if (r.success) {
      this.deserialize(r);   
    }
    this.response = r;
    return this;
  },

  upload: function upload(resource) {
    if (!resource instanceof Resource) {
      throw new Error("First argument must be a Resource instance.");
    }

    return resource.put(this);
  },

  
  
  uri: function(base) {
    if (this.collection && this.id) {
      let url = Utils.makeURI(base + this.collection + "/" + this.id);
      url.QueryInterface(Ci.nsIURL);
      return url;
    }
    return null;
  },

  deserialize: function deserialize(json) {
    this.data = json.constructor.toString() == String ? JSON.parse(json) : json;

    try {
      
      this.payload = JSON.parse(this.payload);
    } catch(ex) {}
  },

  toJSON: function toJSON() {
    
    let obj = {};
    for (let [key, val] in Iterator(this.data))
      obj[key] = key == "payload" ? JSON.stringify(val) : val;
    if (this.ttl)
      obj.ttl = this.ttl;
    return obj;
  },

  toString: function toString() {
    return "{ " +
      "id: "       + this.id        + "  " +
      "index: "    + this.sortindex + "  " +
      "modified: " + this.modified  + "  " +
      "ttl: "      + this.ttl       + "  " +
      "payload: "  + JSON.stringify(this.payload) +
      " }";
  }
};

Utils.deferGetSet(WBORecord, "data", ["id", "modified", "sortindex", "payload"]);

this.CryptoWrapper = function CryptoWrapper(collection, id) {
  this.cleartext = {};
  WBORecord.call(this, collection, id);
  this.ciphertext = null;
  this.id = id;
}
CryptoWrapper.prototype = {
  __proto__: WBORecord.prototype,
  _logName: "Sync.Record.CryptoWrapper",

  ciphertextHMAC: function ciphertextHMAC(keyBundle) {
    let hasher = keyBundle.sha256HMACHasher;
    if (!hasher) {
      throw "Cannot compute HMAC without an HMAC key.";
    }

    return Utils.bytesAsHex(Utils.digestUTF8(this.ciphertext, hasher));
  },

  








  encrypt: function encrypt(keyBundle) {
    if (!keyBundle) {
      throw new Error("A key bundle must be supplied to encrypt.");
    }

    this.IV = Svc.Crypto.generateRandomIV();
    this.ciphertext = Svc.Crypto.encrypt(JSON.stringify(this.cleartext),
                                         keyBundle.encryptionKeyB64, this.IV);
    this.hmac = this.ciphertextHMAC(keyBundle);
    this.cleartext = null;
  },

  
  decrypt: function decrypt(keyBundle) {
    if (!this.ciphertext) {
      throw "No ciphertext: nothing to decrypt?";
    }

    if (!keyBundle) {
      throw new Error("A key bundle must be supplied to decrypt.");
    }

    
    let computedHMAC = this.ciphertextHMAC(keyBundle);

    if (computedHMAC != this.hmac) {
      Utils.throwHMACMismatch(this.hmac, computedHMAC);
    }

    
    let cleartext = Svc.Crypto.decrypt(this.ciphertext,
                                       keyBundle.encryptionKeyB64, this.IV);
    let json_result = JSON.parse(cleartext);

    if (json_result && (json_result instanceof Object)) {
      this.cleartext = json_result;
      this.ciphertext = null;
    } else {
      throw "Decryption failed: result is <" + json_result + ">, not an object.";
    }

    
    if (this.cleartext.id != this.id)
      throw "Record id mismatch: " + this.cleartext.id + " != " + this.id;

    return this.cleartext;
  },

  toString: function toString() {
    let payload = this.deleted ? "DELETED" : JSON.stringify(this.cleartext);

    return "{ " +
      "id: "         + this.id          + "  " +
      "index: "      + this.sortindex   + "  " +
      "modified: "   + this.modified    + "  " +
      "ttl: "        + this.ttl         + "  " +
      "payload: "    + payload          + "  " +
      "collection: " + (this.collection || "undefined") +
      " }";
  },

  
  get id() WBORecord.prototype.__lookupGetter__("id").call(this),

  
  set id(val) {
    WBORecord.prototype.__lookupSetter__("id").call(this, val);
    return this.cleartext.id = val;
  },
};

Utils.deferGetSet(CryptoWrapper, "payload", ["ciphertext", "IV", "hmac"]);
Utils.deferGetSet(CryptoWrapper, "cleartext", "deleted");




this.RecordManager = function RecordManager(service) {
  this.service = service;

  this._log = Log.repository.getLogger(this._logName);
  this._records = {};
}
RecordManager.prototype = {
  _recordType: CryptoWrapper,
  _logName: "Sync.RecordManager",

  import: function RecordMgr_import(url) {
    this._log.trace("Importing record: " + (url.spec ? url.spec : url));
    try {
      
      this.response = {};
      this.response = this.service.resource(url).get();

      
      if (!this.response.success)
        return null;

      let record = new this._recordType(url);
      record.deserialize(this.response);

      return this.set(url, record);
    } catch(ex) {
      this._log.debug("Failed to import record: " + Utils.exceptionStr(ex));
      return null;
    }
  },

  get: function RecordMgr_get(url) {
    
    let spec = url.spec ? url.spec : url;
    if (spec in this._records)
      return this._records[spec];
    return this.import(url);
  },

  set: function RecordMgr_set(url, record) {
    let spec = url.spec ? url.spec : url;
    return this._records[spec] = record;
  },

  contains: function RecordMgr_contains(url) {
    if ((url.spec || url) in this._records)
      return true;
    return false;
  },

  clearCache: function recordMgr_clearCache() {
    this._records = {};
  },

  del: function RecordMgr_del(url) {
    delete this._records[url];
  }
};







this.CollectionKeyManager = function CollectionKeyManager() {
  this.lastModified = 0;
  this._collections = {};
  this._default = null;

  this._log = Log.repository.getLogger("Sync.CollectionKeyManager");
}



CollectionKeyManager.prototype = {

  
  
  
  _compareKeyBundleCollections: function _compareKeyBundleCollections(m1, m2) {
    let changed = [];

    function process(m1, m2) {
      for (let k1 in m1) {
        let v1 = m1[k1];
        let v2 = m2[k1];
        if (!(v1 && v2 && v1.equals(v2)))
          changed.push(k1);
      }
    }

    
    process(m1, m2);
    process(m2, m1);

    
    changed.sort();
    let last;
    changed = [x for each (x in changed) if ((x != last) && (last = x))];
    return {same: changed.length == 0,
            changed: changed};
  },

  get isClear() {
   return !this._default;
  },

  clear: function clear() {
    this._log.info("Clearing collection keys...");
    this.lastModified = 0;
    this._collections = {};
    this._default = null;
  },

  keyForCollection: function(collection) {
    if (collection && this._collections[collection])
      return this._collections[collection];

    return this._default;
  },

  




  _makeWBO: function(collections, defaultBundle) {
    let wbo = new CryptoWrapper(CRYPTO_COLLECTION, KEYS_WBO);
    let c = {};
    for (let k in collections) {
      c[k] = collections[k].keyPairB64;
    }
    wbo.cleartext = {
      "default":     defaultBundle ? defaultBundle.keyPairB64 : null,
      "collections": c,
      "collection":  CRYPTO_COLLECTION,
      "id":          KEYS_WBO
    };
    return wbo;
  },

  


  asWBO: function(collection, id)
    this._makeWBO(this._collections, this._default),

  


  newKeys: function(collections) {
    let newDefaultKey = new BulkKeyBundle(DEFAULT_KEYBUNDLE_NAME);
    newDefaultKey.generateRandom();

    let newColls = {};
    if (collections) {
      collections.forEach(function (c) {
        let b = new BulkKeyBundle(c);
        b.generateRandom();
        newColls[c] = b;
      });
    }
    return [newDefaultKey, newColls];
  },

  



  generateNewKeysWBO: function(collections) {
    let newDefaultKey, newColls;
    [newDefaultKey, newColls] = this.newKeys(collections);

    return this._makeWBO(newColls, newDefaultKey);
  },

  
  
  updateNeeded: function(info_collections) {

    this._log.info("Testing for updateNeeded. Last modified: " + this.lastModified);

    
    if (!this.lastModified)
      return true;

    
    
    if (!(CRYPTO_COLLECTION in info_collections))
      return true;

    
    return (info_collections[CRYPTO_COLLECTION] > this.lastModified);
  },

  
  
  
  
  
  
  
  
  
  setContents: function setContents(payload, modified) {

    if (!modified)
      throw "No modified time provided to setContents.";

    let self = this;

    this._log.info("Setting collection keys contents. Our last modified: " +
                   this.lastModified + ", input modified: " + modified + ".");

    if (!payload)
      throw "No payload in CollectionKeyManager.setContents().";

    if (!payload.default) {
      this._log.warn("No downloaded default key: this should not occur.");
      this._log.warn("Not clearing local keys.");
      throw "No default key in CollectionKeyManager.setContents(). Cannot proceed.";
    }

    
    let b = new BulkKeyBundle(DEFAULT_KEYBUNDLE_NAME);
    b.keyPairB64 = payload.default;
    let newDefault = b;

    
    let newCollections = {};
    if ("collections" in payload) {
      this._log.info("Processing downloaded per-collection keys.");
      let colls = payload.collections;
      for (let k in colls) {
        let v = colls[k];
        if (v) {
          let keyObj = new BulkKeyBundle(k);
          keyObj.keyPairB64 = v;
          if (keyObj) {
            newCollections[k] = keyObj;
          }
        }
      }
    }

    
    let sameDefault = (this._default && this._default.equals(newDefault));
    let collComparison = this._compareKeyBundleCollections(newCollections, this._collections);
    let sameColls = collComparison.same;

    if (sameDefault && sameColls) {
      self._log.info("New keys are the same as our old keys! Bumped local modified time.");
      self.lastModified = modified;
      return false;
    }

    
    this.clear();

    this._log.info("Saving downloaded keys.");
    this._default     = newDefault;
    this._collections = newCollections;

    
    self._log.info("Bumping last modified to " + modified);
    self.lastModified = modified;

    return sameDefault ? collComparison.changed : true;
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

this.Collection = function Collection(uri, recordObj, service) {
  if (!service) {
    throw new Error("Collection constructor requires a service.");
  }

  Resource.call(this, uri);

  
  let res = service.resource(uri);
  this.authenticator = res.authenticator;

  this._recordObj = recordObj;
  this._service = service;

  this._full = false;
  this._ids = null;
  this._limit = 0;
  this._older = 0;
  this._newer = 0;
  this._data = [];
}
Collection.prototype = {
  __proto__: Resource.prototype,
  _logName: "Sync.Collection",

  _rebuildURL: function Coll__rebuildURL() {
    
    this.uri.QueryInterface(Ci.nsIURL);

    let args = [];
    if (this.older)
      args.push('older=' + this.older);
    else if (this.newer) {
      args.push('newer=' + this.newer);
    }
    if (this.full)
      args.push('full=1');
    if (this.sort)
      args.push('sort=' + this.sort);
    if (this.ids != null)
      args.push("ids=" + this.ids);
    if (this.limit > 0 && this.limit != Infinity)
      args.push("limit=" + this.limit);

    this.uri.query = (args.length > 0)? '?' + args.join('&') : '';
  },

  
  get full() { return this._full; },
  set full(value) {
    this._full = value;
    this._rebuildURL();
  },

  
  get ids() this._ids,
  set ids(value) {
    this._ids = value;
    this._rebuildURL();
  },

  
  get limit() this._limit,
  set limit(value) {
    this._limit = value;
    this._rebuildURL();
  },

  
  get older() { return this._older; },
  set older(value) {
    this._older = value;
    this._rebuildURL();
  },

  
  get newer() { return this._newer; },
  set newer(value) {
    this._newer = value;
    this._rebuildURL();
  },

  
  
  
  
  get sort() { return this._sort; },
  set sort(value) {
    this._sort = value;
    this._rebuildURL();
  },

  pushData: function Coll_pushData(data) {
    this._data.push(data);
  },

  clearRecords: function Coll_clearRecords() {
    this._data = [];
  },

  set recordHandler(onRecord) {
    
    let coll = this;

    
    coll.setHeader("Accept", "application/newlines");

    this._onProgress = function() {
      let newline;
      while ((newline = this._data.indexOf("\n")) > 0) {
        
        let json = this._data.slice(0, newline);
        this._data = this._data.slice(newline + 1);

        
        let record = new coll._recordObj();
        record.deserialize(json);
        onRecord(record);
      }
    };
  },
};
