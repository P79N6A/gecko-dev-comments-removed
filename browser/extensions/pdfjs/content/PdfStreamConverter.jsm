



















'use strict';

var EXPORTED_SYMBOLS = ['PdfStreamConverter'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const MOZ_CENTRAL = JSON.parse('true');
const PDFJS_EVENT_ID = 'pdf.js.message';
const PDF_CONTENT_TYPE = 'application/pdf';
const PREF_PREFIX = 'pdfjs';
const PDF_VIEWER_WEB_PAGE = 'resource://pdf.js/web/viewer.html';
const MAX_NUMBER_OF_PREFS = 50;
const MAX_STRING_PREF_LENGTH = 128;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/NetUtil.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'NetworkManager',
  'resource://pdf.js/network.js');

XPCOMUtils.defineLazyModuleGetter(this, 'PrivateBrowsingUtils',
  'resource://gre/modules/PrivateBrowsingUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'PdfJsTelemetry',
  'resource://pdf.js/PdfJsTelemetry.jsm');

var Svc = {};
XPCOMUtils.defineLazyServiceGetter(Svc, 'mime',
                                   '@mozilla.org/mime;1',
                                   'nsIMIMEService');

function getContainingBrowser(domWindow) {
  return domWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation)
                  .QueryInterface(Ci.nsIDocShell)
                  .chromeEventHandler;
}

function getChromeWindow(domWindow) {
  return getContainingBrowser(domWindow).ownerDocument.defaultView;
}

function getFindBar(domWindow) {
  var browser = getContainingBrowser(domWindow);
  try {
    var tabbrowser = browser.getTabBrowser();
    var tab = tabbrowser._getTabForBrowser(browser);
    return tabbrowser.getFindBar(tab);
  } catch (e) {
    
    var chromeWindow = browser.ownerDocument.defaultView;
    return chromeWindow.gFindBar;
  }
}

function setBoolPref(pref, value) {
  Services.prefs.setBoolPref(pref, value);
}

function getBoolPref(pref, def) {
  try {
    return Services.prefs.getBoolPref(pref);
  } catch (ex) {
    return def;
  }
}

function setIntPref(pref, value) {
  Services.prefs.setIntPref(pref, value);
}

function getIntPref(pref, def) {
  try {
    return Services.prefs.getIntPref(pref);
  } catch (ex) {
    return def;
  }
}

function setStringPref(pref, value) {
  var str = Cc['@mozilla.org/supports-string;1']
              .createInstance(Ci.nsISupportsString);
  str.data = value;
  Services.prefs.setComplexValue(pref, Ci.nsISupportsString, str);
}

function getStringPref(pref, def) {
  try {
    return Services.prefs.getComplexValue(pref, Ci.nsISupportsString).data;
  } catch (ex) {
    return def;
  }
}

function log(aMsg) {
  if (!getBoolPref(PREF_PREFIX + '.pdfBugEnabled', false))
    return;
  var msg = 'PdfStreamConverter.js: ' + (aMsg.join ? aMsg.join('') : aMsg);
  Services.console.logStringMessage(msg);
  dump(msg + '\n');
}

function getDOMWindow(aChannel) {
  var requestor = aChannel.notificationCallbacks ?
                  aChannel.notificationCallbacks :
                  aChannel.loadGroup.notificationCallbacks;
  var win = requestor.getInterface(Components.interfaces.nsIDOMWindow);
  return win;
}

function getLocalizedStrings(path) {
  var stringBundle = Cc['@mozilla.org/intl/stringbundle;1'].
      getService(Ci.nsIStringBundleService).
      createBundle('chrome://pdf.js/locale/' + path);

  var map = {};
  var enumerator = stringBundle.getSimpleEnumeration();
  while (enumerator.hasMoreElements()) {
    var string = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);
    var key = string.key, property = 'textContent';
    var i = key.lastIndexOf('.');
    if (i >= 0) {
      property = key.substring(i + 1);
      key = key.substring(0, i);
    }
    if (!(key in map))
      map[key] = {};
    map[key][property] = string.value;
  }
  return map;
}
function getLocalizedString(strings, id, property) {
  property = property || 'textContent';
  if (id in strings)
    return strings[id][property];
  return id;
}

