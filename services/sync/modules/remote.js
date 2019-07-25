



































const EXPORTED_SYMBOLS = ['Resource', 'JsonFilter', 'CryptoFilter',
                          'Keychain', 'RemoteStore'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/dav.js");
Cu.import("resource://weave/stores.js");

Function.prototype.async = Async.sugar;


function RequestException(resource, action, request) {
  this._resource = resource;
  this._action = action;
  this._request = request;
  this.location = Components.stack.caller;
}
RequestException.prototype = {
  get resource() { return this._resource; },
  get action() { return this._action; },
  get request() { return this._request; },
  get status() { return this._request.status; },
  toString: function ReqEx_toString() {
    return "Could not " + this._action + " resource " + this._resource.path +
      " (" + this._request.status + ")";
  }
};

function Resource(path) {
  this._init(path);
}
Resource.prototype = {
  get identity() { return this._identity; },
  set identity(value) { this._identity = value; },

  get dav() { return this._dav; },
  set dav(value) { this._dav = value; },

  get path() { return this._path; },
  set path(value) {
    this._dirty = true;
    this._path = value;
  },

  get data() { return this._data; },
  set data(value) {
    this._dirty = true;
    this._data = value;
  },

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  get lastRequest() { return this._lastRequest; },
  get downloaded() { return this._downloaded; },
  get dirty() { return this._dirty; },

  pushFilter: function Res_pushFilter(filter) {
    this._filters.push(filter);
  },

  popFilter: function Res_popFilter() {
    return this._filters.pop();
  },

  clearFilters: function Res_clearFilters() {
    this._filters = [];
  },

  _init: function Res__init(path) {
    this._identity = null; 
    this._dav = null; 
    this._path = path;
    this._data = null;
    this._downloaded = false;
    this._dirty = false;
    this._filters = [];
    this._lastRequest = null;
    this._log = Log4Moz.Service.getLogger("Service.Resource");
  },

  
  _sync: function Res__sync() {
    let self = yield;
    let ret;

    
    
    

    if (this.dirty) {
      this.put(self.cb);
      ret = yield;

    } else if (!this.downloaded) {
      this.get(self.cb);
      ret = yield;
    }

    self.done(ret);
  },
  sync: function Res_sync(onComplete) {
    this._sync.async(this, onComplete);
  },

  _request: function Res__request(action, data) {
    let self = yield;
    let listener, timer;
    let iter = 0;

    if ("PUT" == action) {
      for each (let filter in this._filters) {
        data = yield filter.beforePUT.async(filter, self.cb, data);
      }
    }

    while (true) {
      switch (action) {
      case "GET":
        DAV.GET(this.path, self.cb);
        break;
      case "PUT":
        DAV.PUT(this.path, data, self.cb);
        break;
      case "DELETE":
        DAV.DELETE(this.path, self.cb);
        break;
      default:
        throw "Unknown request action for Resource";
      }
      this._lastRequest = yield;

      if (action == "DELETE" &&
          Utils.checkStatus(this._lastRequest.status, null, [[200,300],404])) {
        this._dirty = false;
        this._data = null;
        break;

      } else if (Utils.checkStatus(this._lastRequest.status)) {
        this._log.debug(action + " request successful");
        this._dirty = false;
        if (action == "GET")
          this._data = this._lastRequest.responseText;
        else if (action == "PUT")
          this._data = data;
        break;

      } else if (action == "GET" && this._lastRequest.status == 404) {
        throw new RequestException(this, action, this._lastRequest);

      } else if (iter >= 10) {
        
        throw new RequestException(this, action, this._lastRequest);

      } else {
        
        if (!timer) {
          listener = new Utils.EventListener(self.cb);
          timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        }
        yield timer.initWithCallback(listener, iter * iter * 1000,
                                     timer.TYPE_ONE_SHOT);
        iter++;
      }
    }

    if ("GET" == action) {
      let filters = this._filters.slice(); 
      for each (let filter in filters.reverse()) {
        this._data = yield filter.afterGET.async(filter, self.cb, this._data);
      }
    }

    self.done(this._data);
  },

  get: function Res_get(onComplete) {
    this._request.async(this, onComplete, "GET");
  },

  put: function Res_put(onComplete, data) {
    if ("undefined" == typeof(data))
      data = this._data;
    this._request.async(this, onComplete, "PUT", data);
  },

  delete: function Res_delete(onComplete) {
    this._request.async(this, onComplete, "DELETE");
  }
};

function ResourceSet(basePath) {
  this._init(basePath);
}
ResourceSet.prototype = {
  __proto__: new Resource(),
  _init: function ResSet__init(basePath) {
    this.__proto__.__proto__._init.call(this);
    this._basePath = basePath;
    this._log = Log4Moz.Service.getLogger("Service.ResourceSet");
  },

  _hack: function ResSet__hack(action, id, data) {
    let self = yield;
    let savedData = this._data;

    if ("PUT" == action)
      this._data = data;

    this._path = this._basePath + id;
    yield this._request.async(this, self.cb, action, data);

    let newData = this._data;
    this._data = savedData;
    if (this._data == null)
      this._data = {};
    this._data[id] = newData;

    self.done(this._data[id]);
  },

  get: function ResSet_get(onComplete, id) {
    this._hack.async(this, onComplete, "GET", id);
  },

  put: function ResSet_put(onComplete, id, data) {
    this._hack.async(this, onComplete, "PUT", id, data);
  },

  delete: function ResSet_delete(onComplete, id) {
    this._hack.async(this, onComplete, "DELETE", id);
  }
};

function ResourceFilter() {
  this._log = Log4Moz.Service.getLogger("Service.ResourceFilter");
}
ResourceFilter.prototype = {
  beforePUT: function ResFilter_beforePUT(data) {
    let self = yield;
    this._log.debug("Doing absolutely nothing")
    self.done(data);
  },
  afterGET: function ResFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Doing absolutely nothing")
    self.done(data);
  }
};

