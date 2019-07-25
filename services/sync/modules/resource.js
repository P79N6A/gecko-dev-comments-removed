




































const EXPORTED_SYMBOLS = ["Resource"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/ext/Observers.js");
Cu.import("resource://weave/ext/Preferences.js");
Cu.import("resource://weave/ext/Sync.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/auth.js");




function Resource(uri) {
  this._init(uri);
}
Resource.prototype = {
  _logName: "Net.Resource",

  
  
  
  
  
  
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
      this._headers[arguments[i]] = arguments[i + 1];
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

  _init: function Res__init(uri) {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.network.resources")];
    this.uri = uri;
    this._headers = {'Content-type': 'text/plain'};
  },

  
  
  
  
  
  
  _createRequest: function Res__createRequest() {
    let channel = Svc.IO.newChannel(this.spec, null, null).
      QueryInterface(Ci.nsIRequest).QueryInterface(Ci.nsIHttpChannel);

    
    channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    
    
    channel.notificationCallbacks = new badCertListener();
    
    
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

      let type = ('Content-Type' in this._headers)?
        this._headers['Content-Type'] : 'text/plain';

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
      let chanStack = ex.stack.trim().split(/\n/).slice(1);
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
          headers[header] = value;
        }
      });
      status = channel.responseStatus;
      success = channel.requestSucceeded;

      if (success) {
        this._log.debug(action + " success: " + status);
        if (this._log.level <= Log4Moz.Level.Trace)
          this._log.trace(action + " Body: " + this._data);
      }
      else {
        let log = "debug";
        let mesg = action + " fail: " + status;

        
        if (this._log.level <= Log4Moz.Level.Trace) {
          log = "trace";
          mesg += " " + this._data;
        }

        this._log[log](mesg);
      }
    }
    
    catch(ex) {
      this._log.debug(action + " cached: " + status);
    }

    let ret = new String(this._data);
    ret.headers = headers;
    ret.status = status;
    ret.success = success;

    
    Utils.lazy2(ret, "obj", function() JSON.parse(ret));

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
}
ChannelListener.prototype = {
  onStartRequest: function Channel_onStartRequest(channel) {
    
    channel.QueryInterface(Ci.nsIHttpChannel);

    let log = "trace";
    let mesg = channel.requestMethod + " request for " + channel.URI.spec;
    
    if (this._log.level > Log4Moz.Level.Trace) {
      log = "debug";
      if (mesg.length > 200)
        mesg = mesg.substr(0, 200) + "...";
    }
    this._log[log](mesg);

    this._data = '';
  },

  onStopRequest: function Channel_onStopRequest(channel, context, status) {
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
  }
};








function badCertListener() {
}
badCertListener.prototype = {
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
