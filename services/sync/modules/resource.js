




































const EXPORTED_SYMBOLS = ["Resource"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/auth.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/ext/Preferences.js");
Cu.import("resource://services-sync/ext/Sync.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");




function Resource(uri) {
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._log.level =
    Log4Moz.Level[Utils.prefs.getCharPref("log.logger.network.resources")];
  this.uri = uri;
  this._headers = {};
}
Resource.prototype = {
  _logName: "Net.Resource",

  
  
  
  serverTime: null,

  
  
  
  
  
  
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
  setHeader: function Res_setHeader() {
    if (arguments.length % 2)
      throw "setHeader only accepts arguments in multiples of 2";
    for (let i = 0; i < arguments.length; i += 2) {
      this._headers[arguments[i].toLowerCase()] = arguments[i + 1];
    }
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
    let channel = Svc.IO.newChannel(this.spec, null, null).
      QueryInterface(Ci.nsIRequest).QueryInterface(Ci.nsIHttpChannel);

    
    channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;

    
    channel.notificationCallbacks = new BadCertListener();

    
    let headers = this.headers;
    for (let key in headers) {
      if (key == 'Authorization')
        this._log.trace("HTTP Header " + key + ": ***** (suppressed)");
      else
        this._log.trace("HTTP Header " + key + ": " + headers[key]);
      channel.setRequestHeader(key, headers[key], false);
    }
    return channel;
  },

  _onProgress: function Res__onProgress(channel) {},

  
  
  
  
  
  _request: function Res__request(action, data) {
    let iter = 0;
    let channel = this._createRequest();

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

    
    
    let [chanOpen, chanCb] = Sync.withCb(channel.asyncOpen, channel);
    let listener = new ChannelListener(chanCb, this._onProgress, this._log);
    channel.requestMethod = action;

    
    try {
      this._data = chanOpen(listener, null);
    }
    catch(ex) {
      
      let error = Error(ex.message);
      let chanStack = [];
      if (ex.stack)
        chanStack = ex.stack.trim().split(/\n/).slice(1);
      let requestStack = error.stack.split(/\n/).slice(1);

      
      for (let i = 0; i <= 1; i++)
        requestStack[i] = requestStack[i].replace(/\(".*"\)@/, "(...)@");

      error.stack = chanStack.concat(requestStack).join("\n");
      throw error;
    }

    
    let headers = {};
    let status = 0;
    let success = true;
    try {
      
      channel.visitResponseHeaders({
        visitHeader: function visitHeader(header, value) {
          headers[header.toLowerCase()] = value;
        }
      });
      status = channel.responseStatus;
      success = channel.requestSucceeded;

      
      let mesg = [action, success ? "success" : "fail", status,
        channel.URI.spec].join(" ");
      if (mesg.length > 200)
        mesg = mesg.substr(0, 200) + "â€¦";
      this._log.debug(mesg);
      
      if (this._log.level <= Log4Moz.Level.Trace)
        this._log.trace(action + " body: " + this._data);

      
      if (headers["x-weave-backoff"])
        Observers.notify("weave:service:backoff:interval", parseInt(headers["x-weave-backoff"], 10))
    }
    
    catch(ex) {
      this._log.debug(action + " cached: " + status);
    }

    let ret = new String(this._data);
    ret.headers = headers;
    ret.status = status;
    ret.success = success;

    
    Utils.lazy2(ret, "obj", function() JSON.parse(ret));

    
    if (status == 401) {
      
      let subject = {
        newUri: "",
        resource: this,
        response: ret
      }
      Observers.notify("weave:resource:status:401", subject);

      
      if (subject.newUri != "") {
        this.uri = subject.newUri;
        return this._request.apply(this, arguments);
      }
    }

    return ret;
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





function ChannelListener(onComplete, onProgress, logger) {
  this._onComplete = onComplete;
  this._onProgress = onProgress;
  this._log = logger;
  this.delayAbort();
}
ChannelListener.prototype = {
  
  ABORT_TIMEOUT: 300000,

  onStartRequest: function Channel_onStartRequest(channel) {
    channel.QueryInterface(Ci.nsIHttpChannel);

    
    try {
      Resource.serverTime = channel.getResponseHeader("X-Weave-Timestamp") - 0;
    }
    catch(ex) {}

    this._log.trace(channel.requestMethod + " " + channel.URI.spec);
    this._data = '';
    this.delayAbort();
  },

  onStopRequest: function Channel_onStopRequest(channel, context, status) {
    
    this.abortTimer.clear();

    if (this._data == '')
      this._data = null;

    
    if (!Components.isSuccessCode(status))
      this._onComplete.throw(Error(Components.Exception("", status).name));

    this._onComplete(this._data);
  },

  onDataAvailable: function Channel_onDataAvail(req, cb, stream, off, count) {
    let siStream = Cc["@mozilla.org/scriptableinputstream;1"].
      createInstance(Ci.nsIScriptableInputStream);
    siStream.init(stream);

    this._data += siStream.read(count);
    this._onProgress();
    this.delayAbort();
  },

  


  delayAbort: function delayAbort() {
    Utils.delay(this.abortRequest, this.ABORT_TIMEOUT, this, "abortTimer");
  },

  abortRequest: function abortRequest() {
    
    this.onStopRequest = function() {};
    this.onDataAvailable = function() {};
    this._onComplete.throw(Error("Aborting due to channel inactivity."));
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
    
    let log = Log4Moz.repository.getLogger("Service.CertListener");
    log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.network.resources")];
    log.debug("Invalid HTTPS certificate encountered, ignoring!");

    return true;
  }
};