function JsonFilter() {
  this._log = Log4Moz.Service.getLogger("Service.JsonFilter");
}
JsonFilter.prototype = {
  __proto__: new ResourceFilter(),

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return this._json;
  },

  beforePUT: function JsonFilter_beforePUT(data) {
    let self = yield;
    this._log.debug("Encoding data as JSON");
    this._os.notifyObservers(null, "weave:service:sync:status", "stats.encoding-json");
    self.done(this._json.encode(data));
  },

  afterGET: function JsonFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Decoding JSON data");
    this._os.notifyObservers(null, "weave:service:sync:status", "stats.decoding-json");
    self.done(this._json.decode(data));
  }
};

function CryptoFilter(identity) {
  this._identity = identity;
  this._log = Log4Moz.Service.getLogger("Service.CryptoFilter");
}
CryptoFilter.prototype = {
  __proto__: new ResourceFilter(),

  get _os() {
    let os = Cc["@mozilla.org/observer-service;1"].
      getService(Ci.nsIObserverService);
    this.__defineGetter__("_os", function() os);
    return os;
  },

  beforePUT: function CryptoFilter_beforePUT(data) {
    let self = yield;
    this._log.debug("Encrypting data");
    this._os.notifyObservers(null, "weave:service:sync:status", "status.encrypting");
    let ret = yield Crypto.encryptData.async(Crypto, self.cb, data, this._identity);
    self.done(ret);
  },

  afterGET: function CryptoFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Decrypting data");
    this._os.notifyObservers(null, "weave:service:sync:status", "status.decrypting");
    let ret = yield Crypto.decryptData.async(Crypto, self.cb, data, this._identity);
    self.done(ret);
  }
};

function Keychain(prefix) {
  this._init(prefix);
}
Keychain.prototype = {
  __proto__: new Resource(),
  _init: function Keychain__init(prefix) {
    this.__proto__.__proto__._init.call(this, prefix + "keys.json");
    this.pushFilter(new JsonFilter());
  },
  _initialize: function Keychain__initialize(identity) {
    let self = yield;
    let wrappedSymkey;

    if ("none" != Utils.prefs.getCharPref("encryption")) {
      this._os.notifyObservers(null, "weave:service:sync:status", "status.generating-random-key");

      yield Crypto.randomKeyGen.async(Crypto, self.cb, identity);

      
      let idRSA = ID.get('WeaveCryptoID');
      this._os.notifyObservers(null, "weave:service:sync:status", "status.encrypting-key");
      wrappedSymkey = yield Crypto.wrapKey.async(Crypto, self.cb,
                                                 identity.bulkKey, idRSA);
    }

    let keys = {ring: {}, bulkIV: identity.bulkIV};
    this._os.notifyObservers(null, "weave:service:sync:status", "status.uploading-key");
    keys.ring[identity.username] = wrappedSymkey;
    yield this.put(self.cb, keys);
  },
  initialize: function Keychain_initialize(onComplete, identity) {
    this._initialize.async(this, onComplete, identity);
  },
  _getKeyAndIV: function Keychain__getKeyAndIV(identity) {
    let self = yield;

    this._os.notifyObservers(null, "weave:service:sync:status", "status.downloading-keyring");

    yield this.get(self.cb);

    if (!this.data || !this.data.ring || !this.data.ring[identity.username])
      throw "Keyring does not contain a key for this user";

    
    this._os.notifyObservers(null, "weave:service:sync:status", "status.decrypting-key");
    let idRSA = ID.get('WeaveCryptoID');
    let symkey = yield Crypto.unwrapKey.async(Crypto, self.cb,
                           this.data.ring[identity.username], idRSA);
    let iv = this.data.bulkIV;

    identity.bulkKey = symkey;
    identity.bulkIV  = iv;
  },
  getKeyAndIV: function Keychain_getKeyAndIV(onComplete, identity) {
    this._getKeyAndIV.async(this, onComplete, identity);
  }
  
};

