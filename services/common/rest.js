



const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const EXPORTED_SYMBOLS = ["RESTRequest", "RESTResponse"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");

const Prefs = new Preferences("services.common.rest.");
























































function RESTRequest(uri) {
  this.status = this.NOT_SENT;

  
  
  if (!(uri instanceof Ci.nsIURI)) {
    uri = Services.io.newURI(uri, null, null);
  }
  this.uri = uri;

  this._headers = {};
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level =
    Log4Moz.Level[Prefs.get("log.logger.rest.request")];
}
RESTRequest.prototype = {

  _logName: "Services.Common.RESTRequest",

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIBadCertListener2,
    Ci.nsIInterfaceRequestor,
    Ci.nsIChannelEventSink
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
      CommonUtils.namedTimer(this.abortTimeout, this.timeout * 1000, this,
                             "timeoutTimer");
    }
  },

  


  abortTimeout: function abortTimeout() {
    this.abort();
    let error = Components.Exception("Aborting due to channel inactivity.",
                                     Cr.NS_ERROR_NET_TIMEOUT);
    if (!this.onComplete) {
      this._log.error("Unexpected error: onComplete not defined in " +
                      "abortTimeout.")
      return;
    }
    this.onComplete(error);
  },

  

  onStartRequest: function onStartRequest(channel) {
    if (this.status == this.ABORTED) {
      this._log.trace("Not proceeding with onStartRequest, request was aborted.");
      return;
    }

    try {
      channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (ex) {
      this._log.error("Unexpected error: channel is not a nsIHttpChannel!");
      this.status = this.ABORTED;
      channel.cancel(Cr.NS_BINDING_ABORTED);
      return;
    }

    this.status = this.IN_PROGRESS;

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
    if (this.timeoutTimer) {
      
      this.timeoutTimer.clear();
    }

    
    if (this.status == this.ABORTED) {
      this._log.trace("Not proceeding with onStopRequest, request was aborted.");
      return;
    }

    try {
      channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (ex) {
      this._log.error("Unexpected error: channel not nsIHttpChannel!");
      this.status = this.ABORTED;
      return;
    }
    this.status = this.COMPLETED;

    let statusSuccess = Components.isSuccessCode(statusCode);
    let uri = channel && channel.URI && channel.URI.spec || "<unknown>";
    this._log.trace("Channel for " + channel.requestMethod + " " + uri +
                    " returned status code " + statusCode);

    if (!this.onComplete) {
      this._log.error("Unexpected error: onComplete not defined in " +
                      "abortRequest.");
      this.onProgress = null;
      return;
    }

    
    
    
    if (!statusSuccess) {
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
      this._log.debug(CommonUtils.exceptionStr(ex));
      throw ex;
    }

    try {
      this.onProgress();
    } catch (ex) {
      this._log.warn("Got exception calling onProgress handler, aborting " +
                     this.method + " " + req.URI.spec);
      this._log.debug("Exception: " + CommonUtils.exceptionStr(ex));
      this.abort();

      if (!this.onComplete) {
        this._log.error("Unexpected error: onComplete not defined in " +
                        "onDataAvailable.");
        this.onProgress = null;
        return;
      }

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
  },

  
  asyncOnChannelRedirect:
    function asyncOnChannelRedirect(oldChannel, newChannel, flags, callback) {

    try {
      newChannel.QueryInterface(Ci.nsIHttpChannel);
    } catch (ex) {
      this._log.error("Unexpected error: channel not nsIHttpChannel!");
      callback.onRedirectVerifyCallback(Cr.NS_ERROR_NO_INTERFACE);
      return;
    }

    this.channel = newChannel;

    
    callback.onRedirectVerifyCallback(Cr.NS_OK);
  }
};





function RESTResponse() {
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level =
    Log4Moz.Level[Prefs.get("log.logger.rest.response")];
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
                      CommonUtils.exceptionStr(ex));
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
                      CommonUtils.exceptionStr(ex));
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
                      CommonUtils.exceptionStr(ex));
      return null;
    }

    delete this.headers;
    return this.headers = headers;
  },

  


  body: null

};
