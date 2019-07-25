




































const EXPORTED_SYMBOLS = ['ClientData'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/dav.js");
Cu.import("resource://weave/remote.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'ClientData', ClientDataSvc);

function ClientDataSvc() {
  this._init();
}
ClientDataSvc.prototype = {
  _getCharPref: function ClientData__getCharPref(pref, defaultCb) {
    let value;
    try {
      value = Utils.prefs.getCharPref(pref);
    } catch (e) {
      value = defaultCb();
      Utils.prefs.setCharPref(pref, value);
    }
    return value;
  },

  get GUID() {
    return this._getCharPref("client.GUID",
                             function() { return Utils.makeGUID(); });
  },
  set GUID(value) {
    Utils.prefs.setCharPref("client.GUID", value);
  },

  get name() {
    return this._getCharPref("client.name",
                             function() { return "cheese"; });
  },
  set GUID(value) {
    Utils.prefs.setCharPref("client.name", value);
  },

  get type() {
    return this._getCharPref("client.type",
                             function() { return "gruyere"; });
  },
  set GUID(value) {
    Utils.prefs.setCharPref("client.type", value);
  },

  _init: function ClientData__init() {
    this._log = Log4Moz.Service.getLogger("Service.ClientData");
    this._remoteFile = new Resource("meta/clients");
    this._remote.pushFilter(new JsonFilter());
  },

  _wrap: function ClientData__wrap() {
    return {
      GUID: this.GUID,
      name: this.name,
      type: this.type
    };
  },

  _refresh: function ClientData__refresh() {
    let self = yield;

    let ret = yield DAV.MKCOL("meta", self.cb);
    if(!ret)
      throw "Could not create meta information directory";

    try {
      yield this._remote.get(self.cb);
    } catch(e if e.status == 404) {
      this._remote.data = {};
    }
    this._remote.data[this.GUID] = this._wrap();
    yield this._remote.put(self.cb);
    this._log.debug("Successfully downloaded clients file from server");
  },
  refresh: function ClientData_refresh() {
    this._refresh.async(this, onComplete);
  }
};
