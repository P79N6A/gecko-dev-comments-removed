



const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/constants.js");

this.EXPORTED_SYMBOLS = ["SyncStorageRequest"];

const STORAGE_REQUEST_TIMEOUT = 5 * 60; 




this.SyncStorageRequest = function SyncStorageRequest(uri) {
  RESTRequest.call(this, uri);

  this.authenticator = null;
}
SyncStorageRequest.prototype = {

  __proto__: RESTRequest.prototype,

  _logName: "Sync.StorageRequest",

  









  userAgent:
    Services.appinfo.name + "/" + Services.appinfo.version +  
    " FxSync/" + WEAVE_VERSION + "." +                        
    Services.appinfo.appBuildID + ".",                        

  


  timeout: STORAGE_REQUEST_TIMEOUT,

  dispatch: function dispatch(method, data, onComplete, onProgress) {
    
    if (Svc.Prefs.get("sendVersionInfo", true)) {
      let ua = this.userAgent + Svc.Prefs.get("client.type", "desktop");
      this.setHeader("user-agent", ua);
    }

    if (this.authenticator) {
      this.authenticator(this);
    } else {
      this._log.debug("No authenticator found.");
    }

    return RESTRequest.prototype.dispatch.apply(this, arguments);
  },

  onStartRequest: function onStartRequest(channel) {
    RESTRequest.prototype.onStartRequest.call(this, channel);
    if (this.status == this.ABORTED) {
      return;
    }

    let headers = this.response.headers;
    
    if (headers["x-weave-timestamp"]) {
      SyncStorageRequest.serverTime = parseFloat(headers["x-weave-timestamp"]);
    }

    
    
    if (headers["x-weave-backoff"]) {
      Svc.Obs.notify("weave:service:backoff:interval",
                     parseInt(headers["x-weave-backoff"], 10));
    }

    if (this.response.success && headers["x-weave-quota-remaining"]) {
      Svc.Obs.notify("weave:service:quota:remaining",
                     parseInt(headers["x-weave-quota-remaining"], 10));
    }
  },

  onStopRequest: function onStopRequest(channel, context, statusCode) {
    if (this.status != this.ABORTED) {
      let resp = this.response;
      let contentLength = resp.headers ? resp.headers["content-length"] : "";

      if (resp.success && contentLength &&
          contentLength != resp.body.length) {
        this._log.warn("The response body's length of: " + resp.body.length +
                       " doesn't match the header's content-length of: " +
                       contentLength + ".");
      }
    }

    RESTRequest.prototype.onStopRequest.apply(this, arguments);
  }
};
