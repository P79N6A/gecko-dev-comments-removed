




































const EXPORTED_SYMBOLS = ['Resource', 'JsonFilter'];

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
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/auth.js");

Function.prototype.async = Async.sugar;

function RequestException(resource, action, request) {
  this._resource = resource;
  this._action = action;
  this._request = request;
  this.location = Components.stack.caller;
}
RequestException.prototype = {
  get resource() { return this._resource; },
  get action() { return this._action; },
  get request() { return this._request; },
  get status() { return this._request.status; },
  toString: function ReqEx_toString() {
    return "Could not " + this._action + " resource " + this._resource.spec +
      " (" + this._request.responseStatus + ")";
  }
};

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
    this._dirty = true;
    this._downloaded = false;
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
    this._dirty = true;
    this._data = value;
  },

  _lastChannel: null,
  _downloaded: false,
  _dirty: false,
  get lastChannel() this._lastChannel,
  get downloaded() this._downloaded,
  get dirty() this._dirty,

  _filters: null,
  pushFilter: function Res_pushFilter(filter) {
    this._filters.push(filter);
  },
  popFilter: function Res_popFilter() {
    return this._filters.pop();
  },
  clearFilters: function Res_clearFilters() {
    this._filters = [];
  },

  _init: function Res__init(uri) {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.network.resources")];
    this.uri = uri;
    this._headers = {'Content-type': 'text/plain'};
    this._filters = [];
  },

  _createRequest: function Res__createRequest() {
    this._lastChannel = Svc.IO.newChannel(this.spec, null, null).
      QueryInterface(Ci.nsIRequest);
    
    let loadFlags = this._lastChannel.loadFlags;
    loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    this._lastChannel.loadFlags = loadFlags;
    this._lastChannel = this._lastChannel.QueryInterface(Ci.nsIHttpChannel);

    this._lastChannel.notificationCallbacks = new badCertListener();

    let headers = this.headers; 
    for (let key in headers) {
      if (key == 'Authorization')
        this._log.trace("HTTP Header " + key + ": ***** (suppressed)");
      else
        this._log.trace("HTTP Header " + key + ": " + headers[key]);
      this._lastChannel.setRequestHeader(key, headers[key], false);
    }
    return this._lastChannel;
  },

  _onProgress: function Res__onProgress(event) {
    this._lastProgress = Date.now();
  },

  filterUpload: function Resource_filterUpload() {
    this._data = this._filters.reduce(function(data, filter) {
      return filter.beforePUT(data);
    }, this._data);
  },

  filterDownload: function Resource_filterDownload() {
    this._data = this._filters.reduceRight(function(data, filter) {
      return filter.afterGET(data);
    }, this._data);
  },

  _request: function Res__request(action, data) {
    let iter = 0;
    let channel = this._createRequest();

    if ("undefined" != typeof(data))
      this._data = data;

    if ("PUT" == action || "POST" == action) {
      this.filterUpload();
      this._log.trace(action + " Body:\n" + this._data);

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
    this._data = chanOpen(listener, null);

    if (!channel.requestSucceeded) {
      this._log.debug(action + " request failed (" + channel.responseStatus + ")");
      if (this._data)
        this._log.debug("Error response: \n" + this._data);
      throw new RequestException(this, action, channel);

    } else {
      this._log.debug(action + " request successful (" + channel.responseStatus  + ")");

      switch (action) {
      case "DELETE":
        if (Utils.checkStatus(channel.responseStatus, null, [[200,300],404])){
          this._dirty = false;
          this._data = null;
        }
        break;
      case "GET":
      case "POST":
        this._log.trace(action + " Body:\n" + this._data);
        this.filterDownload();
        break;
      }
    }

    return this._data;
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
    this._log.debug(channel.requestMethod + " request for " +
      channel.URI.spec);
    this._data = '';
  },

  onStopRequest: function Channel_onStopRequest(channel, ctx, time) {
    if (this._data == '')
      this._data = null;

    this._onComplete(this._data);
  },

  onDataAvailable: function Channel_onDataAvail(req, cb, stream, off, count) {
    this._onProgress();

    let siStream = Cc["@mozilla.org/scriptableinputstream;1"].
      createInstance(Ci.nsIScriptableInputStream);
    siStream.init(stream);

    this._data += siStream.read(count);
  }
};


function RecordParser(data) {
  this._data = data;
}
RecordParser.prototype = {
  getNextRecord: function RecordParser_getNextRecord() {
    let start;
    let bCount = 0;
    let done = false;

    for (let i = 1; i < this._data.length; i++) {
      if (this._data[i] == '{') {
        if (bCount == 0)
          start = i;
        bCount++;
      } else if (this._data[i] == '}') {
        bCount--;
        if (bCount == 0)
          done = true;
      }

      if (done) {
        let ret = this._data.substring(start, i + 1);
        this._data = this._data.substring(i + 1);
        return ret;
      }
    }

    return false;
  },

  append: function RecordParser_append(data) {
    this._data += data;
  }
};

function JsonFilter() {
  let level = "Debug";
  try { level = Utils.prefs.getCharPref("log.logger.network.jsonFilter"); }
  catch (e) {  }
  this._log = Log4Moz.repository.getLogger("Net.JsonFilter");
  this._log.level = Log4Moz.Level[level];
}
JsonFilter.prototype = {
  beforePUT: function JsonFilter_beforePUT(data) {
    this._log.trace("Encoding data as JSON");
    return JSON.stringify(data);
  },

  afterGET: function JsonFilter_afterGET(data) {
    this._log.trace("Decoding JSON data");
    return JSON.parse(data);
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
