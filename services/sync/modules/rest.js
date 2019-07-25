







































const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/constants.js");

const EXPORTED_SYMBOLS = ["RESTRequest", "SyncStorageRequest"];

const STORAGE_REQUEST_TIMEOUT = 5 * 60; 

























































function RESTRequest(uri) {
  this.status = this.NOT_SENT;

  
  
  if (!(uri instanceof Ci.nsIURI)) {
    uri = Services.io.newURI(uri, null, null);
  }
  this.uri = uri;

  this._headers = {};
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level =
    Log4Moz.Level[Svc.Prefs.get("log.logger.network.resources")];
}
RESTRequest.prototype = {

  _logName: "Sync.RESTRequest",

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIBadCertListener2,
    Ci.nsIInterfaceRequestor
  ]),

  

  


  uri: null,

  


  method: null,

  


  response: null,

  


  loadFlags: Ci.nsIRequest.LOAD_BYPASS_CACHE | Ci.nsIRequest.INHIBIT_CACHING,

  


  channel: null,

  




  status: null,

  NOT_SENT:    0,
  SENT:        1,
  IN_PROGRESS: 2,
  COMPLETED:   4,
  ABORTED:     8,

  





  timeout: null,

  







  onComplete: function onComplete(error) {
  },

  




  onProgress: function onProgress() {
  },

  


  setHeader: function setHeader(name, value) {
    this._headers[name.toLowerCase()] = value;
  },

  









  get: function get(onComplete, onProgress) {
    return this.dispatch("GET", null, onComplete, onProgress);
  },

  












  put: function put(data, onComplete, onProgress) {
    return this.dispatch("PUT", data, onComplete, onProgress);
  },

  












  post: function post(data, onComplete, onProgress) {
    return this.dispatch("POST", data, onComplete, onProgress);
  },

  









  delete: function delete_(onComplete, onProgress) {
    return this.dispatch("DELETE", null, onComplete, onProgress);
  },

  


  abort: function abort() {
    if (this.status != this.SENT && this.status != this.IN_PROGRESS) {
      throw "Can only abort a request that has been sent.";
    }

    this.status = this.ABORTED;
    this.channel.cancel(Cr.NS_BINDING_ABORTED);

    if (this.timeoutTimer) {
      
      this.timeoutTimer.clear();
    }
  },

  

  dispatch: function dispatch(method, data, onComplete, onProgress) {
    if (this.status != this.NOT_SENT) {
      throw "Request has already been sent!";
    }

    this.method = method;
    if (onComplete) {
      this.onComplete = onComplete;
    }
    if (onProgress) {
      this.onProgress = onProgress;
    }

    
    let channel = Services.io.newChannelFromURI(this.uri, null, null)
                          .QueryInterface(Ci.nsIRequest)
                          .QueryInterface(Ci.nsIHttpChannel);
    this.channel = channel;
    channel.loadFlags |= this.loadFlags;
    channel.notificationCallbacks = this;

    
    let headers = this._headers;
    for (let key in headers) {
      if (key == 'authorization') {
        this._log.trace("HTTP Header " + key + ": ***** (suppressed)");
      } else {
        this._log.trace("HTTP Header " + key + ": " + headers[key]);
      }
      channel.setRequestHeader(key, headers[key], false);
    }

    
    if (method == "PUT" || method == "POST") {
      
      if (typeof data != "string") {
        data = JSON.stringify(data);
      }

      this._log.debug(method + " Length: " + data.length);
      if (this._log.level <= Log4Moz.Level.Trace) {
        this._log.trace(method + " Body: " + data);
      }

      let stream = Cc["@mozilla.org/io/string-input-stream;1"]
                     .createInstance(Ci.nsIStringInputStream);
      stream.setData(data, data.length);

      let type = headers["content-type"] || "text/plain";
      channel.QueryInterface(Ci.nsIUploadChannel);
      channel.setUploadStream(stream, type, data.length);
    }
    
    
    channel.requestMethod = method;

    
    channel.asyncOpen(this, null);
    this.status = this.SENT;
    this.delayTimeout();
    return this;
  },

  


  delayTimeout: function delayTimeout() {
    if (this.timeout) {
      Utils.namedTimer(this.abortTimeout, this.timeout * 1000, this,
                       "timeoutTimer");
    }
  },

  


  abortTimeout: function abortTimeout() {
    this.abort();
    let error = Components.Exception("Aborting due to channel inactivity.",
                                     Cr.NS_ERROR_NET_TIMEOUT);
    this.onComplete(error);
  },

  

  onStartRequest: function onStartRequest(channel) {
    
    this.channel = channel;

    if (this.status == this.ABORTED) {
      this._log.trace("Not proceeding with onStartRequest, request was aborted.");
      return;
    }
    this.status = this.IN_PROGRESS;

    channel.QueryInterface(Ci.nsIHttpChannel);
    this._log.trace("onStartRequest: " + channel.requestMethod + " " +
                    channel.URI.spec);

    
    let response = this.response = new RESTResponse();
    response.request = this;
    response.body = "";

    
    
    this._inputStream = Cc["@mozilla.org/scriptableinputstream;1"]
                          .createInstance(Ci.nsIScriptableInputStream);

    this.delayTimeout();
  },

  onStopRequest: function onStopRequest(channel, context, statusCode) {
    
    this.channel = channel;

    if (this.timeoutTimer) {
      
      this.timeoutTimer.clear();
    }

    
    if (this.status == this.ABORTED) {
      this._log.trace("Not proceeding with onStopRequest, request was aborted.");
      return;
    }
    this.status = this.COMPLETED;

    












    let requestStatus = Cr.NS_ERROR_UNEXPECTED;
    let statusSuccess = Components.isSuccessCode(statusCode);
    try {
      
      requestStatus = channel.status;
      this._log.trace("Request status is " + requestStatus);
    } catch (ex) {
      this._log.warn("Got exception " + Utils.exceptionStr(ex) +
                     " fetching channel.status.");
    }
    if (statusSuccess && (statusCode != requestStatus)) {
      this._log.error("Request status " + requestStatus +
                      " does not match status arg " + statusCode);
      try {
        channel.responseStatus;
      } catch (ex) {
        this._log.error("... and we got " + Utils.exceptionStr(ex) +
                        " retrieving responseStatus.");
      }
    }

    let requestStatusSuccess = Components.isSuccessCode(requestStatus);

    let uri = channel && channel.URI && channel.URI.spec || "<unknown>";
    this._log.trace("Channel for " + channel.requestMethod + " " + uri +
                    " returned status code " + statusCode);

    
    
    
    if (!statusSuccess || !requestStatusSuccess) {
      let message = Components.Exception("", statusCode).name;
      let error = Components.Exception(message, statusCode);
      this.onComplete(error);
      this.onComplete = this.onProgress = null;
      return;
    }

    this._log.debug(this.method + " " + uri + " " + this.response.status);

    
    if (this._log.level <= Log4Moz.Level.Trace) {
      this._log.trace(this.method + " body: " + this.response.body);
    }

    delete this._inputStream;

    this.onComplete(null);
    this.onComplete = this.onProgress = null;
  },

  onDataAvailable: function onDataAvailable(req, cb, stream, off, count) {
    this._inputStream.init(stream);
    try {
      this.response.body += this._inputStream.read(count);
    } catch (ex) {
      this._log.warn("Exception thrown reading " + count +
                     " bytes from the channel.");
      this._log.debug(Utils.exceptionStr(ex));
      throw ex;
    }

    try {
      this.onProgress();
    } catch (ex) {
      this._log.warn("Got exception calling onProgress handler, aborting " +
                     this.method + " " + req.URI.spec);
      this._log.debug("Exception: " + Utils.exceptionStr(ex));
      this.abort();
      this.onComplete(ex);
      this.onComplete = this.onProgress = null;
      return;
    }

    this.delayTimeout();
  },

  

  getInterface: function(aIID) {
    return this.QueryInterface(aIID);
  },

  

  notifyCertProblem: function notifyCertProblem(socketInfo, sslStatus, targetHost) {
    this._log.warn("Invalid HTTPS certificate encountered!");
    
    
    return true;
  }
};