function makeContentReadable(obj, window) {
  return Cu.cloneInto(obj, window);
}


function PdfDataListener(length) {
  this.length = length; 
  this.data = new Uint8Array(length >= 0 ? length : 0x10000);
  this.loaded = 0;
}

PdfDataListener.prototype = {
  append: function PdfDataListener_append(chunk) {
    var willBeLoaded = this.loaded + chunk.length;
    if (this.length >= 0 && this.length < willBeLoaded) {
      this.length = -1; 
    }
    if (this.length < 0 && this.data.length < willBeLoaded) {
      
      
      var newLength = this.data.length;
      for (; newLength < willBeLoaded; newLength *= 2) {}
      var newData = new Uint8Array(newLength);
      newData.set(this.data);
      this.data = newData;
    }
    this.data.set(chunk, this.loaded);
    this.loaded = willBeLoaded;
    this.onprogress(this.loaded, this.length >= 0 ? this.length : void(0));
  },
  getData: function PdfDataListener_getData() {
    var data = this.data;
    if (this.loaded != data.length)
      data = data.subarray(0, this.loaded);
    delete this.data; 
    return data;
  },
  finish: function PdfDataListener_finish() {
    this.isDataReady = true;
    if (this.oncompleteCallback) {
      this.oncompleteCallback(this.getData());
    }
  },
  error: function PdfDataListener_error(errorCode) {
    this.errorCode = errorCode;
    if (this.oncompleteCallback) {
      this.oncompleteCallback(null, errorCode);
    }
  },
  onprogress: function() {},
  get oncomplete() {
    return this.oncompleteCallback;
  },
  set oncomplete(value) {
    this.oncompleteCallback = value;
    if (this.isDataReady) {
      value(this.getData());
    }
    if (this.errorCode) {
      value(null, this.errorCode);
    }
  }
};


function ChromeActions(domWindow, contentDispositionFilename) {
  this.domWindow = domWindow;
  this.contentDispositionFilename = contentDispositionFilename;
  this.telemetryState = {
    documentInfo: false,
    firstPageInfo: false,
    streamTypesUsed: [],
    fontTypesUsed: [],
    startAt: Date.now()
  };
}

