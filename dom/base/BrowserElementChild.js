



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function debug(msg) {
  
}

function sendAsyncMsg(msg, data) {
  sendAsyncMessage('browser-element-api:' + msg, data);
}

function sendSyncMsg(msg, data) {
  return sendSyncMessage('browser-element-api:' + msg, data);
}












function BrowserElementChild() {
  this._init();
};

BrowserElementChild.prototype = {
  _init: function() {
    debug("Starting up.");
    sendAsyncMsg("hello");

    docShell.QueryInterface(Ci.nsIWebProgress)
            .addProgressListener(this._progressListener,
                                 Ci.nsIWebProgress.NOTIFY_LOCATION |
                                 Ci.nsIWebProgress.NOTIFY_STATE_WINDOW);

    
    
    
    
    
    
    
    
    
    
    content.QueryInterface(Ci.nsIInterfaceRequestor)
           .getInterface(Components.interfaces.nsIDOMWindowUtils)
           .setIsApp(sendSyncMsg('get-mozapp')[0]);

    addEventListener('DOMTitleChanged',
                     this._titleChangedHandler.bind(this),
                      true,
                      false);

    addEventListener('DOMLinkAdded',
                     this._iconChangedHandler.bind(this),
                      true,
                      false);
  },

  _titleChangedHandler: function(e) {
    debug("Got titlechanged: (" + e.target.title + ")");
    var win = e.target.defaultView;

    
    
    if (win == content) {
      sendAsyncMsg('titlechange', e.target.title);
    }
    else {
      debug("Not top level!");
    }
  },

  _iconChangedHandler: function(e) {
    debug("Got iconchanged: (" + e.target.href + ")");
    var hasIcon = e.target.rel.split(' ').some(function(x) {
      return x.toLowerCase() === 'icon';
    });

    if (hasIcon) {
      var win = e.target.ownerDocument.defaultView;
      
      
      if (win == content) {
        sendAsyncMsg('iconchange', e.target.href);
      }
      else {
        debug("Not top level!");
      }
    }
  },

  
  
  _progressListener: {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                           Ci.nsISupportsWeakReference,
                                           Ci.nsISupports]),
    _seenLoadStart: false,

    onLocationChange: function(webProgress, request, location, flags) {
      
      if (webProgress != docShell) {
        return;
      }

      
      
      if (!this._seenLoadStart) {
        return;
      }

      sendAsyncMsg('locationchange', location.spec);
    },

    onStateChange: function(webProgress, request, stateFlags, status) {
      if (webProgress != docShell) {
        return;
      }

      if (stateFlags & Ci.nsIWebProgressListener.STATE_START) {
        this._seenLoadStart = true;
        sendAsyncMsg('loadstart');
      }

      if (stateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
        sendAsyncMsg('loadend');
      }
    },

    onStatusChange: function(webProgress, request, status, message) {},
    onProgressChange: function(webProgress, request, curSelfProgress,
                               maxSelfProgress, curTotalProgress, maxTotalProgress) {},
    onSecurityChange: function(webProgress, request, aState) {}
  },
};

var api = new BrowserElementChild();
