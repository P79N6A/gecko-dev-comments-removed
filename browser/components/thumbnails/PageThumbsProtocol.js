
















"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Cr = Components.results;
const Ci = Components.interfaces;

Cu.import("resource:///modules/PageThumbs.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");




function Protocol() {
}

Protocol.prototype = {
  


  get scheme() PageThumbs.scheme,

  


  get defaultPort() -1,

  


  get protocolFlags() {
    return Ci.nsIProtocolHandler.URI_DANGEROUS_TO_LOAD |
           Ci.nsIProtocolHandler.URI_NORELATIVE |
           Ci.nsIProtocolHandler.URI_NOAUTH;
  },

  





  newURI: function Proto_newURI(aSpec, aOriginCharset) {
    let uri = Cc["@mozilla.org/network/simple-uri;1"].createInstance(Ci.nsIURI);
    uri.spec = aSpec;
    return uri;
  },

  




  newChannel: function Proto_newChannel(aURI) {
    return new Channel(aURI);
  },

  



  allowPort: function () false,

  classID: Components.ID("{5a4ae9b5-f475-48ae-9dce-0b4c1d347884}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler])
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([Protocol]);




function Channel(aURI) {
  this._uri = aURI;

  
  this.originalURI = aURI;

  
  this._responseHeaders = {"content-type": PageThumbs.contentType};
}

Channel.prototype = {
  


  _wasOpened: false,

  




  asyncOpen: function Channel_asyncOpen(aListener, aContext) {
    if (this._wasOpened)
      throw Cr.NS_ERROR_ALREADY_OPENED;

    if (this.canceled)
      return;

    this._listener = aListener;
    this._context = aContext;

    this._isPending = true;
    this._wasOpened = true;

    
    this._readCache(function (aData) {
      let telemetryThumbnailFound = true;

      
      if (!aData) {
        this._responseStatus = 404;
        this._responseText = "Not Found";
        telemetryThumbnailFound = false;
      }

      Services.telemetry.getHistogramById("FX_THUMBNAILS_HIT_OR_MISS")
        .add(telemetryThumbnailFound);

      this._startRequest();

      if (!this.canceled) {
        this._addToLoadGroup();

        if (aData)
          this._serveData(aData);

        if (!this.canceled)
          this._stopRequest();
      }
    }.bind(this));
  },

  



  _readCache: function Channel_readCache(aCallback) {
    let {url} = parseURI(this._uri);

    
    if (!url) {
      aCallback(null);
      return;
    }

    
    PageThumbsCache.getReadEntry(url, function (aEntry) {
      let inputStream = aEntry && aEntry.openInputStream(0);

      function closeEntryAndFinish(aData) {
        if (aEntry) {
          aEntry.close();
        }
        aCallback(aData);
      }

      
      if (!inputStream || !inputStream.available()) {
        closeEntryAndFinish();
        return;
      }

      try {
        
        NetUtil.asyncFetch(inputStream, function (aData, aStatus) {
          
          if (this.canceled)
            return;

          
          if (!Components.isSuccessCode(aStatus) || !aData.available())
            aData = null;

          closeEntryAndFinish(aData);
        }.bind(this));
      } catch (e) {
        closeEntryAndFinish();
      }
    }.bind(this));
  },

  


  _startRequest: function Channel_startRequest() {
    try {
      this._listener.onStartRequest(this, this._context);
    } catch (e) {
      
      this.cancel(Cr.NS_BINDING_ABORTED);
    }
  },

  



  _serveData: function Channel_serveData(aData) {
    try {
      let available = aData.available();
      this._listener.onDataAvailable(this, this._context, aData, 0, available);
    } catch (e) {
      
      this.cancel(Cr.NS_BINDING_ABORTED);
    }
  },

  


  _stopRequest: function Channel_stopRequest() {
    try {
      this._listener.onStopRequest(this, this._context, this.status);
    } catch (e) {
      
    }

    
    this._cleanup();
  },

  


  _addToLoadGroup: function Channel_addToLoadGroup() {
    if (this.loadGroup)
      this.loadGroup.addRequest(this, this._context);
  },

  


  _removeFromLoadGroup: function Channel_removeFromLoadGroup() {
    if (!this.loadGroup)
      return;

    try {
      this.loadGroup.removeRequest(this, this._context, this.status);
    } catch (e) {
      
    }
  },

  


  _cleanup: function Channel_cleanup() {
    this._removeFromLoadGroup();
    this.loadGroup = null;

    this._isPending = false;

    delete this._listener;
    delete this._context;
  },

  

  contentType: PageThumbs.contentType,
  contentLength: -1,
  owner: null,
  contentCharset: null,
  notificationCallbacks: null,

  get URI() this._uri,
  get securityInfo() null,

  


  open: function Channel_open() {
    
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  

  redirectionLimit: 10,
  requestMethod: "GET",
  allowPipelining: true,
  referrer: null,

  get requestSucceeded() true,

  _responseStatus: 200,
  get responseStatus() this._responseStatus,

  _responseText: "OK",
  get responseStatusText() this._responseText,

  




  isNoCacheResponse: function () false,

  




  isNoStoreResponse: function () false,

  


  getRequestHeader: function Channel_getRequestHeader() {
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  



  setRequestHeader: function Channel_setRequestHeader() {
    if (this._wasOpened)
      throw Cr.NS_ERROR_IN_PROGRESS;
  },

  


  visitRequestHeaders: function () {},

  




  getResponseHeader: function Channel_getResponseHeader(aHeader) {
    let name = aHeader.toLowerCase();
    if (name in this._responseHeaders)
      return this._responseHeaders[name];

    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  




  setResponseHeader: function Channel_setResponseHeader(aHeader, aValue, aMerge) {
    let name = aHeader.toLowerCase();
    if (!aValue && !aMerge)
      delete this._responseHeaders[name];
    else
      this._responseHeaders[name] = aValue;
  },

  



  visitResponseHeaders: function Channel_visitResponseHeaders(aVisitor) {
    for (let name in this._responseHeaders) {
      let value = this._responseHeaders[name];

      try {
        aVisitor.visitHeader(name, value);
      } catch (e) {
        
        return;
      }
    }
  },

  

  loadFlags: Ci.nsIRequest.LOAD_NORMAL,
  loadGroup: null,

  get name() this._uri.spec,

  _status: Cr.NS_OK,
  get status() this._status,

  _isPending: false,
  isPending: function () this._isPending,

  resume: function () {},
  suspend: function () {},

  



  cancel: function Channel_cancel(aStatus) {
    if (this.canceled)
      return;

    this._isCanceled = true;
    this._status = aStatus;

    this._cleanup();
  },

  

  documentURI: null,

  _isCanceled: false,
  get canceled() this._isCanceled,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIChannel,
                                         Ci.nsIHttpChannel,
                                         Ci.nsIHttpChannelInternal,
                                         Ci.nsIRequest])
};






function parseURI(aURI) {
  let {scheme, staticHost} = PageThumbs;
  let re = new RegExp("^" + scheme + "://" + staticHost + ".*?\\?");
  let query = aURI.spec.replace(re, "");
  let params = {};

  query.split("&").forEach(function (aParam) {
    let [key, value] = aParam.split("=").map(decodeURIComponent);
    params[key.toLowerCase()] = value;
  });

  return params;
}
