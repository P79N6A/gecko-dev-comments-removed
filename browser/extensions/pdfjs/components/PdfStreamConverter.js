


'use strict';

var EXPORTED_SYMBOLS = ['PdfStreamConverter'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const MOZ_CENTRAL = true;
const PDFJS_EVENT_ID = 'pdf.js.message';
const PDF_CONTENT_TYPE = 'application/pdf';
const PREF_PREFIX = 'pdfjs';
const MAX_DATABASE_LENGTH = 4096;
const FIREFOX_ID = '{ec8030f7-c20a-464f-9b0e-13a3a9e97384}';
const SEAMONKEY_ID = '{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}';

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/NetUtil.jsm');


let appInfo = Cc['@mozilla.org/xre/app-info;1']
                  .getService(Ci.nsIXULAppInfo);
let privateBrowsing, inPrivateBrowsing;
let Svc = {};
XPCOMUtils.defineLazyServiceGetter(Svc, 'mime',
                                   '@mozilla.org/mime;1',
                                   'nsIMIMEService');

if (appInfo.ID === FIREFOX_ID) {
  privateBrowsing = Cc['@mozilla.org/privatebrowsing;1']
                          .getService(Ci.nsIPrivateBrowsingService);
  inPrivateBrowsing = privateBrowsing.privateBrowsingEnabled;
} else if (appInfo.ID === SEAMONKEY_ID) {
  privateBrowsing = null;
  inPrivateBrowsing = false;
}

function getBoolPref(pref, def) {
  try {
    return Services.prefs.getBoolPref(pref);
  } catch (ex) {
    return def;
  }
}

function setStringPref(pref, value) {
  let str = Cc['@mozilla.org/supports-string;1']
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
  let msg = 'PdfStreamConverter.js: ' + (aMsg.join ? aMsg.join('') : aMsg);
  Services.console.logStringMessage(msg);
  dump(msg + '\n');
}

function getDOMWindow(aChannel) {
  var requestor = aChannel.notificationCallbacks;
  var win = requestor.getInterface(Components.interfaces.nsIDOMWindow);
  return win;
}

function isEnabled() {
  if (MOZ_CENTRAL) {
    var disabled = getBoolPref(PREF_PREFIX + '.disabled', false);
    if (disabled)
      return false;
    
    
    var handlerInfo = Svc.mime
                         .getFromTypeAndExtension('application/pdf', 'pdf');
    return handlerInfo.alwaysAskBeforeHandling == false &&
           handlerInfo.preferredAction == Ci.nsIHandlerInfo.handleInternally;
  }
  
  
  return true;
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


function ChromeActions(domWindow) {
  this.domWindow = domWindow;
}

ChromeActions.prototype = {
  download: function(data, sendResponse) {
    var originalUrl = data.originalUrl;
    
    
    var originalUri = NetUtil.newURI(data.originalUrl);
    var blobUri = data.blobUrl ? NetUtil.newURI(data.blobUrl) : originalUri;
    var extHelperAppSvc =
          Cc['@mozilla.org/uriloader/external-helper-app-service;1'].
             getService(Ci.nsIExternalHelperAppService);
    var frontWindow = Cc['@mozilla.org/embedcomp/window-watcher;1'].
                         getService(Ci.nsIWindowWatcher).activeWindow;

    NetUtil.asyncFetch(blobUri, function(aInputStream, aResult) {
      if (!Components.isSuccessCode(aResult)) {
        if (sendResponse)
          sendResponse(true);
        return;
      }
      
      
      let channel = Cc['@mozilla.org/network/input-stream-channel;1'].
                       createInstance(Ci.nsIInputStreamChannel);
      channel.setURI(originalUri);
      channel.contentStream = aInputStream;
      channel.QueryInterface(Ci.nsIChannel);

      var listener = {
        extListener: null,
        onStartRequest: function(aRequest, aContext) {
          this.extListener = extHelperAppSvc.doContent('application/pdf',
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
  setDatabase: function(data) {
    if (inPrivateBrowsing)
      return;
    
    if (data.length > MAX_DATABASE_LENGTH)
      return;
    setStringPref(PREF_PREFIX + '.database', data);
  },
  getDatabase: function() {
    if (inPrivateBrowsing)
      return '{}';
    return getStringPref(PREF_PREFIX + '.database', '{}');
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
  searchEnabled: function() {
    return getBoolPref(PREF_PREFIX + '.searchEnabled', false);
  },
  fallback: function(url, sendResponse) {
    var self = this;
    var domWindow = this.domWindow;
    var strings = getLocalizedStrings('chrome.properties');
    var message = getLocalizedString(strings, 'unsupported_feature');

    var win = Services.wm.getMostRecentWindow('navigator:browser');
    var browser = win.gBrowser.getBrowserForDocument(domWindow.top.document);
    var notificationBox = win.gBrowser.getNotificationBox(browser);
    
    
    
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
                                       notificationBox.PRIORITY_WARNING_LOW,
                                       buttons,
                                       function eventsCallback(eventType) {
      
      
      if (eventType !== 'removed')
        return;
      
      
      if (!sentResponse)
        sendResponse(false);
    });
  }
};


function RequestListener(actions) {
  this.actions = actions;
}

RequestListener.prototype.receive = function(event) {
  var message = event.target;
  var doc = message.ownerDocument;
  var action = message.getUserData('action');
  var data = message.getUserData('data');
  var sync = message.getUserData('sync');
  var actions = this.actions;
  if (!(action in actions)) {
    log('Unknown action: ' + action);
    return;
  }
  if (sync) {
    var response = actions[action].call(this.actions, data);
    message.setUserData('response', response, null);
  } else {
    var response;
    if (!message.getUserData('callback')) {
      doc.documentElement.removeChild(message);
      response = null;
    } else {
      response = function sendResponse(response) {
        message.setUserData('response', response, null);

        var listener = doc.createEvent('HTMLEvents');
        listener.initEvent('pdf.js.response', true, false);
        return message.dispatchEvent(listener);
      }
    }
    actions[action].call(this.actions, data, response);
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
    if (!isEnabled())
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    
    var skipConversion = false;
    try {
      var request = aCtxt;
      request.QueryInterface(Ci.nsIHttpChannel);
      skipConversion = (request.requestMethod !== 'GET');
    } catch (e) {
      
    }
    if (skipConversion)
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    
    this.listener = aListener;
  },

  
  onDataAvailable: function(aRequest, aContext, aInputStream, aOffset, aCount) {
    
    log('SANITY CHECK: onDataAvailable SHOULD NOT BE CALLED!');
  },

  
  onStartRequest: function(aRequest, aContext) {

    
    aRequest.QueryInterface(Ci.nsIChannel);
    
    aRequest.cancel(Cr.NS_BINDING_ABORTED);

    
    var ioService = Services.io;
    var channel = ioService.newChannel(
                    'resource://pdf.js/web/viewer.html', null, null);

    var listener = this.listener;
    
    
    var proxy = {
      onStartRequest: function() {
        listener.onStartRequest.apply(listener, arguments);
      },
      onDataAvailable: function() {
        listener.onDataAvailable.apply(listener, arguments);
      },
      onStopRequest: function() {
        var domWindow = getDOMWindow(channel);
        
        if (domWindow.document.documentURIObject.equals(aRequest.URI)) {
          let requestListener = new RequestListener(
                                      new ChromeActions(domWindow));
          domWindow.addEventListener(PDFJS_EVENT_ID, function(event) {
            requestListener.receive(event);
          }, false, true);
        }
        listener.onStopRequest.apply(listener, arguments);
      }
    };

    
    channel.originalURI = aRequest.URI;
    channel.asyncOpen(proxy, aContext);
  },

  
  onStopRequest: function(aRequest, aContext, aStatusCode) {
    
  }
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([PdfStreamConverter]);