ChromeActions.prototype = {
  isInPrivateBrowsing: function() {
    return PrivateBrowsingUtils.isWindowPrivate(this.domWindow);
  },
  download: function(data, sendResponse) {
    var self = this;
    var originalUrl = data.originalUrl;
    
    
    var originalUri = NetUtil.newURI(data.originalUrl);
    var filename = data.filename;
    if (typeof filename !== 'string' || 
        (!/\.pdf$/i.test(filename) && !data.isAttachment)) {
      filename = 'document.pdf';
    }
    var blobUri = data.blobUrl ? NetUtil.newURI(data.blobUrl) : originalUri;
    var extHelperAppSvc =
          Cc['@mozilla.org/uriloader/external-helper-app-service;1'].
             getService(Ci.nsIExternalHelperAppService);
    var frontWindow = Cc['@mozilla.org/embedcomp/window-watcher;1'].
                         getService(Ci.nsIWindowWatcher).activeWindow;

    var docIsPrivate = this.isInPrivateBrowsing();
    var netChannel = NetUtil.newChannel(blobUri);
    if ('nsIPrivateBrowsingChannel' in Ci &&
        netChannel instanceof Ci.nsIPrivateBrowsingChannel) {
      netChannel.setPrivate(docIsPrivate);
    }
    NetUtil.asyncFetch(netChannel, function(aInputStream, aResult) {
      if (!Components.isSuccessCode(aResult)) {
        if (sendResponse)
          sendResponse(true);
        return;
      }
      
      
      var channel = Cc['@mozilla.org/network/input-stream-channel;1'].
                       createInstance(Ci.nsIInputStreamChannel);
      channel.QueryInterface(Ci.nsIChannel);
      try {
        
        channel.contentDisposition = Ci.nsIChannel.DISPOSITION_ATTACHMENT;
        if (self.contentDispositionFilename) {
          channel.contentDispositionFilename = self.contentDispositionFilename;
        } else {
          channel.contentDispositionFilename = filename;
        }
      } catch (e) {}
      channel.setURI(originalUri);
      channel.contentStream = aInputStream;
      if ('nsIPrivateBrowsingChannel' in Ci &&
          channel instanceof Ci.nsIPrivateBrowsingChannel) {
        channel.setPrivate(docIsPrivate);
      }

      var listener = {
        extListener: null,
        onStartRequest: function(aRequest, aContext) {
          this.extListener = extHelperAppSvc.doContent(
            (data.isAttachment ? 'application/octet-stream' :
                                 'application/pdf'),
            aRequest, frontWindow, false);
          this.extListener.onStartRequest(aRequest, aContext);
        },
        onStopRequest: function(aRequest, aContext, aStatusCode) {
          if (this.extListener)
            this.extListener.onStopRequest(aRequest, aContext, aStatusCode);
          
          if (sendResponse)
            sendResponse(false);
        },
        onDataAvailable: function(aRequest, aContext, aInputStream, aOffset,
                                  aCount) {
          this.extListener.onDataAvailable(aRequest, aContext, aInputStream,
                                           aOffset, aCount);
        }
      };

      channel.asyncOpen(listener, null);
    });
  },
  getLocale: function() {
    return getStringPref('general.useragent.locale', 'en-US');
  },
  getStrings: function(data) {
    try {
      
      if (!('localizedStrings' in this))
        this.localizedStrings = getLocalizedStrings('viewer.properties');

      var result = this.localizedStrings[data];
      return JSON.stringify(result || null);
    } catch (e) {
      log('Unable to retrive localized strings: ' + e);
      return 'null';
    }
  },
  pdfBugEnabled: function() {
    return getBoolPref(PREF_PREFIX + '.pdfBugEnabled', false);
  },
  supportsIntegratedFind: function() {
    
    if (this.domWindow.frameElement !== null) {
      return false;
    }
    
    var findBar = getFindBar(this.domWindow);
    return findBar && ('updateControlState' in findBar);
  },
  supportsDocumentFonts: function() {
    var prefBrowser = getIntPref('browser.display.use_document_fonts', 1);
    var prefGfx = getBoolPref('gfx.downloadable_fonts.enabled', true);
    return (!!prefBrowser && prefGfx);
  },
  supportsDocumentColors: function() {
    return getBoolPref('browser.display.use_document_colors', true);
  },
  reportTelemetry: function (data) {
    var probeInfo = JSON.parse(data);
    switch (probeInfo.type) {
      case 'documentInfo':
        if (!this.telemetryState.documentInfo) {
          PdfJsTelemetry.onDocumentVersion(probeInfo.version | 0);
          PdfJsTelemetry.onDocumentGenerator(probeInfo.generator | 0);
          if (probeInfo.formType) {
            PdfJsTelemetry.onForm(probeInfo.formType === 'acroform');
          }
          this.telemetryState.documentInfo = true;
        }
        break;
      case 'pageInfo':
        if (!this.telemetryState.firstPageInfo) {
          var duration = Date.now() - this.telemetryState.startAt;
          PdfJsTelemetry.onTimeToView(duration);
          this.telemetryState.firstPageInfo = true;
        }
        break;
      case 'documentStats':
        
        
        
        var documentStats = probeInfo.stats;
        if (!documentStats || typeof documentStats !== 'object') {
          break;
        }
        var streamTypes = documentStats.streamTypes;
        if (Array.isArray(streamTypes)) {
          var STREAM_TYPE_ID_LIMIT = 20;
          for (var i = 0; i < STREAM_TYPE_ID_LIMIT; i++) {
            if (streamTypes[i] &&
                !this.telemetryState.streamTypesUsed[i]) {
              PdfJsTelemetry.onStreamType(i);
              this.telemetryState.streamTypesUsed[i] = true;
            }
          }
        }
        var fontTypes = documentStats.fontTypes;
        if (Array.isArray(fontTypes)) {
          var FONT_TYPE_ID_LIMIT = 20;
          for (var i = 0; i < FONT_TYPE_ID_LIMIT; i++) {
            if (fontTypes[i] &&
                !this.telemetryState.fontTypesUsed[i]) {
              PdfJsTelemetry.onFontType(i);
              this.telemetryState.fontTypesUsed[i] = true;
            }
          }
        }
        break;
      case 'print':
        PdfJsTelemetry.onPrint();
        break;
    }
  },
  fallback: function(args, sendResponse) {
    var featureId = args.featureId;
    var url = args.url;

    var self = this;
    var domWindow = this.domWindow;
    var strings = getLocalizedStrings('chrome.properties');
    var message;
    if (featureId === 'forms') {
      message = getLocalizedString(strings, 'unsupported_feature_forms');
    } else {
      message = getLocalizedString(strings, 'unsupported_feature');
    }

    PdfJsTelemetry.onFallback();

    var notificationBox = null;
    try {
      
      var mainWindow = domWindow
        .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIWebNavigation)
        .QueryInterface(Components.interfaces.nsIDocShellTreeItem)
        .rootTreeItem
        .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
        .getInterface(Components.interfaces.nsIDOMWindow);
      var browser = mainWindow.gBrowser
                              .getBrowserForDocument(domWindow.top.document);
      notificationBox = mainWindow.gBrowser.getNotificationBox(browser);
    } catch (e) {
      log('Unable to get a notification box for the fallback message');
      return;
    }

    
    
    
    var sentResponse = false;
    var buttons = [{
      label: getLocalizedString(strings, 'open_with_different_viewer'),
      accessKey: getLocalizedString(strings, 'open_with_different_viewer',
                                    'accessKey'),
      callback: function() {
        sentResponse = true;
        sendResponse(true);
      }
    }];
    notificationBox.appendNotification(message, 'pdfjs-fallback', null,
                                       notificationBox.PRIORITY_INFO_LOW,
                                       buttons,
                                       function eventsCallback(eventType) {
      
      
      if (eventType !== 'removed')
        return;
      
      
      if (!sentResponse)
        sendResponse(false);
    });
  },
  updateFindControlState: function(data) {
    if (!this.supportsIntegratedFind())
      return;
    
    var result = data.result;
    var findPrevious = data.findPrevious;
    var findPreviousType = typeof findPrevious;
    if ((typeof result !== 'number' || result < 0 || result > 3) ||
        (findPreviousType !== 'undefined' && findPreviousType !== 'boolean')) {
      return;
    }
    getFindBar(this.domWindow).updateControlState(result, findPrevious);
  },
  setPreferences: function(prefs, sendResponse) {
    var defaultBranch = Services.prefs.getDefaultBranch(PREF_PREFIX + '.');
    var numberOfPrefs = 0;
    var prefValue, prefName;
    for (var key in prefs) {
      if (++numberOfPrefs > MAX_NUMBER_OF_PREFS) {
        log('setPreferences - Exceeded the maximum number of preferences ' +
            'that is allowed to be set at once.');
        break;
      } else if (!defaultBranch.getPrefType(key)) {
        continue;
      }
      prefValue = prefs[key];
      prefName = (PREF_PREFIX + '.' + key);
      switch (typeof prefValue) {
        case 'boolean':
          setBoolPref(prefName, prefValue);
          break;
        case 'number':
          setIntPref(prefName, prefValue);
          break;
        case 'string':
          if (prefValue.length > MAX_STRING_PREF_LENGTH) {
            log('setPreferences - Exceeded the maximum allowed length ' +
                'for a string preference.');
          } else {
            setStringPref(prefName, prefValue);
          }
          break;
      }
    }
    if (sendResponse) {
      sendResponse(true);
    }
  },
  getPreferences: function(prefs, sendResponse) {
    var defaultBranch = Services.prefs.getDefaultBranch(PREF_PREFIX + '.');
    var currentPrefs = {}, numberOfPrefs = 0;
    var prefValue, prefName;
    for (var key in prefs) {
      if (++numberOfPrefs > MAX_NUMBER_OF_PREFS) {
        log('getPreferences - Exceeded the maximum number of preferences ' +
            'that is allowed to be fetched at once.');
        break;
      } else if (!defaultBranch.getPrefType(key)) {
        continue;
      }
      prefValue = prefs[key];
      prefName = (PREF_PREFIX + '.' + key);
      switch (typeof prefValue) {
        case 'boolean':
          currentPrefs[key] = getBoolPref(prefName, prefValue);
          break;
        case 'number':
          currentPrefs[key] = getIntPref(prefName, prefValue);
          break;
        case 'string':
          currentPrefs[key] = getStringPref(prefName, prefValue);
          break;
      }
    }
    if (sendResponse) {
      sendResponse(JSON.stringify(currentPrefs));
    } else {
      return JSON.stringify(currentPrefs);
    }
  }
};

