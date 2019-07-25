


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
function getWindow(top, id) {
  return top.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIDOMWindowUtils)
            .getOuterWindowWithId(id);
}
function windowID(win) {
  return win.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIDOMWindowUtils)
            .outerWindowID;
}
function topWindow(win) {
  return win.QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIWebNavigation)
            .QueryInterface(Ci.nsIDocShellTreeItem)
            .rootTreeItem
            .QueryInterface(Ci.nsIInterfaceRequestor)
            .getInterface(Ci.nsIDOMWindow);
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

    
    channel.originalURI = aRequest.URI;
    channel.asyncOpen(this.listener, aContext);

    
    
    
    
    let window = aRequest.loadGroup.groupObserver
                         .QueryInterface(Ci.nsIWebProgress)
                         .DOMWindow;
    let top = topWindow(window);
    let id = windowID(window);
    window = null;

    top.addEventListener('DOMWindowCreated', function onDOMWinCreated(event) {
      let doc = event.originalTarget;
      let win = doc.defaultView;

      if (id == windowID(win)) {
        top.removeEventListener('DOMWindowCreated', onDOMWinCreated, true);
        if (!doc.documentURIObject.equals(aRequest.URI))
          return;

        let requestListener = new RequestListener(new ChromeActions);
        win.addEventListener(PDFJS_EVENT_ID, function(event) {
          requestListener.receive(event);
        }, false, true);
      } else if (!getWindow(top, id)) {
        top.removeEventListener('DOMWindowCreated', onDOMWinCreated, true);
      }
    }, true);
  },

  
  onStopRequest: function(aRequest, aContext, aStatusCode) {
    
  }
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([PdfStreamConverter]);
