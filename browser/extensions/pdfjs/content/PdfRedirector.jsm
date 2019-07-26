


















'use strict';

var EXPORTED_SYMBOLS = ['PdfRedirector'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const PDF_CONTENT_TYPE = 'application/pdf';
const FIREFOX_ID = '{ec8030f7-c20a-464f-9b0e-13a3a9e97384}';

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/NetUtil.jsm');


function getDOMWindow(aChannel) {
  var requestor = aChannel.notificationCallbacks ?
                  aChannel.notificationCallbacks :
                  aChannel.loadGroup.notificationCallbacks;
  var win = requestor.getInterface(Components.interfaces.nsIDOMWindow);
  return win;
}

function getObjectUrl(window) {
  
  
  var element = window.frameElement, containerElement;
  if (!element) {
    return null; 
  }
  var tagName = element.nodeName;
  while (tagName !== 'EMBED' && tagName !== 'OBJECT') {
    containerElement = element;
    element = element.parentNode;
    if (!element) {
      return null; 
    }
    tagName = element.nodeName;
  }

  
  if (element.displayedType !== element.TYPE_NULL ||
      element.pluginFallbackType !== element.PLUGIN_PLAY_PREVIEW) {
    return null; 
  }
  for (var i = 0; i < element.children.length; i++) {
    if (element.children[i] === containerElement) {
      return null; 
    }
  }

  return element.srcURI.spec;
}

function PdfRedirector() {
}

PdfRedirector.prototype = {

  
  classID: Components.ID('{8cbfd8d0-2042-4976-b3ef-d9dee1efb975}'),
  classDescription: 'pdf.js Redirector',
  contractID:
    '@mozilla.org/streamconv;1?from=application/x-moz-playpreview-pdfjs&to=*/*',

  QueryInterface: XPCOMUtils.generateQI([
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
    
  },

  
  onStartRequest: function(aRequest, aContext) {
    
    aRequest.QueryInterface(Ci.nsIChannel);
    
    aRequest.cancel(Cr.NS_BINDING_ABORTED);

    var domWindow = getDOMWindow(aRequest);
    var pdfUrl = getObjectUrl(domWindow);
    if (!pdfUrl) {
      Services.console.logStringMessage(
        'PdfRedirector.js: PDF location is not specified for OBJECT/EMBED tag');
      return;
    }

    
    var ioService = Services.io;
    var channel = ioService.newChannel(pdfUrl, null, null);

    channel.loadGroup = aRequest.loadGroup;

    channel.asyncOpen(this.listener, aContext);
  },

  
  onStopRequest: function(aRequest, aContext, aStatusCode) {
    
  }
};

