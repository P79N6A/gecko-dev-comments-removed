






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;




const COMPLETE_LENGTH = 32;
const PARTIAL_LENGTH = 4;











const BACKOFF_ERRORS = 2;
const BACKOFF_INTERVAL = 30 * 60;
const BACKOFF_MAX = 8 * 60 * 60;
const BACKOFF_TIME = 5 * 60;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
let keyFactory = Cc["@mozilla.org/security/keyobjectfactory;1"]
                   .getService(Ci.nsIKeyObjectFactory);

function HashCompleter() {
  
  
  
  this._currentRequest = null;

  
  
  this._clientKey = "";
  
  
  this._wrappedKey = "";

  
  this._shuttingDown = false;

  
  
  
  
  
  this._backoff = false;
  
  
  this._backoffTime = 0;
  
  
  this._nextRequestTime = 0;
  
  
  
  this._errorTimes = [];

  Services.obs.addObserver(this, "xpcom-shutdown", true);
}
HashCompleter.prototype = {
  classID: Components.ID("{9111de73-9322-4bfc-8b65-2b727f3e6ec8}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUrlClassifierHashCompleter,
                                         Ci.nsIRunnable,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  
  
  
  complete: function HC_complete(aPartialHash, aCallback) {
    if (!this._currentRequest) {
      this._currentRequest = new HashCompleterRequest(this);

      
      if (this._getHashUrl) {
        Services.tm.currentThread.dispatch(this, Ci.nsIThread.DISPATCH_NORMAL);
      }
    }

    this._currentRequest.add(aPartialHash, aCallback);
  },

  
  
  
  run: function HC_run() {
    if (this._shuttingDown) {
      this._currentRequest = null;
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    }

    if (!this._currentRequest) {
      return;
    }

    if (!this._getHashUrl) {
      throw Cr.NS_ERROR_NOT_INITIALIZED;
    }

    let url = this._getHashUrl;
    if (this._clientKey) {
      this._currentRequest.clientKey = this._clientKey;
      url += "&wrkey=" + this._wrappedKey;
    }

    let uri = Services.io.newURI(url, null, null);
    this._currentRequest.setURI(uri);

    
    try {
      this._currentRequest.begin();
    }
    finally {
      this._currentRequest = null;
    }
  },

  
  
  
  
  rekeyRequested: function HC_rekeyRequested() {
    this.setKeys("", "");

    Services.obs.notifyObservers(this, "url-classifier-rekey-requested", null);
  },

  
  
  
  setKeys: function HC_setKeys(aClientKey, aWrappedKey) {
    if (aClientKey == "") {
      this._clientKey = "";
      this._wrappedKey = "";
      return;
    }

    
    
    this._clientKey = atob(unUrlsafeBase64(aClientKey));
    this._wrappedKey = aWrappedKey;
  },

  get gethashUrl() {
    return this._getHashUrl;
  },
  
  
  set gethashUrl(aNewUrl) {
    this._getHashUrl = aNewUrl;

    if (this._currentRequest) {
      Services.tm.currentThread.dispatch(this, Ci.nsIThread.DISPATCH_NORMAL);
    }
  },

  
  
  
  
  
  
  
  
  
  
  noteServerResponse: function HC_noteServerResponse(aSuccess) {
    if (aSuccess) {
      this._backoff = false;
      this._nextRequestTime = 0;
      this._backoffTime = 0;
      return;
    }

    let now = Date.now();

    
    
    this._errorTimes.push(now);
    if (this._errorTimes.length > BACKOFF_ERRORS) {
      this._errorTimes.shift();
    }

    if (this._backoff) {
      this._backoffTime *= 2;
    } else if (this._errorTimes.length == BACKOFF_ERRORS &&
               ((now - this._errorTimes[0]) / 1000) <= BACKOFF_TIME) {
      this._backoff = true;
      this._backoffTime = BACKOFF_INTERVAL;
    }

    if (this._backoff) {
      this._backoffTime = Math.min(this._backoffTime, BACKOFF_MAX);
      this._nextRequestTime = now + (this._backoffTime * 1000);
    }
  },

  
  
  get nextRequestTime() {
    return this._nextRequestTime;
  },

  observe: function HC_observe(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      this._shuttingDown = true;
    }
  },
};

