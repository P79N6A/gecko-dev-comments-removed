




































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
  this._log = Log4Moz.Service.getLogger("Service.ClientData");
}
ClientDataSvc.prototype = {
  getClientData: function ClientData_getClientData() {
    let self = yield;

    DAV.MKCOL("meta", self.cb);
    let ret = yield;
    if(!ret)
      throw "Could not create meta information directory";

    DAV.GET("meta/clients", self.cb);
    let ret = yield;

    if (Utils.checkStatus(ret.status)) {
      this._log.debug("Could not get clients file from server");
      self.done(false);
      return;
    }

    this._ClientData = this._json.decode(ret.responseText);

    this._log.debug("Successfully downloaded clients file from server");
    self.done(true);
  },

  uploadClientData: function ClientData_uploadClientData() {
    let self = yield;
    let json = this._json.encode(this._ClientData);

    DAV.MKCOL("meta", self.cb);
    let ret = yield;
    if(!ret)
      throw "Could not create meta information directory";

    DAV.PUT("meta/clients", json, self.cb);
    let ret = yield;

    if(Utils.checkStatus(ret.status)) {
      this._log.debug("Could not upload clients file from server");
      self.done(false);
      return;
    }

    this._log.debug("Successfully uploaded clients file to server");
    self.done(true);
  }
};
