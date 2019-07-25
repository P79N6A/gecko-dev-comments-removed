


'use strict';

var EXPORTED_SYMBOLS = ['PdfStreamConverter'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const PDFJS_EVENT_ID = 'pdf.js.message';
const PDF_CONTENT_TYPE = 'application/pdf';
const EXT_PREFIX = 'extensions.uriloader@pdf.js';
const MAX_DATABASE_LENGTH = 4096;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

let application = Cc['@mozilla.org/fuel/application;1']
                    .getService(Ci.fuelIApplication);
let privateBrowsing = Cc['@mozilla.org/privatebrowsing;1']
                        .getService(Ci.nsIPrivateBrowsingService);
let inPrivateBrowswing = privateBrowsing.privateBrowsingEnabled;

function log(aMsg) {
  if (!application.prefs.getValue(EXT_PREFIX + '.pdfBugEnabled', false))
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


function ChromeActions() {
  this.inPrivateBrowswing = privateBrowsing.privateBrowsingEnabled;
}
ChromeActions.prototype = {
  download: function(data) {
    Services.wm.getMostRecentWindow('navigator:browser').saveURL(data);
  },
  setDatabase: function(data) {
    if (this.inPrivateBrowswing)
      return;
    
    if (data.length > MAX_DATABASE_LENGTH)
      return;
    application.prefs.setValue(EXT_PREFIX + '.database', data);
  },
  getDatabase: function() {
    if (this.inPrivateBrowswing)
      return '{}';
    return application.prefs.getValue(EXT_PREFIX + '.database', '{}');
  },
  pdfBugEnabled: function() {
    return application.prefs.getValue(EXT_PREFIX + '.pdfBugEnabled', false);
  }
};



function RequestListener(actions) {
  this.actions = actions;
}

RequestListener.prototype.receive = function(event) {
  var message = event.target;
  var action = message.getUserData('action');
  var data = message.getUserData('data');
  var actions = this.actions;
  if (!(action in actions)) {
    log('Unknown action: ' + action);
    return;
  }
  var response = actions[action].call(this.actions, data);
  message.setUserData('response', response, null);
};


function PdfStreamConverter() {
}

PdfStreamConverter.prototype = {

  
  classID: Components.ID('{6457a96b-2d68-439a-bcfa-44465fbcdbb1}'),
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
          let requestListener = new RequestListener(new ChromeActions);
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
