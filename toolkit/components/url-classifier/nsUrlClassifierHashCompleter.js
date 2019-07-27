



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;




const COMPLETE_LENGTH = 32;
const PARTIAL_LENGTH = 4;









const BACKOFF_ERRORS = 2;
const BACKOFF_INTERVAL = 30 * 60 * 1000;
const BACKOFF_MAX = 8 * 60 * 60 * 1000;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function HashCompleter() {
  
  
  
  this._currentRequest = null;
  
  this._pendingRequests = {};

  
  this._backoffs = {};

  
  this._shuttingDown = false;

  Services.obs.addObserver(this, "xpcom-shutdown", true);
}

HashCompleter.prototype = {
  classID: Components.ID("{9111de73-9322-4bfc-8b65-2b727f3e6ec8}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUrlClassifierHashCompleter,
                                         Ci.nsIRunnable,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsITimerCallback,
                                         Ci.nsISupports]),

  
  
  
  complete: function HC_complete(aPartialHash, aGethashUrl, aCallback) {
    if (!aGethashUrl) {
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    }

    if (!this._currentRequest) {
      this._currentRequest = new HashCompleterRequest(this, aGethashUrl);
    }
    if (this._currentRequest.gethashUrl == aGethashUrl) {
      this._currentRequest.add(aPartialHash, aCallback);
    } else {
      if (!this._pendingRequests[aGethashUrl]) {
        this._pendingRequests[aGethashUrl] =
          new HashCompleterRequest(this, aGethashUrl);
      }
      this._pendingRequests[aGethashUrl].add(aPartialHash, aCallback);
    }

    if (!this._backoffs[aGethashUrl]) {
      
      
      var jslib = Cc["@mozilla.org/url-classifier/jslib;1"]
                  .getService().wrappedJSObject;
      this._backoffs[aGethashUrl] = new jslib.RequestBackoff(
        BACKOFF_ERRORS ,
        60*1000 ,
        10 ,
        0 ,
        BACKOFF_INTERVAL ,
        BACKOFF_MAX );
    }
    
    
    Services.tm.currentThread.dispatch(this, Ci.nsIThread.DISPATCH_NORMAL);
  },

  
  
  
  run: function() {
    
    if (this._shuttingDown) {
      this._currentRequest = null;
      this._pendingRequests = null;
      for (var url in this._backoffs) {
        this._backoffs[url] = null;
      }
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    }

    
    let pendingUrls = Object.keys(this._pendingRequests);
    if (!this._currentRequest && (pendingUrls.length > 0)) {
      let nextUrl = pendingUrls[0];
      this._currentRequest = this._pendingRequests[nextUrl];
      delete this._pendingRequests[nextUrl];
    }

    if (this._currentRequest) {
      try {
        this._currentRequest.begin();
      } finally {
        
        this._currentRequest = null;
      }
    }
  },

  
  
  finishRequest: function(url, aStatus) {
    this._backoffs[url].noteServerResponse(aStatus);
    Services.tm.currentThread.dispatch(this, Ci.nsIThread.DISPATCH_NORMAL);
  },

  
  canMakeRequest: function(aGethashUrl) {
    return this._backoffs[aGethashUrl].canMakeRequest();
  },

  
  
  
  noteRequest: function(aGethashUrl) {
    return this._backoffs[aGethashUrl].noteRequest();
  },

  observe: function HC_observe(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      this._shuttingDown = true;
    }
  },
};

