




































const EXPORTED_SYMBOLS = ['ClientData'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/type_records/clientData.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'ClientData', ClientDataSvc);

function ClientDataSvc() {
  this._init();
}
ClientDataSvc.prototype = {
  name: "clients",

  _init: function ClientData__init() {
    this._log = Log4Moz.repository.getLogger("Service.ClientData");
  },

  get baseURL() {
    let url = Utils.prefs.getCharPref("serverURL");
    if (url && url[url.length-1] != '/')
      url += '/';
    url += "0.3/user/";
    return url;
  },

  get engineURL() {
    return this.baseURL + ID.get('WeaveID').username + '/' + this.name + '/';
  },

  get GUID() {
    return this._getCharPref("client.GUID", function() Utils.makeGUID());
  },
  set GUID(value) {
    Utils.prefs.setCharPref("client.GUID", value);
  },

  get name() {
    return this._getCharPref("client.name", function() "Firefox");
  },
  set GUID(value) {
    Utils.prefs.setCharPref("client.name", value);
  },

  get type() {
    return this._getCharPref("client.type", function() "desktop");
  },
  set GUID(value) {
    Utils.prefs.setCharPref("client.type", value);
  },

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

  refresh: function ClientData_refresh(onComplete) {
    let fn = function() {
      let self = yield;

      let record = new ClientRec();
      record.uri = this.engineURL + this.GUID;
      try {
        yield record.get(self.cb);
      } catch (e) {
        
        
        record = new ClientRec();
        record.uri = this.engineURL + this.GUID;
      }

      record.name = this.name;
      record.type = this.type;
      yield record.put(self.cb);
    };
    fn.async(this, onComplete);
  }
};
