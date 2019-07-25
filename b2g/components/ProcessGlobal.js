





'use strict';










const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const kWebApiShimFile = 'chrome://browser/content/webapi.js';

Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

function debug(msg) {
  log(msg);
}
function log(msg) {
  
  
}

function ProcessGlobal() {}
ProcessGlobal.prototype = {
  classID: Components.ID('{1a94c87a-5ece-4d11-91e1-d29c29f21b28}'),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function pg_observe(subject, topic, data) {
    switch (topic) {
    case 'app-startup': {
      Services.obs.addObserver(this, 'console-api-log-event', false);
      Services.obs.addObserver(this, 'in-process-browser-frame-shown', false);
      Services.obs.addObserver(this, 'remote-browser-frame-shown', false);
      break;
    }
    case 'console-api-log-event': {
      
      
      let message = subject.wrappedJSObject;
      let prefix = ('Content JS ' + message.level.toUpperCase() +
                    ' at ' + message.filename + ':' + message.lineNumber +
                    ' in ' + (message.functionName || 'anonymous') + ': ');
      Services.console.logStringMessage(prefix + Array.join(message.arguments,
                                                            ' '));
      break;
    }
    case 'remote-browser-frame-shown':
    case 'in-process-browser-frame-shown': {
      let frameLoader = subject.QueryInterface(Ci.nsIFrameLoader);
      let mm = frameLoader.messageManager;
      try {
        mm.loadFrameScript(kWebApiShimFile, true);
      } catch (e) {
        log('Error loading ' + kWebApiShimFile + ' as frame script: ' + e + '\n');
      }
      break;
    }
    }
  },
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([ProcessGlobal]);