function HashCompleterRequest(aCompleter, aGethashUrl) {
  
  this._completer = aCompleter;
  
  this._requests = [];
  
  this._channel = null;
  
  this._response = "";
  
  this._shuttingDown = false;
  this.gethashUrl = aGethashUrl;
}
HashCompleterRequest.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRequestObserver,
                                         Ci.nsIStreamListener,
                                         Ci.nsIObserver,
                                         Ci.nsISupports]),

  
  
  add: function HCR_add(aPartialHash, aCallback) {
    this._requests.push({
      partialHash: aPartialHash,
      callback: aCallback,
      responses: [],
    });
  },

  
  
  
  begin: function HCR_begin() {
    if (!this._completer.canMakeRequest(this.gethashUrl)) {
      dump("hashcompleter: Can't make request to " + this.gethashUrl + "\n");
      this.notifyFailure(Cr.NS_ERROR_ABORT);
      return;
    }

    Services.obs.addObserver(this, "xpcom-shutdown", false);

    try {
      this.openChannel();
      
      
      this._completer.noteRequest(this.gethashUrl);
    }
    catch (err) {
      this.notifyFailure(err);
      throw err;
    }
  },

  notify: function HCR_notify() {
    
    
    
    if (this._channel && this._channel.isPending()) {
      dump("hashcompleter: cancelling request to " + this.gethashUrl + "\n");
      this._channel.cancel(Cr.NS_BINDING_ABORTED);
    }
  },

  
  openChannel: function HCR_openChannel() {
    let loadFlags = Ci.nsIChannel.INHIBIT_CACHING |
                    Ci.nsIChannel.LOAD_BYPASS_CACHE;

    let uri = Services.io.newURI(this.gethashUrl, null, null);
    let channel = Services.io.newChannelFromURI(uri);
    channel.loadFlags = loadFlags;

    this._channel = channel;

    let body = this.buildRequest();
    this.addRequestBody(body);

    
    
    this.timer_ = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    let timeout = Services.prefs.getIntPref(
      "urlclassifier.gethash.timeout_ms");
    this.timer_.initWithCallback(this, timeout, this.timer_.TYPE_ONE_SHOT);
    channel.asyncOpen(this, null);
  },

  
  
  buildRequest: function HCR_buildRequest() {
    
    
    let prefixes = [];

    for (let i = 0; i < this._requests.length; i++) {
      let request = this._requests[i];
      if (prefixes.indexOf(request.partialHash) == -1) {
        prefixes.push(request.partialHash);
      }
    }

    
    
    let i = prefixes.length;
    while (i--) {
      let j = Math.floor(Math.random() * (i + 1));
      let temp = prefixes[i];
      prefixes[i] = prefixes[j];
      prefixes[j] = temp;
    }

    let body;
    body = PARTIAL_LENGTH + ":" + (PARTIAL_LENGTH * prefixes.length) +
           "\n" + prefixes.join("");

    return body;
  },

  
  addRequestBody: function HCR_addRequestBody(aBody) {
    let inputStream = Cc["@mozilla.org/io/string-input-stream;1"].
                      createInstance(Ci.nsIStringInputStream);

    inputStream.setData(aBody, aBody.length);

    let uploadChannel = this._channel.QueryInterface(Ci.nsIUploadChannel);
    uploadChannel.setUploadStream(inputStream, "text/plain", -1);

    let httpChannel = this._channel.QueryInterface(Ci.nsIHttpChannel);
    httpChannel.requestMethod = "POST";
  },

  
  
  handleResponse: function HCR_handleResponse() {
    if (this._response == "") {
      return;
    }

    let start = 0;

    let length = this._response.length;
    while (start != length) {
      start = this.handleTable(start);
    }
  },

  
  
  handleTable: function HCR_handleTable(aStart) {
    let body = this._response.substring(aStart);

    
    
    let newlineIndex = body.indexOf("\n");
    if (newlineIndex == -1) {
      throw errorWithStack();
    }
    let header = body.substring(0, newlineIndex);
    let entries = header.split(":");
    if (entries.length != 3) {
      throw errorWithStack();
    }

    let list = entries[0];
    let addChunk = parseInt(entries[1]);
    let dataLength = parseInt(entries[2]);

    if (dataLength % COMPLETE_LENGTH != 0 ||
        dataLength == 0 ||
        dataLength > body.length - (newlineIndex + 1)) {
      throw errorWithStack();
    }

    let data = body.substr(newlineIndex + 1, dataLength);
    for (let i = 0; i < (dataLength / COMPLETE_LENGTH); i++) {
      this.handleItem(data.substr(i * COMPLETE_LENGTH, COMPLETE_LENGTH), list,
                      addChunk);
    }

    return aStart + newlineIndex + 1 + dataLength;
  },

  
  
  handleItem: function HCR_handleItem(aData, aTableName, aChunkId) {
    for (let i = 0; i < this._requests.length; i++) {
      let request = this._requests[i];
      if (aData.substring(0,4) == request.partialHash) {
        request.responses.push({
          completeHash: aData,
          tableName: aTableName,
          chunkId: aChunkId,
        });
      }
    }
  },

  
  
  
  
  notifySuccess: function HCR_notifySuccess() {
    for (let i = 0; i < this._requests.length; i++) {
      let request = this._requests[i];
      for (let j = 0; j < request.responses.length; j++) {
        let response = request.responses[j];
        request.callback.completion(response.completeHash, response.tableName,
                                    response.chunkId);
      }

      request.callback.completionFinished(Cr.NS_OK);
    }
  },
  notifyFailure: function HCR_notifyFailure(aStatus) {
    dump("hashcompleter: notifying failure\n");
    for (let i = 0; i < this._requests.length; i++) {
      let request = this._requests[i];
      request.callback.completionFinished(aStatus);
    }
  },

  onDataAvailable: function HCR_onDataAvailable(aRequest, aContext,
                                                aInputStream, aOffset, aCount) {
    let sis = Cc["@mozilla.org/scriptableinputstream;1"].
              createInstance(Ci.nsIScriptableInputStream);
    sis.init(aInputStream);
    this._response += sis.readBytes(aCount);
  },

  onStartRequest: function HCR_onStartRequest(aRequest, aContext) {
    
    
  },

  onStopRequest: function HCR_onStopRequest(aRequest, aContext, aStatusCode) {
    Services.obs.removeObserver(this, "xpcom-shutdown");

    if (this._shuttingDown) {
      throw Cr.NS_ERROR_ABORT;
    }

    
    
    let httpStatus = 503;
    if (Components.isSuccessCode(aStatusCode)) {
      let channel = aRequest.QueryInterface(Ci.nsIHttpChannel);
      let success = channel.requestSucceeded;
      httpStatus = channel.responseStatus;
      if (!success) {
        aStatusCode = Cr.NS_ERROR_ABORT;
      }
    }

    let success = Components.isSuccessCode(aStatusCode);
    
    this._completer.finishRequest(this.gethashUrl, httpStatus);

    if (success) {
      try {
        this.handleResponse();
      }
      catch (err) {
        dump(err.stack);
        aStatusCode = err.value;
        success = false;
      }
    }

    if (success) {
      this.notifySuccess();
    } else {
      this.notifyFailure(aStatusCode);
    }
  },

  observe: function HCR_observe(aSubject, aTopic, aData) {
    if (aTopic != "xpcom-shutdown") {
      return;
    }

    this._shuttingDown = true;
    if (this._channel) {
      this._channel.cancel(Cr.NS_ERROR_ABORT);
    }
  },
};




function unUrlsafeBase64(aStr) {
  return !aStr ? "" : aStr.replace(/-/g, "+")
                          .replace(/_/g, "/");
}

function errorWithStack() {
  let err = new Error();
  err.value = Cr.NS_ERROR_FAILURE;
  return err;
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([HashCompleter]);
