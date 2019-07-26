



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SystemMessagePermissionsChecker.jsm");

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
                  "SystemMessageManager:HasPendingMessages",
                  "SystemMessageManager:Register",
                  "SystemMessageManager:Unregister",
                  "SystemMessageManager:Message:Return:OK",
                  "SystemMessageManager:AskReadyToRegister",
                  "child-process-shutdown"]

function debug(aMsg) {
  dump("-- SystemMessageInternal " + Date.now() + " : " + aMsg + "\n");
}



function SystemMessageInternal() {
  
  
  this._pages = [];

  
  
  
  
  
  this._listeners = {};

  this._webappsRegistryReady = false;
  this._bufferedSysMsgs = [];

  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.obs.addObserver(this, "webapps-registry-start", false);
  Services.obs.addObserver(this, "webapps-registry-ready", false);
  kMessages.forEach(function(aMsg) {
    ppmm.addMessageListener(aMsg, this);
  }, this);

  Services.obs.notifyObservers(this, "system-message-internal-ready", null);
}

function findTarget(aListeners, aTarget) {
  for (let i = 0; i < aListeners.length; ++i) {
    let listener = aListeners[i];
    if (listener.target === aTarget)
      return listener;
  }

  return null;
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

    
    
    if (!this._sendMessageCommon(aType,
                                 aMessage,
                                 messageID,
                                 aPageURI.spec,
                                 aManifestURI.spec)) {
      return;
    }

    let pagesToOpen = {};
    this._pages.forEach(function(aPage) {
      if (!this._isPageMatched(aPage, aType, aPageURI.spec, aManifestURI.spec)) {
        return;
      }

      
      this._queueMessage(aPage, aMessage, messageID);

      
      
      let key = this._createKeyForPage(aPage);
      if (!pagesToOpen.hasOwnProperty(key)) {
        this._openAppPage(aPage, aMessage);
        pagesToOpen[key] = true;
      }
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
    
    let pagesToOpen = {};
    this._pages.forEach(function(aPage) {
      if (aPage.type == aType) {
        
        
        if (!this._sendMessageCommon(aType,
                                     aMessage,
                                     messageID,
                                     aPage.uri,
                                     aPage.manifest)) {
          return;
        }

        
        this._queueMessage(aPage, aMessage, messageID);

        
        
        let key = this._createKeyForPage(aPage);
        if (!pagesToOpen.hasOwnProperty(key)) {
          this._openAppPage(aPage, aMessage);
          pagesToOpen[key] = true;
        }
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

    
    
    if (["SystemMessageManager:Register",
         "SystemMessageManager:Unregister",
         "SystemMessageManager:GetPendingMessages",
         "SystemMessageManager:HasPendingMessages",
         "SystemMessageManager:Message:Return:OK"].indexOf(aMessage.name) != -1) {
      if (!aMessage.target.assertContainApp(msg.manifest)) {
        debug("Got message from a child process containing illegal manifest URL.");
        return null;
      }
    }

    switch(aMessage.name) {
      case "SystemMessageManager:AskReadyToRegister":
        return true;
        break;
      case "SystemMessageManager:Register":
      {
        debug("Got Register from " + msg.manifest);
        let targets, target;
        if (!(targets = this._listeners[msg.manifest])) {
          this._listeners[msg.manifest] =
            [ { target: aMessage.target, winCount: 1 } ];
        } else if (!(target = findTarget(targets, aMessage.target))) {
          targets[msg.manifest].push({
            target: aMessage.target,
            winCount: 1
          });
        } else {
          target.winCount++;
        }

        debug("listeners for " + msg.manifest + " innerWinID " + msg.innerWindowID);
        break;
      }
      case "child-process-shutdown":
      {
        debug("Got child-process-shutdown from " + aMessage.target);
        for (let manifest in this._listeners) {
          
          let targets = this._listeners[manifest];
          for (let target = 0; target < targets.length; ++target) {
            if (targets[target].target === aMessage.target) {
              
              
              if (targets.length === 1) {
                debug("remove " + manifest );
                delete this._listeners[manifest];
              } else {
                
                
                targets.splice(target, 1);
              }
            }
          }
        }
        break;
      }
      case "SystemMessageManager:Unregister":
      {
        debug("Got Unregister from " + aMessage.target + "innerWinID " + msg.innerWindowID);
        let targets = this._listeners[msg.manifest];
        for (let i = 0; i < targets.length; ++i) {
          if (targets[i].target === aMessage.target) {
            if (--targets[i].winCount === 0) {
              if (targets.length === 1) {
                
                delete this._listeners[msg.manifest];
              } else {
                
                targets.splice(i, 1);
              }

            }
          }
        }

        debug("Removing " + aMessage.target + "innerWinID " + msg.innerWindowID );

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
          return;
        }

        
        let pendingMessages = [];
        page.pendingMessages.forEach(function(aMessage) {
          pendingMessages.push(aMessage.msg);
        });

        
        
        page.pendingMessages.length = 0;

        
        aMessage.target.sendAsyncMessage("SystemMessageManager:GetPendingMessages:Return",
                                         { type: msg.type,
                                           manifest: msg.manifest,
                                           uri: msg.uri,
                                           msgQueue: pendingMessages });
        break;
      }
      case "SystemMessageManager:HasPendingMessages":
      {
        debug("received SystemMessageManager:HasPendingMessages " + msg.type +
          " for " + msg.uri + " @ " + msg.manifest);

        
        
        let page = null;
        this._pages.some(function(aPage) {
          if (this._isPageMatched(aPage, msg.type, msg.uri, msg.manifest)) {
            page = aPage;
          }
          return page !== null;
        }, this);
        if (!page) {
          return false;
        }

        return page.pendingMessages.length != 0;
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
        Services.obs.removeObserver(this, "webapps-registry-start");
        Services.obs.removeObserver(this, "webapps-registry-ready");
        ppmm = null;
        this._pages = null;
        this._bufferedSysMsgs = null;
        break;
      case "webapps-registry-start":
        this._webappsRegistryReady = false;
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
        this._bufferedSysMsgs.length = 0;
        break;
    }
  },

  _queueMessage: function _queueMessage(aPage, aMessage, aMessageID) {
    
    
    aPage.pendingMessages.push({ msg: aMessage, msgID: aMessageID });
    if (aPage.pendingMessages.length > kMaxPendingMessages) {
      aPage.pendingMessages.splice(0, 1);
    }
  },

  _openAppPage: function _openAppPage(aPage, aMessage) {
    
    let page = { uri: aPage.uri,
                 manifest: aPage.manifest,
                 type: aPage.type,
                 target: aMessage.target };
    debug("Asking to open " + JSON.stringify(page));
    Services.obs.notifyObservers(this, "system-messages-open-app", JSON.stringify(page));
  },

  _isPageMatched: function _isPageMatched(aPage, aType, aPageURI, aManifestURI) {
    return (aPage.type === aType &&
            aPage.manifest === aManifestURI &&
            aPage.uri === aPageURI)
  },

  _createKeyForPage: function _createKeyForPage(aPage) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);

    
    ["type", "manifest", "uri"].forEach(function(aProp) {
      let data = converter.convertToByteArray(aPage[aProp], {});
      hasher.update(data, data.length);
    });

    return hasher.finish(true);
  },

  _sendMessageCommon:
    function _sendMessageCommon(aType, aMessage, aMessageID, aPageURI, aManifestURI) {
    
    if (!SystemMessagePermissionsChecker
          .isSystemMessagePermittedToSend(aType,
                                          aPageURI,
                                          aManifestURI)) {
      return false;
    }

    let winTargets = this._listeners[aManifestURI];
    if (winTargets) {
      for (let target = 0; target < winTargets.length; ++target) {
          let manager = winTargets[target].target;
          manager.sendAsyncMessage("SystemMessageManager:Message",
                                   { type: aType,
                                     msg: aMessage,
                                     manifest: aManifestURI,
                                     uri: aPageURI,
                                     msgID: aMessageID });
      }
    }

    return true;
  },

  classID: Components.ID("{70589ca5-91ac-4b9e-b839-d6a88167d714}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesInternal, Ci.nsIObserver])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SystemMessageInternal]);