var RangedChromeActions = (function RangedChromeActionsClosure() {
  


  function RangedChromeActions(
              domWindow, contentDispositionFilename, originalRequest,
              dataListener) {

    ChromeActions.call(this, domWindow, contentDispositionFilename);
    this.dataListener = dataListener;
    this.originalRequest = originalRequest;

    this.pdfUrl = originalRequest.URI.spec;
    this.contentLength = originalRequest.contentLength;

    
    var httpHeaderVisitor = {
      headers: {},
      visitHeader: function(aHeader, aValue) {
        if (aHeader === 'Range') {
          
          
          
          
          return;
        }
        this.headers[aHeader] = aValue;
      }
    };
    originalRequest.visitRequestHeaders(httpHeaderVisitor);

    var self = this;
    var xhr_onreadystatechange = function xhr_onreadystatechange() {
      if (this.readyState === 1) { 
        var netChannel = this.channel;
        if ('nsIPrivateBrowsingChannel' in Ci &&
            netChannel instanceof Ci.nsIPrivateBrowsingChannel) {
          var docIsPrivate = self.isInPrivateBrowsing();
          netChannel.setPrivate(docIsPrivate);
        }
      }
    };
    var getXhr = function getXhr() {
      const XMLHttpRequest = Components.Constructor(
          '@mozilla.org/xmlextras/xmlhttprequest;1');
      var xhr = new XMLHttpRequest();
      xhr.addEventListener('readystatechange', xhr_onreadystatechange);
      return xhr;
    };

    this.networkManager = new NetworkManager(this.pdfUrl, {
      httpHeaders: httpHeaderVisitor.headers,
      getXhr: getXhr
    });

    
    
    domWindow.addEventListener('unload', function unload(e) {
      self.networkManager.abortAllRequests();
      domWindow.removeEventListener(e.type, unload);
    });
  }

  RangedChromeActions.prototype = Object.create(ChromeActions.prototype);
  var proto = RangedChromeActions.prototype;
  proto.constructor = RangedChromeActions;

  proto.initPassiveLoading = function RangedChromeActions_initPassiveLoading() {
    this.originalRequest.cancel(Cr.NS_BINDING_ABORTED);
    this.originalRequest = null;
    this.domWindow.postMessage({
      pdfjsLoadAction: 'supportsRangedLoading',
      pdfUrl: this.pdfUrl,
      length: this.contentLength,
      data: this.dataListener.getData()
    }, '*');
    this.dataListener = null;

    return true;
  };

  proto.requestDataRange = function RangedChromeActions_requestDataRange(args) {
    var begin = args.begin;
    var end = args.end;
    var domWindow = this.domWindow;
    
    
    
    this.networkManager.requestRange(begin, end, {
      onDone: function RangedChromeActions_onDone(args) {
        domWindow.postMessage({
          pdfjsLoadAction: 'range',
          begin: args.begin,
          chunk: args.chunk
        }, '*');
      },
      onProgress: function RangedChromeActions_onProgress(evt) {
        domWindow.postMessage({
          pdfjsLoadAction: 'rangeProgress',
          loaded: evt.loaded,
        }, '*');
      }
    });
  };

  return RangedChromeActions;
})();

