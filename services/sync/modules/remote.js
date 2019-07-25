



































const EXPORTED_SYMBOLS = ['Resource', 'RemoteStore'];

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
      this.put(self.cb, this.data);
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
        filter.beforePUT.async(filter, self.cb, data);
        data = yield;
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
        timer.initWithCallback(listener, iter * 100, timer.TYPE_ONE_SHOT);
        yield;
        iter++;
      }
    }

    if ("GET" == action) {
      let filters = this._filters.slice(); 
      for each (let filter in filters.reverse()) {
        filter.afterGET.async(filter, self.cb, this._data);
        this._data = yield;
      }
    }

    self.done(this._data);
  },

  get: function Res_get(onComplete) {
    this._request.async(this, onComplete, "GET");
  },

  put: function Res_put(onComplete, data) {
    this._request.async(this, onComplete, "PUT", data);
  },

  delete: function Res_delete(onComplete) {
    this._request.async(this, onComplete, "DELETE");
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

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return this._json;
  },

  beforePUT: function JsonFilter_beforePUT(data) {
    let self = yield;
    this._log.debug("Encoding data as JSON");
    self.done(this._json.encode(data));
  },

  afterGET: function JsonFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Decoding JSON data");
    self.done(this._json.decode(data));
  }
};

function CryptoFilter(remoteStore, algProp) {
  this._remote = remoteStore;
  this._algProp = algProp; 
  this._log = Log4Moz.Service.getLogger("Service.CryptoFilter");
}
CryptoFilter.prototype = {
  __proto__: new ResourceFilter(),

  beforePUT: function CryptoFilter_beforePUT(data) {
    let self = yield;
    this._log.debug("Encrypting data");
    Crypto.PBEencrypt.async(Crypto, self.cb, data, ID.get(this._remote.cryptoId));
    let ret = yield;
    self.done(ret);
  },

  afterGET: function CryptoFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Decrypting data");
    if (!this._remote.status.data)
      throw "Remote status must be initialized before crypto filter can be used"
    let alg = this._remote.status.data[this._algProp];
    Crypto.PBEdecrypt.async(Crypto, self.cb, data, ID.get(this._remote.cryptoId));
    let ret = yield;
    self.done(ret);
  }
};

function Status(remoteStore) {
  this._init(remoteStore);
}
Status.prototype = {
  __proto__: new Resource(),
  _init: function Status__init(remoteStore) {
    this._remote = remoteStore;
    this.__proto__.__proto__._init.call(this, this._remote.serverPrefix + "status.json");
    this.pushFilter(new JsonFilter());
  }
};

function Keychain(remoteStore) {
  this._init(remoteStore);
}
Keychain.prototype = {
  __proto__: new Resource(),
  _init: function Keychain__init(remoteStore) {
    this._remote = remoteStore;
    this.__proto__.__proto__._init.call(this, this._remote.serverPrefix + "keys.json");
    this.pushFilter(new JsonFilter());
  },
  _getKey: function Keychain__getKey(identity) {
    let self = yield;

    this.get(self.cb);
    yield;
    if (!this.data || !this.data.ring || !this.data.ring[identity.username])
      throw "Keyring does not contain a key for this user";
    Crypto.RSAdecrypt.async(Crypto, self.cb,
                            this.data.ring[identity.username], identity);
    let symkey = yield;

    self.done(symkey);
  },
  getKey: function Keychain_getKey(onComplete, identity) {
    this._getKey.async(this, onComplete, identity);
  }
  
};

function Snapshot(remoteStore) {
  this._init(remoteStore);
}
Snapshot.prototype = {
  __proto__: new Resource(),
  _init: function Snapshot__init(remoteStore) {
    this._remote = remoteStore;
    this.__proto__.__proto__._init.call(this, this._remote.serverPrefix + "snapshot.json");
    this.pushFilter(new JsonFilter());
    this.pushFilter(new CryptoFilter(remoteStore, "snapshotEncryption"));
  }
};

function Deltas(remoteStore) {
  this._init(remoteStore);
}
Deltas.prototype = {
  __proto__: new Resource(),
  _init: function Deltas__init(remoteStore) {
    this._remote = remoteStore;
    this.__proto__.__proto__._init.call(this, this._remote.serverPrefix + "deltas.json");
    this.pushFilter(new JsonFilter());
    this.pushFilter(new CryptoFilter(remoteStore, "deltasEncryption"));
  }
};

function RemoteStore(serverPrefix, cryptoId) {
  this._prefix = serverPrefix;
  this._cryptoId = cryptoId;
  this._log = Log4Moz.Service.getLogger("Service.RemoteStore");
}
RemoteStore.prototype = {
  get serverPrefix() this._prefix,
  set serverPrefix(value) {
    this._prefix = value;
    this.status.serverPrefix = value;
    this.keys.serverPrefix = value;
    this.snapshot.serverPrefix = value;
    this.deltas.serverPrefix = value;
  },

  get cryptoId() this._cryptoId,
  set cryptoId(value) {
    this.__cryptoId = value;
    
  },

  get status() {
    let status = new Status(this);
    this.__defineGetter__("status", function() status);
    return status;
  },

  get keys() {
    let keys = new Keychain(this);
    this.__defineGetter__("keys", function() keys);
    return keys;
  },

  get snapshot() {
    let snapshot = new Snapshot(this);
    this.__defineGetter__("snapshot", function() snapshot);
    return snapshot;
  },

  get deltas() {
    let deltas = new Deltas(this);
    this.__defineGetter__("deltas", function() deltas);
    return deltas;
  },

  _initSession: function RStore__initSession(serverPrefix, cryptoId) {
    let self = yield;

    if (serverPrefix)
      this.serverPrefix = serverPrefix;
    if (cryptoId)
      this.cryptoId = cryptoId;
    if (!this.serverPrefix || !this.cryptoId)
      throw "RemoteStore: cannot initialize without a server prefix and crypto ID";

    this.status.data = null;
    this.keys.data = null;
    this.snapshot.data = null;
    this.deltas.data = null;

    DAV.MKCOL(this.serverPrefix, self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create remote folder";

    this._log.debug("Downloading status file");
    this.status.get(self.cb);
    yield;
    this._log.debug("Downloading status file... done");

    this._inited = true;
  },
  initSession: function RStore_initSession(onComplete, serverPrefix, cryptoId) {
    this._initSession.async(this, onComplete, serverPrefix, cryptoId);
  },

  closeSession: function RStore_closeSession() {
    this._inited = false;
    this.status.data = null;
    this.keys.data = null;
    this.snapshot.data = null;
    this.deltas.data = null;
  },

  _appendDelta: function RStore__appendDelta(delta) {
    let self = yield;
    if (this.deltas.data == null) {
      this.deltas.get(self.cb);
      yield;
      if (this.deltas.data == null)
        this.deltas.data = [];
    }
    this.deltas.data.push(delta);
    this.deltas.put(self.cb, this.deltas.data);
    yield;
  },
  appendDelta: function RStore_appendDelta(onComplete, delta) {
    this._appendDeltas.async(this, onComplete, delta);
  },

  _getUpdates: function RStore__getUpdates(lastSyncSnap) {
    let self = yield;

  },
  getUpdates: function RStore_getUpdates(onComplete, lastSyncSnap) {
    this._getUpdates.async(this, onComplete);
  }
};