function RemoteStore(engine) {
  this._engine = engine;
  this._log = Log4Moz.Service.getLogger("Service.RemoteStore");
}
RemoteStore.prototype = {
  get serverPrefix() this._engine.serverPrefix,
  get engineId() this._engine.engineId,

  get status() {
    let status = new Resource(this.serverPrefix + "status.json");
    status.pushFilter(new JsonFilter());
    this.__defineGetter__("status", function() status);
    return status;
  },

  get keys() {
    let keys = new Keychain(this.serverPrefix);
    this.__defineGetter__("keys", function() keys);
    return keys;
  },

  get _snapshot() {
    let snapshot = new Resource(this.serverPrefix + "snapshot.json");
    snapshot.pushFilter(new JsonFilter());
    snapshot.pushFilter(new CryptoFilter(this._engine.engineId));
    this.__defineGetter__("_snapshot", function() snapshot);
    return snapshot;
  },

  get _deltas() {
    let deltas = new ResourceSet(this.serverPrefix + "deltas/");
    deltas.pushFilter(new JsonFilter());
    deltas.pushFilter(new CryptoFilter(this._engine.engineId));
    this.__defineGetter__("_deltas", function() deltas);
    return deltas;
  },

  _openSession: function RStore__openSession() {
    let self = yield;

    if (!this.serverPrefix || !this.engineId)
      throw "Cannot initialize RemoteStore: engine has no server prefix or crypto ID";

    this.status.data = null;
    this.keys.data = null;
    this._snapshot.data = null;
    this._deltas.data = null;

    let ret = yield DAV.MKCOL(this.serverPrefix + "deltas", self.cb);
    if (!ret)
      throw "Could not create remote folder";

    this._log.debug("Downloading status file");
    this._os.notifyObservers(null, "weave:service:sync:status", "status.downloading-status");

    yield this.status.get(self.cb);
    this._log.debug("Downloading status file... done");

    
    if (this.status.data.formatVersion > ENGINE_STORAGE_FORMAT_VERSION) {
      this._log.error("Server uses storage format v" +
                      this.status.data.formatVersion +
                      ", this client understands up to v" +
                      ENGINE_STORAGE_FORMAT_VERSION);
      throw "Incompatible remote store format";
    }
  },
  openSession: function RStore_openSession(onComplete) {
    this._openSession.async(this, onComplete);
  },

  closeSession: function RStore_closeSession() {
    this.status.data = null;
    this.keys.data = null;
    this._snapshot.data = null;
    this._deltas.data = null;
  },

  
  
  _initialize: function RStore__initialize(snapshot) {
    let self = yield;

    yield this.keys.initialize(self.cb, this.engineId);
    this._os.notifyObservers(null, "weave:service:sync:status", "status.uploading-snapshot");
    yield this._snapshot.put(self.cb, snapshot.data);
    

    let c = 0;
    for (GUID in snapshot.data)
      c++;

    this._os.notifyObservers(null, "weave:service:sync:status", "status.uploading-status");
    yield this.status.put(self.cb,
                          {GUID: snapshot.GUID,
                           formatVersion: ENGINE_STORAGE_FORMAT_VERSION,
                           snapVersion: snapshot.version,
                           maxVersion: snapshot.version,
                           snapEncryption: Crypto.defaultAlgorithm,
                           deltasEncryption: Crypto.defaultAlgorithm,
                           itemCount: c});
    this._log.info("Full upload to server successful");
  },
  initialize: function RStore_initialize(onComplete, snapshot) {
    this._initialize.async(this, onComplete, snapshot);
  },

  
  _wipe: function Engine__wipe() {
    let self = yield;
    this._log.debug("Deleting remote store data");
    yield this.status.delete(self.cb);
    yield this.keys.delete(self.cb);
    yield this._snapshot.delete(self.cb);
    
    this._log.debug("Server files deleted");
  },
  wipe: function Engine_wipe(onComplete) {
    this._wipe.async(this, onComplete)
  },

  
  
  _getLatestFromScratch: function RStore__getLatestFromScratch() {
    let self = yield;
    let status = this.status.data;

    this._log.info("Downloading all server data from scratch");
    this._os.notifyObservers(null, "weave:service:sync:status", "status.downloading-snapshot");

    let snap = new SnapshotStore();
    snap.data = yield this._snapshot.get(self.cb);
    snap.version = status.maxVersion;

    for (let id = status.snapVersion + 1; id <= status.maxVersion; id++) {
      let delta = yield this._deltas.get(self.cb, id);
      yield snap.applyCommands.async(snap, self.cb, delta);
    }

    self.done(snap);
  },

  
  
  _getLatestFromSnap: function RStore__getLatestFromSnap(lastSyncSnap) {
    let self = yield;
    let deltas, snap = new SnapshotStore();
    snap.version = this.status.data.maxVersion;

    if (lastSyncSnap.version < this.status.data.snapVersion) {
      this._log.trace("Getting latest from snap --> scratch");
      snap = yield this._getLatestFromScratch.async(this, self.cb);
      self.done(snap);
      return;

    } else if (lastSyncSnap.version >= this.status.data.snapVersion &&
               lastSyncSnap.version < this.status.data.maxVersion) {
      this._log.debug("Using last sync snapshot as starting point for server snapshot");
      snap.data = Utils.deepCopy(lastSyncSnap.data);
      this._log.info("Downloading server deltas");
      this._os.notifyObservers(null, "weave:service:sync:status", "status.downloading-deltas");
      deltas = [];
      let min = lastSyncSnap.version + 1;
      let max = this.status.data.maxVersion;
      for (let id = min; id <= max; id++) {
        let delta = yield this._deltas.get(self.cb, id);
        deltas.push(delta);
      }

    } else if (lastSyncSnap.version == this.status.data.maxVersion) {
      this._log.debug("Using last sync snapshot as server snapshot (snap version == max version)");
      this._log.trace("Local snapshot version == server maxVersion");
      snap.data = Utils.deepCopy(lastSyncSnap.data);
      deltas = [];

    } else { 
      this._log.error("Server snapshot is older than local snapshot");
      throw "Server snapshot is older than local snapshot";
    }

    try {
      for (var i = 0; i < deltas.length; i++) {
        yield snap.applyCommands.async(snap, self.cb, deltas[i]);
      }
    } catch (e) {
      this._log.warn("Error applying remote deltas to saved snapshot, attempting a full download");
      this._log.debug("Exception: " + Utils.exceptionStr(e));
      this._log.trace("Stack:\n" + Utils.stackTrace(e));
      snap = yield this._getLatestFromScratch.async(this, self.cb);
    }

    self.done(snap);
  },

  
  
  _wrap: function RStore__wrap(snapshot) {
    let self = yield;
    if (snapshot)
      self.done(yield this._getLatestFromSnap.async(this, self.cb, snapshot));
    else
      self.done(yield this._getLatestFromScratch.async(this, self.cb));
  },
  wrap: function RStore_wrap(onComplete, snapshot) {
    this._wrap.async(this, onComplete, snapshot);
  },

  
  _appendDelta: function RStore__appendDelta(snapshot, delta, metadata) {
    let self = yield;

    if (metadata) {
      for (let key in metadata)
        this.status.data[key] = metadata[key];
    }

    
    let id = this.status.data.maxVersion;

    
    
    this._os.notifyObservers(null, "weave:service:sync:status",
                             "status.uploading-deltas");
    yield this._deltas.put(self.cb, id, delta);

    
    
    if ((id - this.status.data.snapVersion) > KEEP_DELTAS) {
      this._os.notifyObservers(null, "weave:service:sync:status",
                               "status.uploading-snapshot");
      yield this.snapshot.put(self.cb, snapshot.data);
      this.status.data.snapVersion = id;
    }

    
    
    
    

    
    this._os.notifyObservers(null, "weave:service:sync:status",
                             "status.uploading-status");
    yield this.status.put(self.cb);
  },
  appendDelta: function RStore_appendDelta(onComplete, snapshot, delta, metadata) {
    this._appendDelta.async(this, onComplete, snapshot, delta, metadata);
  }
};
