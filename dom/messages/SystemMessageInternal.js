



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

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");


let kMaxPendingMessages;
try {
  kMaxPendingMessages = Services.prefs.getIntPref("dom.messages.maxPendingMessages");
} catch(e) {
  
  kMaxPendingMessages = 5;
}

const kMessages =["SystemMessageManager:GetPendingMessages",
                  "SystemMessageManager:Register",
                  "SystemMessageManager:Message:Return:OK",
                  "child-process-shutdown"]

function debug(aMsg) {
  
}



function SystemMessageInternal() {
  
  
  this._pages = [];
  this._listeners = {};

  this._webappsRegistryReady = false;
  this._bufferedSysMsgs = [];

  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.obs.addObserver(this, "webapps-registry-ready", false);
  kMessages.forEach(function(aMsg) {
    ppmm.addMessageListener(aMsg, this);
  }, this);
}

SystemMessageInternal.prototype = {
  sendMessage: function sendMessage(aType, aMessage, aPageURI, aManifestURI) {
    
    
    if (!this._webappsRegistryReady) {
      this._bufferedSysMsgs.push({ how: "send",
                                   type: aType,
                                   msg: aMessage,
                                   pageURI: aPageURI,
                                   manifestURI: aManifestURI });
      return;
    }

    
    
    let messageID = gUUIDGenerator.generateUUID().toString();

    debug("Sending " + aType + " " + JSON.stringify(aMessage) +
      " for " + aPageURI.spec + " @ " + aManifestURI.spec);
    if (this._listeners[aManifestURI.spec]) {
      this._listeners[aManifestURI.spec].forEach(function sendMsg(aListener) {
        aListener.sendAsyncMessage("SystemMessageManager:Message",
                                   { type: aType,
                                     msg: aMessage,
                                     manifest: aManifestURI.spec,
                                     uri: aPageURI.spec,
                                     msgID: messageID })
      });
    }

    this._pages.forEach(function(aPage) {
      if (!this._isPageMatched(aPage, aType, aPageURI.spec, aManifestURI.spec)) {
        return;
      }

      this._openAppPage(aPage, aMessage, messageID);
    }, this);
  },

  broadcastMessage: function broadcastMessage(aType, aMessage) {
    
    
    if (!this._webappsRegistryReady) {
      this._bufferedSysMsgs.push({ how: "broadcast",
                                   type: aType,
                                   msg: aMessage });
      return;
    }

    
    
    let messageID = gUUIDGenerator.generateUUID().toString();

    debug("Broadcasting " + aType + " " + JSON.stringify(aMessage));
    
    this._pages.forEach(function(aPage) {
      if (aPage.type == aType) {
        if (this._listeners[aPage.manifest]) {
          this._listeners[aPage.manifest].forEach(function sendMsg(aListener) {
            aListener.sendAsyncMessage("SystemMessageManager:Message",
                                       { type: aType,
                                         msg: aMessage,
                                         manifest: aPage.manifest,
                                         uri: aPage.uri,
                                         msgID: messageID })
          });
        }
        this._openAppPage(aPage, aMessage, messageID);
      }
    }, this);
  },

  registerPage: function registerPage(aType, aPageURI, aManifestURI) {
    if (!aPageURI || !aManifestURI) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    this._pages.push({ type: aType,
                       uri: aPageURI.spec,
                       manifest: aManifestURI.spec,
                       pendingMessages: [] });
  },

  receiveMessage: function receiveMessage(aMessage) {
    let msg = aMessage.json;
    switch(aMessage.name) {
      case "SystemMessageManager:Register":
      {
        let manifest = msg.manifest;
        debug("Got Register from " + manifest);
        if (!this._listeners[manifest]) {
          this._listeners[manifest] = [];
        }
        this._listeners[manifest].push(aMessage.target);
        debug("listeners for " + manifest + " : " + this._listeners[manifest].length);
        break;
      }
      case "child-process-shutdown":
      {
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
      }
      case "SystemMessageManager:GetPendingMessages":
      {
        debug("received SystemMessageManager:GetPendingMessages " + msg.type +
          " for " + msg.uri + " @ " + msg.manifest);

        
        
        let page = null;
        this._pages.some(function(aPage) {
          if (this._isPageMatched(aPage, msg.type, msg.uri, msg.manifest)) {
            page = aPage;
          }
          return page !== null;
        }, this);
        if (!page) {
          return null;
        }

        
        let pendingMessages = [];
        page.pendingMessages.forEach(function(aMessage) {
          pendingMessages.push(aMessage.msg);
        });

        
        
        page.pendingMessages.length = 0;

        return pendingMessages;
        break;
      }
      case "SystemMessageManager:Message:Return:OK":
      {
        debug("received SystemMessageManager:Message:Return:OK " + msg.type +
          " for " + msg.uri + " @ " + msg.manifest);

        
        
        this._pages.forEach(function(aPage) {
          if (!this._isPageMatched(aPage, msg.type, msg.uri, msg.manifest)) {
            return;
          }

          let pendingMessages = aPage.pendingMessages;
          for (let i = 0; i < pendingMessages.length; i++) {
            if (pendingMessages[i].msgID === msg.msgID) {
              pendingMessages.splice(i, 1);
              break;
            }
          }
        }, this);
        break;
      }
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        kMessages.forEach(function(aMsg) {
          ppmm.removeMessageListener(aMsg, this);
        }, this);
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, "webapps-registry-ready");
        ppmm = null;
        this._pages = null;
        this._bufferedSysMsgs = null;
        break;
      case "webapps-registry-ready":
        
        
        this._webappsRegistryReady = true;
        this._bufferedSysMsgs.forEach(function(aSysMsg) {
          switch (aSysMsg.how) {
            case "send":
              this.sendMessage(
                aSysMsg.type, aSysMsg.msg, aSysMsg.pageURI, aSysMsg.manifestURI);
              break;
            case "broadcast":
              this.broadcastMessage(aSysMsg.type, aSysMsg.msg);
              break;
          }
        }, this);
        this._bufferedSysMsgs = null;
        break;
    }
  },

  _openAppPage: function _openAppPage(aPage, aMessage, aMessageID) {
    
    
    aPage.pendingMessages.push({ msg: aMessage, msgID: aMessageID });
    if (aPage.pendingMessages.length > kMaxPendingMessages) {
      aPage.pendingMessages.splice(0, 1);
    }

    
    let page = { uri: aPage.uri,
                 manifest: aPage.manifest,
                 type: aPage.type,
                 target: aMessage.target };
    debug("Asking to open " + JSON.stringify(page));
    Services.obs.notifyObservers(this, "system-messages-open-app", JSON.stringify(page));
  },

  _isPageMatched: function _isPageMatched(aPage, aType, aUri, aManifest) {
    return (aPage.type === aType &&
            aPage.manifest === aManifest &&
            aPage.uri === aUri)
  },

  classID: Components.ID("{70589ca5-91ac-4b9e-b839-d6a88167d714}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesInternal, Ci.nsIObserver])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SystemMessageInternal]);
