



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

function debug(msg) {
  dump("BrowserElementChild - " + msg + "\n");
}

function sendAsyncMsg(msg, data) {
  sendAsyncMessage('browser-element-api:' + msg, data);
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

    addEventListener('DOMTitleChanged',
                     this._titleChangedHandler.bind(this),
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