var StandardChromeActions = (function StandardChromeActionsClosure() {

  


  function StandardChromeActions(domWindow, contentDispositionFilename,
                                 dataListener) {

    ChromeActions.call(this, domWindow, contentDispositionFilename);
    this.dataListener = dataListener;
  }

  StandardChromeActions.prototype = Object.create(ChromeActions.prototype);
  var proto = StandardChromeActions.prototype;
  proto.constructor = StandardChromeActions;

  proto.initPassiveLoading =
      function StandardChromeActions_initPassiveLoading() {

    if (!this.dataListener) {
      return false;
    }

    var self = this;

    this.dataListener.onprogress = function ChromeActions_dataListenerProgress(
                                      loaded, total) {
      self.domWindow.postMessage({
        pdfjsLoadAction: 'progress',
        loaded: loaded,
        total: total
      }, '*');
    };

    this.dataListener.oncomplete = function ChromeActions_dataListenerComplete(
                                      data, errorCode) {
      self.domWindow.postMessage({
        pdfjsLoadAction: 'complete',
        data: data,
        errorCode: errorCode
      }, '*');

      delete self.dataListener;
    };

    return true;
  };

  return StandardChromeActions;
})();


function RequestListener(actions) {
  this.actions = actions;
}

RequestListener.prototype.receive = function(event) {
  var message = event.target;
  var doc = message.ownerDocument;
  var action = event.detail.action;
  var data = event.detail.data;
  var sync = event.detail.sync;
  var actions = this.actions;
  if (!(action in actions)) {
    log('Unknown action: ' + action);
    return;
  }
  if (sync) {
    var response = actions[action].call(this.actions, data);
    event.detail.response = response;
  } else {
    var response;
    if (!event.detail.responseExpected) {
      doc.documentElement.removeChild(message);
      response = null;
    } else {
      response = function sendResponse(response) {
        try {
          var listener = doc.createEvent('CustomEvent');
          let detail = makeContentReadable({response: response}, doc.defaultView);
          listener.initCustomEvent('pdf.js.response', true, false, detail);
          return message.dispatchEvent(listener);
        } catch (e) {
          
          
          return false;
        }
      };
    }
    actions[action].call(this.actions, data, response);
  }
};



