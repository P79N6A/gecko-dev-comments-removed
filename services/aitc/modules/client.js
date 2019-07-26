



"use strict";

const EXPORTED_SYMBOLS = ["AitcClient"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");

const DEFAULT_INITIAL_BACKOFF = 600000; 
const DEFAULT_MAX_BACKOFF = 21600000;   
const DEFAULT_REQUEST_FAILURE_THRESHOLD = 3;














function AitcClient(token, state) {
  this.updateToken(token);

  this._log = Log4Moz.repository.getLogger("Service.AITC.Client");
  this._log.level = Log4Moz.Level[
    Preferences.get("services.aitc.client.log.level")
  ];

  this._state = state;
  this._backoff = !!state.get("backoff", false);
  this._backoffTime = 0;
  this._consecutiveFailures = 0;
  this._maxFailures = state.get("requestFailureThreshold",
    DEFAULT_REQUEST_FAILURE_THRESHOLD);

  this._timeout = state.get("timeout", 120);
  this._appsLastModified = parseInt(state.get("lastModified", "0"), 10);
  this._log.info("Client initialized with token endpoint: " + this.uri);
}
AitcClient.prototype = {
  _requiredLocalKeys: [
    "origin", "receipts", "manifestURL", "installOrigin"
  ],
  _requiredRemoteKeys: [
    "origin", "name", "receipts", "manifestPath", "installOrigin",
    "installedAt", "modifiedAt"
  ],

  





  updateToken: function updateToken(token) {
    this.uri = token.endpoint.replace(/\/+$/, "");
    this.token = {id: token.id, key: token.key};
  },

  






  remoteInstall: function remoteInstall(app, cb) {
    if (!cb) {
      throw new Error("remoteInstall called without callback");
    }

    
    let self = this;
    DOMApplicationRegistry.getManifestFor(app.origin, function gotManifest(m) {
      if (m) {
        app.name = m.name;
      }
      self._putApp(self._makeRemoteApp(app), cb);
    });
  },

  






  remoteUninstall: function remoteUninstall(app, cb) {
    if (!cb) {
      throw new Error("remoteUninstall called without callback");
    }

    app.name = "Uninstalled"; 
    let record = this._makeRemoteApp(app);
    record.hidden = true;
    this._putApp(record, cb);
  },

  




  getApps: function getApps(cb) {
    if (!cb) {
      throw new Error("getApps called but no callback provided");
    }

    if (!this._isRequestAllowed()) {
      cb(null, null);
      return;
    }

    let uri = this.uri + "/apps/?full=1";
    let req = new TokenAuthenticatedRESTRequest(uri, this.token);
    req.timeout = this._timeout;
    req.setHeader("Content-Type", "application/json");

    if (this._appsLastModified) {
      req.setHeader("X-If-Modified-Since", this._appsLastModified);
    }

    let self = this;
    req.get(function _getAppsCb(err) {
      self._processGetApps(err, cb, req);
    });
  },

  


  _processGetApps: function _processGetApps(err, cb, req) {
    
    this._setBackoff(req);

    if (err) {
      this._log.error("getApps request error " + err);
      cb(err, null);
      return;
    }

    
    if (req.response.status == 401) {
      let msg = new Error("getApps failed due to 401 authentication failure");
      this._log.info(msg);
      msg.authfailure = true;
      cb(msg, null);
      return;
    }
    
    if (req.response.status == 304) {
      this._log.info("getApps returned 304");
      cb(null, null);
      return;
    }
    if (req.response.status != 200) {
      this._log.error(req);
      cb(new Error("Unexpected error with getApps"), null);
      return;
    }

    let apps;
    try {
      let tmp = JSON.parse(req.response.body);
      tmp = tmp["apps"];
      
      apps = tmp.map(this._makeLocalApp, this);
      this._log.info("getApps succeeded and got " + apps.length);
    } catch (e) {
      this._log.error(CommonUtils.exceptionStr(e));
      cb(new Error("Exception in getApps " + e), null);
      return;
    }

    
    try {
      cb(null, apps);
      
      this._appsLastModified = parseInt(req.response.headers["x-timestamp"], 10);
      this._state.set("lastModified", ""  + this._appsLastModified);
    } catch (e) {
      this._log.error("Exception in getApps callback " + e);
    }
  },

  




  _makeRemoteApp: function _makeRemoteApp(app) {
    for each (let key in this.requiredLocalKeys) {
      if (!(key in app)) {
        throw new Error("Local app missing key " + key);
      }
    }

    let record = {
      name:          app.name,
      origin:        app.origin,
      receipts:      app.receipts,
      manifestPath:  app.manifestURL,
      installOrigin: app.installOrigin
    };
    if ("modifiedAt" in app) {
      record.modifiedAt = app.modifiedAt;
    }
    if ("installedAt" in app) {
      record.installedAt = app.installedAt;
    }
    return record;
  },

  



  _makeLocalApp: function _makeLocalApp(app) {
    for each (let key in this._requiredRemoteKeys) {
      if (!(key in app)) {
        throw new Error("Remote app missing key " + key);
      }
    }

    let record = {
      origin:         app.origin,
      installOrigin:  app.installOrigin,
      installedAt:    app.installedAt,
      modifiedAt:     app.modifiedAt,
      manifestURL:    app.manifestPath,
      receipts:       app.receipts
    };
    if ("hidden" in app) {
      record.hidden = app.hidden;
    }
    return record;
  },

  



  _putApp: function _putApp(app, cb) {
    if (!this._isRequestAllowed()) {
      
      
      
      
      
      let err = new Error("Backoff in effect, aborting PUT");
      err.processed = false;
      cb(err, null);
      return;
    }

    let uri = this._makeAppURI(app.origin);
    let req = new TokenAuthenticatedRESTRequest(uri, this.token);
    req.timeout = this._timeout;
    req.setHeader("Content-Type", "application/json");

    if (app.modifiedAt) {
      req.setHeader("X-If-Unmodified-Since", "" + app.modified);
    }

    let self = this;
    this._log.info("Trying to _putApp to " + uri);
    req.put(JSON.stringify(app), function _putAppCb(err) {
      self._processPutApp(err, cb, req);
    });
  },

  


  _processPutApp: function _processPutApp(error, cb, req) {
    this._setBackoff(req);

    if (error) {
      this._log.error("_putApp request error " + error);
      cb(error, null);
      return;
    }

    let err = null;
    switch (req.response.status) {
      case 201:
      case 204:
        this._log.info("_putApp succeeded");
        cb(null, true);
        break;

      case 401:
        
        err = new Error("_putApp failed due to 401 authentication failure");
        this._log.warn(err);
        err.authfailure = true;
        cb(err, null);
        break;

      case 409:
        
        err = new Error("_putApp failed due to 409 conflict");
        this._log.warn(err);
        cb(err,null);
        break;

      case 400:
      case 412:
      case 413:
        let msg = "_putApp returned: " + req.response.status;
        this._log.warn(msg);
        err = new Error(msg);
        err.processed = true;
        cb(err, null);
        break;

      default:
        this._error(req);
        err = new Error("Unexpected error with _putApp");
        err.processed = false;
        cb(err, null);
        break;
    }
  },

  


  _error: function _error(req) {
    this._log.error("Catch-all error for request: " +
      req.uri.asciiSpec + " " + req.response.status + " with: " +
      req.response.body);
  },

  _makeAppURI: function _makeAppURI(origin) {
    let part = CommonUtils.encodeBase64URL(
      CryptoUtils.UTF8AndSHA1(origin)
    ).replace("=", "");
    return this.uri + "/apps/" + part;
  },

  
  _isRequestAllowed: function _isRequestAllowed() {
    if (!this._backoff) {
      return true;
    }

    let time = Date.now();
    let backoff = parseInt(this._state.get("backoff", 0), 10);

    if (time < backoff) {
      this._log.warn(backoff - time + "ms left for backoff, aborting request");
      return false;
    }

    this._backoff = false;
    this._state.set("backoff", "0");
    return true;
  },

  
  _setBackoff: function _setBackoff(req) {
    let backoff = 0;
    let statusCodesWithoutBackoff = [200, 201, 204, 304, 401];

    let val;
    if (req.response.headers["Retry-After"]) {
      val = req.response.headers["Retry-After"];
      backoff = parseInt(val, 10);
      this._log.warn("Retry-Header header was seen: " + val);
    } else if (req.response.headers["X-Backoff"]) {
      val = req.response.headers["X-Backoff"];
      backoff = parseInt(val, 10);
      this._log.warn("X-Backoff header was seen: " + val);
    } else if (statusCodesWithoutBackoff.indexOf(req.response.status) === -1) {
      
      this._consecutiveFailures++;
      if (this._consecutiveFailures === this._maxFailures) {
        
        backoff = this._state.get("backoff.initial", DEFAULT_INITIAL_BACKOFF);
      } else if (this._consecutiveFailures > this._maxFailures) {
        
        backoff = Math.min(
          
          this._backoffTime * 2,
          
          this._state.get("backoff.max", DEFAULT_MAX_BACKOFF)
        );
      }
    } else {
      
      this._consecutiveFailures = 0;
    }
    if (backoff) {
      this._backoff = true;
      let time = Date.now();
      this._state.set("backoff", "" + (time + backoff));
      this._backoffTime = backoff;
      this._log.info("Client setting backoff to: " + backoff + "ms");
    }
  },
};
