



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");


let kMaxPendingMessages;
try {
  kMaxPendingMessages = Services.prefs.getIntPref("dom.messages.maxPendingMessages");
} catch(e) {
  
  kMaxPendingMessages = 5;
}

function debug(aMsg) { 
  
}



function SystemMessageInternal() {
  
  
  this._pages = [];
  Services.obs.addObserver(this, "xpcom-shutdown", false);
  ppmm.addMessageListener("SystemMessageManager:GetPending", this);
}

SystemMessageInternal.prototype = {
  sendMessage: function sendMessage(aType, aMessage, aPageURI, aManifestURI) {
    this._sendMessage(aType, aMessage, aPageURI.spec, aManifestURI.spec);
  },

  broadcastMessage: function broadcastMessage(aType, aMessage) {
    this._pages.forEach(function(aPage) {
      if (aPage.type == aType) {
        this._sendMessage(aType, aMessage, aPage.uri, aPage.manifest);
      }
    }.bind(this))
  },

  registerPage: function registerPage(aType, aPageURI, aManifestURI) {
    if (!aPageURI || !aManifestURI) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    this._pages.push({ type: aType,
                       uri: aPageURI.spec,
                       manifest: aManifestURI.spec,
                       pending: [] });
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("received SystemMessageManager:GetPending " + aMessage.json.type + " for " + aMessage.json.uri + " @ " + aMessage.json.manifest);
    
    let msg = aMessage.json;
    debug(JSON.stringify(msg));

    
    let page = null;
    this._pages.some(function(aPage) {
      if (aPage.uri == msg.uri &&
          aPage.type == msg.type &&
          aPage.manifest == msg.manifest) {
        page = aPage;
      }
      return page !== null;
    });

    if (!page) {
      return null;
    }

    let pending = page.pending;
    
    
    page.pending = [];

    return pending;
  },

  observe: function observe(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      ppmm.removeMessageListener("SystemMessageManager:GetPending", this);
      Services.obs.removeObserver(this, "xpcom-shutdown");
      ppmm = null;
      this._pages = null;
    }
  },

  _sendMessage: function _sendMessage(aType, aMessage, aPageURI, aManifestURI) {
    debug("Broadcasting " + aType + " " + JSON.stringify(aMessage));
    ppmm.broadcastAsyncMessage("SystemMessageManager:Message" , { type: aType,
                                                                  msg: aMessage,
                                                                  manifest: aManifestURI });

    
    this._pages.forEach(function sendMess_openPage(aPage) {
      if (aPage.type != aType ||
          aPage.manifest != aManifestURI ||
          aPage.uri != aPageURI) {
        return;
      }

      aPage.pending.push(aMessage);
      if (aPage.pending.length > kMaxPendingMessages) {
        aPage.pending.splice(0, 1);
      }

      
      let page = { uri: aPage.uri,
                   manifest: aPage.manifest,
                   type: aPage.type,
                   target: aMessage.target };
      debug("Asking to open  " + JSON.stringify(page));
      Services.obs.notifyObservers(this, "system-messages-open-app", JSON.stringify(page));
    }.bind(this))
  },

  classID: Components.ID("{70589ca5-91ac-4b9e-b839-d6a88167d714}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesInternal, Ci.nsIObserver])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SystemMessageInternal]);
