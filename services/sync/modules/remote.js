



































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
      for each (let filter in this._filters.reverse()) {
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
    this._log.debug("Encoding data as JSON")
    self.done(this._json.encode(data));
  },

  afterGET: function JsonFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Decoding JSON data")
    self.done(this._json.decode(data));
  }
};


function PBECryptoFilter(identity) {
  this._identity = identity;
  this._log = Log4Moz.Service.getLogger("Service.PBECryptoFilter");
}
PBECryptoFilter.prototype = {
  __proto__: new ResourceFilter(),

  beforePUT: function PBEFilter_beforePUT(data) {
    let self = yield;
    this._log.debug("Encrypting data")
    Crypto.PBEencrypt.async(Crypto, self.cb, data, this._identity);
    let ret = yield;
    self.done(ret);
  },

  afterGET: function PBEFilter_afterGET(data) {
    let self = yield;
    this._log.debug("Decrypting data")
    Crypto.PBEdecrypt.async(Crypto, self.cb, data, this._identity);
    let ret = yield;
    self.done(ret);
  }
};

function RemoteStore(serverPrefix, cryptoId) {
  this._prefix = serverPrefix;
  this._cryptoId = cryptoId;
  this._init();
}
RemoteStore.prototype = {
  _init: function Remote__init(serverPrefix, cryptoId) {
    if (!this._prefix || !this._cryptoId)
      return;
    let json = new JsonFilter();
    let crypto = new PBECryptoFilter(this._cryptoId);
    this._status = new Resource(this._prefix + "status.json");
    this._status.pushFilter(json);
    this._keys = new Resource(this._prefix + "keys.json");
    this._keys.pushFilter(new JsonFilter());
    this._snapshot = new Resource(this._prefix + "snapshot.json");
    this._snapshot.pushFilter(json);
    this._snapshot.pushFilter(crypto);
    this._deltas = new Resource(this._prefix + "deltas.json");
    this._deltas.pushFilter(json);
    this._deltas.pushFilter(crypto);
  },

  get status() this._status,
  get keys() this._keys,
  get snapshot() this._snapshot,
  get deltas() this._deltas,

  get serverPrefix() this._prefix,
  set serverPrefix(value) {
    this._prefix = value;
    this._init();
  },

  get cryptoId() this._cryptoId,
  set cryptoId(value) {
    this._cryptoId = value;
    this._init();
  }
};