function RESTResponse() {
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level =
    Log4Moz.Level[Svc.Prefs.get("log.logger.network.resources")];
}
RESTResponse.prototype = {

  _logName: "Sync.RESTResponse",

  


  request: null,

  


  get status() {
    let status;
    try {
      let channel = this.request.channel.QueryInterface(Ci.nsIHttpChannel);
      status = channel.responseStatus;
    } catch (ex) {
      this._log.debug("Caught exception fetching HTTP status code:" +
                      Utils.exceptionStr(ex));
      return null;
    }
    delete this.status;
    return this.status = status;
  },

  


  get success() {
    let success;
    try {
      let channel = this.request.channel.QueryInterface(Ci.nsIHttpChannel);
      success = channel.requestSucceeded;
    } catch (ex) {
      this._log.debug("Caught exception fetching HTTP success flag:" +
                      Utils.exceptionStr(ex));
      return null;
    }
    delete this.success;
    return this.success = success;
  },

  


  get headers() {
    let headers = {};
    try {
      this._log.trace("Processing response headers.");
      let channel = this.request.channel.QueryInterface(Ci.nsIHttpChannel);
      channel.visitResponseHeaders(function (header, value) {
        headers[header.toLowerCase()] = value;
      });
    } catch (ex) {
      this._log.debug("Caught exception processing response headers:" +
                      Utils.exceptionStr(ex));
      return null;
    }

    delete this.headers;
    return this.headers = headers;
  },

  


  body: null

};





function SyncStorageRequest(uri) {
  RESTRequest.call(this, uri);
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

    
    let id = ID.get("WeaveID");
    if (id) {
      let auth_header = "Basic " + btoa(id.username + ':' + id.passwordUTF8);
      this.setHeader("authorization", auth_header);
    } else {
      this._log.debug("Couldn't set Authentication header: WeaveID not found.");
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
  }
};