function FindEventManager(eventElement, contentWindow, chromeWindow) {
  this.types = ['find',
                'findagain',
                'findhighlightallchange',
                'findcasesensitivitychange'];
  this.chromeWindow = chromeWindow;
  this.contentWindow = contentWindow;
  this.eventElement = eventElement;
}

FindEventManager.prototype.bind = function() {
  var unload = function(e) {
    this.unbind();
    this.contentWindow.removeEventListener(e.type, unload);
  }.bind(this);
  this.contentWindow.addEventListener('unload', unload);

  for (var i = 0; i < this.types.length; i++) {
    var type = this.types[i];
    this.eventElement.addEventListener(type, this, true);
  }
};

FindEventManager.prototype.handleEvent = function(e) {
  var chromeWindow = this.chromeWindow;
  var contentWindow = this.contentWindow;
  
  if (chromeWindow.gBrowser.selectedBrowser.contentWindow === contentWindow) {
    var detail = {
      query: e.detail.query,
      caseSensitive: e.detail.caseSensitive,
      highlightAll: e.detail.highlightAll,
      findPrevious: e.detail.findPrevious
    };
    detail = makeContentReadable(detail, contentWindow);
    var forward = contentWindow.document.createEvent('CustomEvent');
    forward.initCustomEvent(e.type, true, true, detail);
    contentWindow.dispatchEvent(forward);
    e.preventDefault();
  }
};

FindEventManager.prototype.unbind = function() {
  for (var i = 0; i < this.types.length; i++) {
    var type = this.types[i];
    this.eventElement.removeEventListener(type, this, true);
  }
};

function PdfStreamConverter() {
}

