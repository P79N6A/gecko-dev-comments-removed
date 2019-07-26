



this.EXPORTED_SYMBOLS = [
  "AsyncResource",
  "Resource"
];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-common/async.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/util.js");

const DEFAULT_LOAD_FLAGS =
  
  Ci.nsIRequest.LOAD_BYPASS_CACHE |
  Ci.nsIRequest.INHIBIT_CACHING |
  
  Ci.nsIRequest.LOAD_ANONYMOUS;























this.AsyncResource = function AsyncResource(uri) {
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level =
    Log4Moz.Level[Svc.Prefs.get("log.logger.network.resources")];
  this.uri = uri;
  this._headers = {};
  this._onComplete = Utils.bind2(this, this._onComplete);
}
AsyncResource.prototype = {
  _logName: "Sync.AsyncResource",

  
  
  
  serverTime: null,

  





  authenticator: null,

  
  
  
  
  
  
  
  
  
  _userAgent:
    Services.appinfo.name + "/" + Services.appinfo.version +  
    " FxSync/" + WEAVE_VERSION + "." +                        
    Services.appinfo.appBuildID + ".",                        

  
  ABORT_TIMEOUT: 300000,

  
  
  
  
  
  get headers() {
    return this._headers;
  },
  set headers(value) {
    this._headers = value;
  },
  setHeader: function Res_setHeader(header, value) {
    this._headers[header.toLowerCase()] = value;
  },
  get headerNames() {
    return Object.keys(this.headers);
  },

  
  
  
  get uri() {
    return this._uri;
  },
  set uri(value) {
    if (typeof value == 'string')
      this._uri = CommonUtils.makeURI(value);
    else
      this._uri = value;
  },

  
  
  
  get spec() {
    if (this._uri)
      return this._uri.spec;
    return null;
  },

  
  
  
  _data: null,
  get data() this._data,
  set data(value) {
    this._data = value;
  },

  
  
  
  
  
  
  _createRequest: function Res__createRequest(method) {
    let channel = Services.io.newChannel(this.spec, null, null)
                          .QueryInterface(Ci.nsIRequest)
                          .QueryInterface(Ci.nsIHttpChannel);

    channel.loadFlags |= DEFAULT_LOAD_FLAGS;

    
    let listener = new ChannelNotificationListener(this.headerNames);
    channel.notificationCallbacks = listener;

    
    if (Svc.Prefs.get("sendVersionInfo", true)) {
      let ua = this._userAgent + Svc.Prefs.get("client.type", "desktop");
      channel.setRequestHeader("user-agent", ua, false);
    }

    let headers = this.headers;

    if (this.authenticator) {
      let result = this.authenticator(this, method);
      if (result && result.headers) {
        for (let [k, v] in Iterator(result.headers)) {
          headers[k.toLowerCase()] = v;
        }
      }
    } else {
      this._log.debug("No authenticator found.");
    }

    for (let [key, value] in Iterator(headers)) {
      if (key == 'authorization')
        this._log.trace("HTTP Header " + key + ": ***** (suppressed)");
      else
        this._log.trace("HTTP Header " + key + ": " + headers[key]);
      channel.setRequestHeader(key, headers[key], false);
    }
    return channel;
  },

  _onProgress: function Res__onProgress(channel) {},

  _doRequest: function _doRequest(action, data, callback) {
    this._log.trace("In _doRequest.");
    this._callback = callback;
    let channel = this._createRequest(action);

    if ("undefined" != typeof(data))
      this._data = data;

    
    if ("PUT" == action || "POST" == action) {
      
      if (this._data.constructor.toString() != String)
        this._data = JSON.stringify(this._data);

      this._log.debug(action + " Length: " + this._data.length);
      this._log.trace(action + " Body: " + this._data);

      let type = ('content-type' in this._headers) ?
        this._headers['content-type'] : 'text/plain';

      let stream = Cc["@mozilla.org/io/string-input-stream;1"].
        createInstance(Ci.nsIStringInputStream);
      stream.setData(this._data, this._data.length);

      channel.QueryInterface(Ci.nsIUploadChannel);
      channel.setUploadStream(stream, type, this._data.length);
    }

    
    
    let listener = new ChannelListener(this._onComplete, this._onProgress,
                                       this._log, this.ABORT_TIMEOUT);
    channel.requestMethod = action;
    channel.asyncOpen(listener, null);
  },

  _onComplete: function _onComplete(error, data, channel) {
    this._log.trace("In _onComplete. Error is " + error + ".");

    if (error) {
      this._callback(error);
      return;
    }

    this._data = data;
    let action = channel.requestMethod;

    this._log.trace("Channel: " + channel);
    this._log.trace("Action: "  + action);

    
    
    let status = 0;
    let success = false;

    try {
      status  = channel.responseStatus;
      success = channel.requestSucceeded;    

      this._log.trace("Status: " + status);
      this._log.trace("Success: " + success);

      
      let mesg = [action, success ? "success" : "fail", status,
                  channel.URI.spec].join(" ");
      this._log.debug("mesg: " + mesg);

      if (mesg.length > 200)
        mesg = mesg.substr(0, 200) + "â€¦";
      this._log.debug(mesg);

      
      if (this._log.level <= Log4Moz.Level.Trace)
        this._log.trace(action + " body: " + data);

    } catch(ex) {
      
      
      this._log.warn("Caught unexpected exception " + CommonUtils.exceptionStr(ex) +
                     " in _onComplete.");
      this._log.debug(CommonUtils.stackTrace(ex));
    }

    
    
    let headers = {};
    try {
      this._log.trace("Processing response headers.");

      
      channel.visitResponseHeaders({
        visitHeader: function visitHeader(header, value) {
          headers[header.toLowerCase()] = value;
        }
      });

      
      
      if (headers["x-weave-backoff"]) {
        let backoff = headers["x-weave-backoff"];
        this._log.debug("Got X-Weave-Backoff: " + backoff);
        Observers.notify("weave:service:backoff:interval",
                         parseInt(backoff, 10));
      }

      if (success && headers["x-weave-quota-remaining"]) {
        Observers.notify("weave:service:quota:remaining",
                         parseInt(headers["x-weave-quota-remaining"], 10));
      }
    } catch (ex) {
      this._log.debug("Caught exception " + CommonUtils.exceptionStr(ex) +
                      " visiting headers in _onComplete.");
      this._log.debug(CommonUtils.stackTrace(ex));
    }

    let ret     = new String(data);
    ret.status  = status;
    ret.success = success;
    ret.headers = headers;

    
    
    
    XPCOMUtils.defineLazyGetter(ret, "obj", function() {
      try {
        return JSON.parse(ret);
      } catch (ex) {
        this._log.warn("Got exception parsing response body: \"" + CommonUtils.exceptionStr(ex));
        
        this._log.debug("Parse fail: Response body starts: \"" +
                        JSON.stringify((ret + "").slice(0, 100)) +
                        "\".");
        throw ex;
      }
    }.bind(this));

    this._callback(null, ret);
  },

  get: function get(callback) {
    this._doRequest("GET", undefined, callback);
  },

  put: function put(data, callback) {
    if (typeof data == "function")
      [data, callback] = [undefined, data];
    this._doRequest("PUT", data, callback);
  },

  post: function post(data, callback) {
    if (typeof data == "function")
      [data, callback] = [undefined, data];
    this._doRequest("POST", data, callback);
  },

  delete: function delete_(callback) {
    this._doRequest("DELETE", undefined, callback);
  }
};









