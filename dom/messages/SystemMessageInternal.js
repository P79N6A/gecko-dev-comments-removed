



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

const kMessages =["SystemMessageManager:GetPending",
                  "SystemMessageManager:Register",
                  "child-process-shutdown"]

function debug(aMsg) {
  
}



function SystemMessageInternal() {
  
  
  this._pages = [];
  this._listeners = {};
  Services.obs.addObserver(this, "xpcom-shutdown", false);
  kMessages.forEach((function(aMsg) {
    ppmm.addMessageListener(aMsg, this);
  }).bind(this));
}

SystemMessageInternal.prototype = {
  sendMessage: function sendMessage(aType, aMessage, aPageURI, aManifestURI) {
    debug("Broadcasting " + aType + " " + JSON.stringify(aMessage));
    if (this._listeners[aManifestURI.spec]) {
      this._listeners[aManifestURI.spec].forEach(function sendMsg(aListener) {
        aListener.sendAsyncMessage("SystemMessageManager:Message",
                                   { type: aType,
                                     msg: aMessage,
                                     manifest: aManifestURI.spec })
      });
    }

    this._pages.forEach(function sendMess_openPage(aPage) {
      if (aPage.type != aType ||
          aPage.manifest != aManifestURI.spec ||
          aPage.uri != aPageURI.spec) {
        return;
      }

      this._processPage(aPage, aMessage);
    }.bind(this))
  },

  broadcastMessage: function broadcastMessage(aType, aMessage) {
    debug("Broadcasting " + aType + " " + JSON.stringify(aMessage));
    
    this._pages.forEach(function(aPage) {
      if (aPage.type == aType) {
        if (this._listeners[aPage.manifest]) {
          this._listeners[aPage.manifest].forEach(function sendMsg(aListener) {
            aListener.sendAsyncMessage("SystemMessageManager:Message",
                                       { type: aType,
                                         msg: aMessage,
                                         manifest: aPage.manifest})
          });
        }
        this._processPage(aPage, aMessage);
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
    let msg = aMessage.json;
    switch(aMessage.name) {
      case "SystemMessageManager:Register":
        let manifest = msg.manifest;
        debug("Got Register from " + manifest);
        if (!this._listeners[manifest]) {
          this._listeners[manifest] = [];
        }
        this._listeners[manifest].push(aMessage.target);
        debug("listeners for " + manifest + " : " + this._listeners[manifest].length);
        break;
      case "child-process-shutdown":
        debug("Got Unregister from " + aMessage.target);
        let mm = aMessage.target;
        for (let manifest in this._listeners) {
          let index = this._listeners[manifest].indexOf(mm);
          while (index != -1) {
            debug("Removing " + mm + " at index " + index);
            this._listeners[manifest].splice(index, 1);
            index = this._listeners[manifest].indexOf(mm);
          }
        }
        break;
      case "SystemMessageManager:GetPending":
        debug("received SystemMessageManager:GetPending " + aMessage.json.type +
          " for " + aMessage.json.uri + " @ " + aMessage.json.manifest);
        
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
        break;
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      kMessages.forEach((function(aMsg) {
        ppmm.removeMessageListener(aMsg, this);
      }).bind(this));
      Services.obs.removeObserver(this, "xpcom-shutdown");
      ppmm = null;
      this._pages = null;
    }
  },

  _processPage: function _processPage(aPage, aMessage) {
    
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
  },

  classID: Components.ID("{70589ca5-91ac-4b9e-b839-d6a88167d714}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesInternal, Ci.nsIObserver])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SystemMessageInternal]);
