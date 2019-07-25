



































const EXPORTED_SYMBOLS = ['Resource', 'RemoteStore'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
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
  this._identity = null; 
  this._dav = null; 
  this._path = path;
  this._data = null;
  this._downloaded = false;
  this._dirty = false;
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

  get downloaded() { return this._downloaded; },
  get dirty() { return this._dirty; },

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
    let ret;

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
      ret = yield;

      if (action == "DELETE" &&
          Utils.checkStatus(ret.status, null, [[200,300],404])) {
        this._dirty = false;
        this._data = null;
        break;

      } else if (Utils.checkStatus(ret.status)) {
        this._dirty = false;
        if (action == "GET")
          this._data = ret.responseText;
        else if (action == "PUT")
          this._data = data;
        break;

      } else if (action == "GET" && ret.status == 404) {
        throw new RequestException(this, action, ret);

      } else if (iter >= 10) {
        
        throw new RequestException(this, action, ret);

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

    self.done(ret);
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

function JsonResource(path) {

}
JsonResource.prototype = {
  __proto__: new Resource(),

  _get: function(onComplete) {
    let self = yield;
    this.__proto__.get(onComplete);
      yield;
  },
  get: function JRes_get(onComplete) {
    foo.async();
  }
};

function RemoteStore(serverPrefix) {
  this._prefix = serverPrefix;
  this._status = new Resource(serverPrefix + "status.json");
  this._keys = new Resource(serverPrefix + "keys.json");
  this._snapshot = new Resource(serverPrefix + "snapshot.json");
  this._deltas = new Resource(serverPrefix + "deltas.json");
}
RemoteStore.prototype = {
  get status() {
    return this._status;
  },
  get keys() {
    return this._keys;
  },
  get snapshot() {
    return this._snapshot;
  },
  get deltas() {
    return this._deltas;
  }
};