this.Resource = function Resource(uri) {
  AsyncResource.call(this, uri);
}
Resource.prototype = {

  __proto__: AsyncResource.prototype,

  _logName: "Sync.Resource",

  
  
  
  
  
  _request: function Res__request(action, data) {
    let cb = Async.makeSyncCallback();
    function callback(error, ret) {
      if (error)
        cb.throw(error);
      cb(ret);
    }

    
    try {
      this._doRequest(action, data, callback);
      return Async.waitForSyncCallback(cb);
    } catch(ex) {
      
      
      let error = Error(ex.message);
      error.result = ex.result;
      let chanStack = [];
      if (ex.stack)
        chanStack = ex.stack.trim().split(/\n/).slice(1);
      let requestStack = error.stack.split(/\n/).slice(1);

      
      for (let i = 0; i <= 1; i++)
        requestStack[i] = requestStack[i].replace(/\(".*"\)@/, "(...)@");

      error.stack = chanStack.concat(requestStack).join("\n");
      throw error;
    }
  },

  
  
  
  get: function Res_get() {
    return this._request("GET");
  },

  
  
  
  put: function Res_put(data) {
    return this._request("PUT", data);
  },

  
  
  
  post: function Res_post(data) {
    return this._request("POST", data);
  },

  
  
  
  delete: function Res_delete() {
    return this._request("DELETE");
  }
};