PdfStreamConverter.prototype = {

  
  classID: Components.ID('{d0c5195d-e798-49d4-b1d3-9324328b2291}'),
  classDescription: 'pdf.js Component',
  contractID: '@mozilla.org/streamconv;1?from=application/pdf&to=*/*',

  QueryInterface: XPCOMUtils.generateQI([
      Ci.nsISupports,
      Ci.nsIStreamConverter,
      Ci.nsIStreamListener,
      Ci.nsIRequestObserver
  ]),

  















  
  convert: function(aFromStream, aFromType, aToType, aCtxt) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  
  asyncConvertData: function(aFromType, aToType, aListener, aCtxt) {
    
    this.listener = aListener;
  },

  
  onDataAvailable: function(aRequest, aContext, aInputStream, aOffset, aCount) {
    if (!this.dataListener) {
      return;
    }

    var binaryStream = this.binaryStream;
    binaryStream.setInputStream(aInputStream);
    var chunk = binaryStream.readByteArray(aCount);
    this.dataListener.append(chunk);
  },

  
  onStartRequest: function(aRequest, aContext) {
    
    var isHttpRequest = false;
    try {
      aRequest.QueryInterface(Ci.nsIHttpChannel);
      isHttpRequest = true;
    } catch (e) {}

    var rangeRequest = false;
    if (isHttpRequest) {
      var contentEncoding = 'identity';
      try {
        contentEncoding = aRequest.getResponseHeader('Content-Encoding');
      } catch (e) {}

      var acceptRanges;
      try {
        acceptRanges = aRequest.getResponseHeader('Accept-Ranges');
      } catch (e) {}

      var hash = aRequest.URI.ref;
      rangeRequest = contentEncoding === 'identity' &&
                     acceptRanges === 'bytes' &&
                     aRequest.contentLength >= 0 &&
                     hash.indexOf('disableRange=true') < 0;
    }

    aRequest.QueryInterface(Ci.nsIChannel);

    aRequest.QueryInterface(Ci.nsIWritablePropertyBag);

    var contentDispositionFilename;
    try {
      contentDispositionFilename = aRequest.contentDispositionFilename;
    } catch (e) {}

    
    aRequest.setProperty('contentType', aRequest.contentType);
    aRequest.contentType = 'text/html';
    if (isHttpRequest) {
      
      aRequest.setResponseHeader('Content-Security-Policy', '', false);
      aRequest.setResponseHeader('Content-Security-Policy-Report-Only', '',
                                 false);
    }

    PdfJsTelemetry.onViewerIsUsed();
    PdfJsTelemetry.onDocumentSize(aRequest.contentLength);


    
    var contentLength = aRequest.contentLength;
    this.dataListener = new PdfDataListener(contentLength);
    this.binaryStream = Cc['@mozilla.org/binaryinputstream;1']
                        .createInstance(Ci.nsIBinaryInputStream);

    
    var ioService = Services.io;
    var channel = ioService.newChannel(
                    PDF_VIEWER_WEB_PAGE, null, null);

    var listener = this.listener;
    var dataListener = this.dataListener;
    
    
    
    
    var proxy = {
      onStartRequest: function(request, context) {
        listener.onStartRequest(aRequest, context);
      },
      onDataAvailable: function(request, context, inputStream, offset, count) {
        listener.onDataAvailable(aRequest, context, inputStream, offset, count);
      },
      onStopRequest: function(request, context, statusCode) {
        
        
        var domWindow = getDOMWindow(channel);
        var actions;
        if (rangeRequest) {
          actions = new RangedChromeActions(
              domWindow, contentDispositionFilename, aRequest, dataListener);
        } else {
          actions = new StandardChromeActions(
              domWindow, contentDispositionFilename, dataListener);
        }
        var requestListener = new RequestListener(actions);
        domWindow.addEventListener(PDFJS_EVENT_ID, function(event) {
          requestListener.receive(event);
        }, false, true);
        if (actions.supportsIntegratedFind()) {
          var chromeWindow = getChromeWindow(domWindow);
          var findBar = getFindBar(domWindow);
          var findEventManager = new FindEventManager(findBar,
                                                      domWindow,
                                                      chromeWindow);
          findEventManager.bind();
        }
        listener.onStopRequest(aRequest, context, statusCode);

        if (domWindow.frameElement) {
          var isObjectEmbed = domWindow.frameElement.tagName !== 'IFRAME' ||
            domWindow.frameElement.className === 'previewPluginContentFrame';
          PdfJsTelemetry.onEmbed(isObjectEmbed);
        }
      }
    };

    
    channel.originalURI = aRequest.URI;
    channel.loadGroup = aRequest.loadGroup;

    
    
    var securityManager = Cc['@mozilla.org/scriptsecuritymanager;1']
                          .getService(Ci.nsIScriptSecurityManager);
    var uri = ioService.newURI(PDF_VIEWER_WEB_PAGE, null, null);
    
    
    var resourcePrincipal = 'getNoAppCodebasePrincipal' in securityManager ?
                            securityManager.getNoAppCodebasePrincipal(uri) :
                            securityManager.getCodebasePrincipal(uri);
    aRequest.owner = resourcePrincipal;
    channel.asyncOpen(proxy, aContext);
  },

  
  onStopRequest: function(aRequest, aContext, aStatusCode) {
    if (!this.dataListener) {
      
      return;
    }

    if (Components.isSuccessCode(aStatusCode))
      this.dataListener.finish();
    else
      this.dataListener.error(aStatusCode);
    delete this.dataListener;
    delete this.binaryStream;
  }
};