function HashCompleterRequest(aCompleter) {
  
  this._completer = aCompleter;
  
  this._requests = [];
  
  
  this._uri = null;
  
  this._channel = null;
  
  this._response = "";
  
  this._clientKey = "";
  
  
  this._rescheduled = false;
  
  
  this._verified = false;
  
  this._shuttingDown = false;
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
    if (Date.now() < this._completer.nextRequestTime) {
      this.notifyFailure(Cr.NS_ERROR_ABORT);
      return;
    }

    Services.obs.addObserver(this, "xpcom-shutdown", false);

    try {
      this.openChannel();
    }
    catch (err) {
      this.notifyFailure(err);
      throw err;
    }
  },

  setURI: function HCR_setURI(aURI) {
    this._uri = aURI;
  },

  
  openChannel: function HCR_openChannel() {
    let loadFlags = Ci.nsIChannel.INHIBIT_CACHING |
                    Ci.nsIChannel.LOAD_BYPASS_CACHE;

    let channel = Services.io.newChannelFromURI(this._uri);
    channel.loadFlags = loadFlags;

    this._channel = channel;

    let body = this.buildRequest();
    this.addRequestBody(body);

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
    if (this._clientKey) {
      start = this.handleMAC(start);

      if (this._rescheduled) {
        return;
      }
    }

    let length = this._response.length;
    while (start != length)
      start = this.handleTable(start);
  },

  
  
  
  handleMAC: function HCR_handleMAC(aStart) {
    this._verified = false;

    let body = this._response.substring(aStart);

    
    
    
    let newlineIndex = body.indexOf("\n");
    if (newlineIndex == -1) {
      throw errorWithStack();
    }

    let serverMAC = body.substring(0, newlineIndex);
    if (serverMAC == "e:pleaserekey") {
      this.rescheduleItems();

      this._completer.rekeyRequested();
      return this._response.length;
    }

    serverMAC = unUrlsafeBase64(serverMAC);

    let keyObject = keyFactory.keyFromString(Ci.nsIKeyObject.HMAC,
                                             this._clientKey);

    let data = body.substring(newlineIndex + 1).split("")
                                              .map(function(x) x.charCodeAt(0));

    let hmac = Cc["@mozilla.org/security/hmac;1"]
                 .createInstance(Ci.nsICryptoHMAC);
    hmac.init(Ci.nsICryptoHMAC.SHA1, keyObject);
    hmac.update(data, data.length);
    let clientMAC = hmac.finish(true);

    if (clientMAC != serverMAC) {
      throw errorWithStack();
    }

    this._verified = true;

    return aStart + newlineIndex + 1;
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
                                    response.chunkId, this._verified);
      }

      request.callback.completionFinished(Cr.NS_OK);
    }
  },
  notifyFailure: function HCR_notifyFailure(aStatus) {
    for (let i = 0; i < this._requests; i++) {
      let request = this._requests[i];
      request.callback.completionFinished(aStatus);
    }
  },

  
  
  
  rescheduleItems: function HCR_rescheduleItems() {
    for (let i = 0; i < this._requests[i]; i++) {
      let request = this._requests[i];
      try {
        this._completer.complete(request.partialHash, request.callback);
      }
      catch (err) {
        request.callback.completionFinished(err);
      }
    }

    this._rescheduled = true;
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

    if (Components.isSuccessCode(aStatusCode)) {
      let channel = aRequest.QueryInterface(Ci.nsIHttpChannel);
      let success = channel.requestSucceeded;
      if (!success) {
        aStatusCode = Cr.NS_ERROR_ABORT;
      }
    }

    let success = Components.isSuccessCode(aStatusCode);
    this._completer.noteServerResponse(success);

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

    if (!this._rescheduled) {
      if (success) {
        this.notifySuccess();
      } else {
        this.notifyFailure(aStatusCode);
      }
    }
  },

  set clientKey(aVal) {
    this._clientKey = aVal;
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

var NSGetFactory = XPCOMUtils.generateNSGetFactory([HashCompleter]);