function ChannelListener(onComplete, onProgress, logger, timeout) {
  this._onComplete = onComplete;
  this._onProgress = onProgress;
  this._log = logger;
  this._timeout = timeout;
  this.delayAbort();
}
ChannelListener.prototype = {

  onStartRequest: function Channel_onStartRequest(channel) {
    this._log.trace("onStartRequest called for channel " + channel + ".");

    try {
      channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (ex) {
      this._log.error("Unexpected error: channel is not a nsIHttpChannel!");
      channel.cancel(Cr.NS_BINDING_ABORTED);
      return;
    }

    
    try {
      AsyncResource.serverTime = channel.getResponseHeader("X-Weave-Timestamp") - 0;
    }
    catch(ex) {}

    this._log.trace("onStartRequest: " + channel.requestMethod + " " +
                    channel.URI.spec);
    this._data = '';
    this.delayAbort();
  },

  onStopRequest: function Channel_onStopRequest(channel, context, status) {
    
    this.abortTimer.clear();

    if (!this._onComplete) {
      this._log.error("Unexpected error: _onComplete not defined in onStopRequest.");
      this._onProgress = null;
      return;
    }

    try {
      channel.QueryInterface(Ci.nsIHttpChannel);
    } catch (ex) {
      this._log.error("Unexpected error: channel is not a nsIHttpChannel!");

      this._onComplete(ex, this._data, channel);
      this._onComplete = this._onProgress = null;
      return;
    }

    let statusSuccess = Components.isSuccessCode(status);
    let uri = channel && channel.URI && channel.URI.spec || "<unknown>";
    this._log.trace("Channel for " + channel.requestMethod + " " + uri + ": " +
                    "isSuccessCode(" + status + ")? " + statusSuccess);

    if (this._data == '') {
      this._data = null;
    }

    
    
    
    if (!statusSuccess) {
      let message = Components.Exception("", status).name;
      let error   = Components.Exception(message, status);

      this._onComplete(error, undefined, channel);
      this._onComplete = this._onProgress = null;
      return;
    }

    this._log.trace("Channel: flags = " + channel.loadFlags +
                    ", URI = " + uri +
                    ", HTTP success? " + channel.requestSucceeded);
    this._onComplete(null, this._data, channel);
    this._onComplete = this._onProgress = null;
  },

  onDataAvailable: function Channel_onDataAvail(req, cb, stream, off, count) {
    let siStream;
    try {
      siStream = Cc["@mozilla.org/scriptableinputstream;1"].createInstance(Ci.nsIScriptableInputStream);
      siStream.init(stream);
    } catch (ex) {
      this._log.warn("Exception creating nsIScriptableInputStream." + CommonUtils.exceptionStr(ex));
      this._log.debug("Parameters: " + req.URI.spec + ", " + stream + ", " + off + ", " + count);
      
      throw ex;
    }

    try {
      this._data += siStream.read(count);
    } catch (ex) {
      this._log.warn("Exception thrown reading " + count + " bytes from " + siStream + ".");
      throw ex;
    }

    try {
      this._onProgress();
    } catch (ex) {
      this._log.warn("Got exception calling onProgress handler during fetch of "
                     + req.URI.spec);
      this._log.debug(CommonUtils.exceptionStr(ex));
      this._log.trace("Rethrowing; expect a failure code from the HTTP channel.");
      throw ex;
    }

    this.delayAbort();
  },

  


  delayAbort: function delayAbort() {
    try {
      CommonUtils.namedTimer(this.abortRequest, this._timeout, this, "abortTimer");
    } catch (ex) {
      this._log.warn("Got exception extending abort timer: " + CommonUtils.exceptionStr(ex));
    }
  },

  abortRequest: function abortRequest() {
    
    this.onStopRequest = function() {};
    let error = Components.Exception("Aborting due to channel inactivity.",
                                     Cr.NS_ERROR_NET_TIMEOUT);
    if (!this._onComplete) {
      this._log.error("Unexpected error: _onComplete not defined in " +
                      "abortRequest.");
      return;
    }
    this._onComplete(error);
  }
};










function ChannelNotificationListener(headersToCopy) {
  this._headersToCopy = headersToCopy;

  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.network.resources")];
}
ChannelNotificationListener.prototype = {
  _logName: "Sync.Resource",

  getInterface: function(aIID) {
    return this.QueryInterface(aIID);
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Ci.nsIBadCertListener2) ||
        aIID.equals(Ci.nsIInterfaceRequestor) ||
        aIID.equals(Ci.nsISupports) ||
        aIID.equals(Ci.nsIChannelEventSink))
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  notifyCertProblem: function certProblem(socketInfo, sslStatus, targetHost) {
    let log = Log4Moz.repository.getLogger("Sync.CertListener");
    log.warn("Invalid HTTPS certificate encountered!");

    
    return true;
  },

  asyncOnChannelRedirect:
    function asyncOnChannelRedirect(oldChannel, newChannel, flags, callback) {

    let oldSpec = (oldChannel && oldChannel.URI) ? oldChannel.URI.spec : "<undefined>";
    let newSpec = (newChannel && newChannel.URI) ? newChannel.URI.spec : "<undefined>";
    this._log.debug("Channel redirect: " + oldSpec + ", " + newSpec + ", " + flags);

    this._log.debug("Ensuring load flags are set.");
    newChannel.loadFlags |= DEFAULT_LOAD_FLAGS;

    
    try {
      if ((flags & Ci.nsIChannelEventSink.REDIRECT_INTERNAL) &&
          newChannel.URI.equals(oldChannel.URI)) {
        this._log.debug("Copying headers for safe internal redirect.");
        for (let header of this._headersToCopy) {
          let value = oldChannel.getRequestHeader(header);
          if (value) {
            let printed = (header == "authorization") ? "****" : value;
            this._log.debug("Header: " + header + " = " + printed);
            newChannel.setRequestHeader(header, value);
          } else {
            this._log.warn("No value for header " + header);
          }
        }
      }
    } catch (ex) {
      this._log.error("Error copying headers: " + CommonUtils.exceptionStr(ex));
    }

    
    try {
      callback.onRedirectVerifyCallback(Cr.NS_OK);
    } catch (ex) {
      this._log.error("onRedirectVerifyCallback threw!" + CommonUtils.exceptionStr(ex));
    }
  }
};
