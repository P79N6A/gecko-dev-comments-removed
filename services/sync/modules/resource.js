






































const EXPORTED_SYMBOLS = ["Resource", "AsyncResource",
                          "Auth", "BrokenBasicAuthenticator",
                          "BasicAuthenticator", "NoOpAuthenticator"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/async.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/ext/Preferences.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");

XPCOMUtils.defineLazyGetter(this, "Auth", function () {
  return new AuthMgr();
});




function NoOpAuthenticator() {}
NoOpAuthenticator.prototype = {
  onRequest: function NoOpAuth_onRequest(headers) {
    return headers;
  }
};



function BrokenBasicAuthenticator(identity) {
  this._id = identity;
}
BrokenBasicAuthenticator.prototype = {
  onRequest: function BasicAuth_onRequest(headers) {
    headers['authorization'] = 'Basic ' +
      btoa(this._id.username + ':' + this._id.password);
    return headers;
  }
};

function BasicAuthenticator(identity) {
  this._id = identity;
}
BasicAuthenticator.prototype = {
  onRequest: function onRequest(headers) {
    headers['authorization'] = 'Basic ' +
      btoa(this._id.username + ':' + this._id.passwordUTF8);
    return headers;
  }
};

function AuthMgr() {
  this._authenticators = {};
  this.defaultAuthenticator = new NoOpAuthenticator();
}
AuthMgr.prototype = {
  defaultAuthenticator: null,

  registerAuthenticator: function AuthMgr_register(match, authenticator) {
    this._authenticators[match] = authenticator;
  },

  lookupAuthenticator: function AuthMgr_lookup(uri) {
    for (let match in this._authenticators) {
      if (uri.match(match))
        return this._authenticators[match];
    }
    return this.defaultAuthenticator;
  }
};
























function AsyncResource(uri) {
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

  
  
  
  
  
  
  
  
  
  _userAgent:
    Services.appinfo.name + "/" + Services.appinfo.version +  
    " FxSync/" + WEAVE_VERSION + "." +                        
    Services.appinfo.appBuildID + ".",                        

  
  ABORT_TIMEOUT: 300000,

  
  
  
  
  
  
  get authenticator() {
    if (this._authenticator)
      return this._authenticator;
    else
      return Auth.lookupAuthenticator(this.spec);
  },
  set authenticator(value) {
    this._authenticator = value;
  },

  
  
  
  
  
  get headers() {
    return this.authenticator.onRequest(this._headers);
  },
  set headers(value) {
    this._headers = value;
  },
  setHeader: function Res_setHeader(header, value) {
    this._headers[header.toLowerCase()] = value;
  },

  
  
  
  get uri() {
    return this._uri;
  },
  set uri(value) {
    if (typeof value == 'string')
      this._uri = Utils.makeURI(value);
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

  
  
  
  
  
  
  _createRequest: function Res__createRequest() {
    let channel = Services.io.newChannel(this.spec, null, null)
                          .QueryInterface(Ci.nsIRequest)
                          .QueryInterface(Ci.nsIHttpChannel);

    
    channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;

    
    channel.notificationCallbacks = new BadCertListener();

    
    if (Svc.Prefs.get("sendVersionInfo", true)) {
      let ua = this._userAgent + Svc.Prefs.get("client.type", "desktop");
      channel.setRequestHeader("user-agent", ua, false);
    }
    
    
    let headers = this.headers;
    for (let key in headers) {
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
    let channel = this._channel = this._createRequest();

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

  _onComplete: function _onComplete(error, data) {
    this._log.trace("In _onComplete. Error is " + error + ".");

    if (error) {
      this._callback(error);
      return;
    }

    this._data = data;
    let channel = this._channel;
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
      
      
      this._log.warn("Caught unexpected exception " + Utils.exceptionStr(ex) +
                     " in _onComplete.");
      this._log.debug(Utils.stackTrace(ex));
    }

    
    
    let headers = {};
    try {
      this._log.trace("Processing response headers.");

      
      channel.visitResponseHeaders({
        visitHeader: function visitHeader(header, value) {
          headers[header.toLowerCase()] = value;
        }
      });

      
      
      if (headers["x-weave-backoff"])
        Observers.notify("weave:service:backoff:interval",
                         parseInt(headers["x-weave-backoff"], 10));

      if (success && headers["x-weave-quota-remaining"])
        Observers.notify("weave:service:quota:remaining",
                         parseInt(headers["x-weave-quota-remaining"], 10));
    } catch (ex) {
      this._log.debug("Caught exception " + Utils.exceptionStr(ex) +
                      " visiting headers in _onComplete.");
      this._log.debug(Utils.stackTrace(ex));
    }

    let ret     = new String(data);
    ret.status  = status;
    ret.success = success;
    ret.headers = headers;

    
    
    
    XPCOMUtils.defineLazyGetter(ret, "obj", function() JSON.parse(ret));

    
    
    if (status == 401) {
      
      let subject = {
        newUri: "",
        resource: this,
        response: ret
      }
      Observers.notify("weave:resource:status:401", subject);

      
      if (subject.newUri != "") {
        this.uri = subject.newUri;
        this._doRequest(action, this._data, this._callback);
        return;
      }
    }

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

  delete: function delete(callback) {
    this._doRequest("DELETE", undefined, callback);
  }
};









function Resource(uri) {
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
    channel.QueryInterface(Ci.nsIHttpChannel);

    
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

    let success = Components.isSuccessCode(status);
    let uri = channel && channel.URI && channel.URI.spec || "<unknown>";
    this._log.trace("Channel for " + channel.requestMethod + " " +
                    uri + ": isSuccessCode(" + status + ")? " +
                    success);

    if (this._data == '')
      this._data = null;

    
    
    
    if (!success) {
      let message = Components.Exception("", status).name;
      let error = Components.Exception(message, status);
      this._onComplete(error);
      return;
    }

    this._log.trace("Channel: flags = " + channel.loadFlags +
                    ", URI = " + uri +
                    ", HTTP success? " + channel.requestSucceeded);
    this._onComplete(null, this._data);
  },

  onDataAvailable: function Channel_onDataAvail(req, cb, stream, off, count) {
    let siStream = Cc["@mozilla.org/scriptableinputstream;1"].
      createInstance(Ci.nsIScriptableInputStream);
    siStream.init(stream);
    try {
      this._data += siStream.read(count);
    } catch (ex) {
      this._log.warn("Exception thrown reading " + count +
                     " bytes from " + siStream + ".");
      throw ex;
    }
    
    try {
      this._onProgress();
    } catch (ex) {
      this._log.warn("Got exception calling onProgress handler during fetch of "
                     + req.URI.spec);
      this._log.debug(Utils.exceptionStr(ex));
      this._log.trace("Rethrowing; expect a failure code from the HTTP channel.");
      throw ex;
    }
    
    this.delayAbort();
  },

  


  delayAbort: function delayAbort() {
    Utils.namedTimer(this.abortRequest, this._timeout, this, "abortTimer");
  },

  abortRequest: function abortRequest() {
    
    this.onStopRequest = function() {};
    let error = Components.Exception("Aborting due to channel inactivity.",
                                     Cr.NS_ERROR_NET_TIMEOUT);
    this._onComplete(error);
  }
};








function BadCertListener() {
}
BadCertListener.prototype = {
  getInterface: function(aIID) {
    return this.QueryInterface(aIID);
  },

  QueryInterface: function(aIID) {
    if (aIID.equals(Components.interfaces.nsIBadCertListener2) ||
        aIID.equals(Components.interfaces.nsIInterfaceRequestor) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  notifyCertProblem: function certProblem(socketInfo, sslStatus, targetHost) {
    
    let log = Log4Moz.repository.getLogger("Sync.CertListener");
    log.level =
      Log4Moz.Level[Svc.Prefs.get("log.logger.network.resources")];
    log.debug("Invalid HTTPS certificate encountered, ignoring!");

    return true;
  }
};
