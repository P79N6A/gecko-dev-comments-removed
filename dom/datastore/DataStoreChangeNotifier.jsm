



"use strict"

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["DataStoreChangeNotifier"];

function debug(s) {
  
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "dataStoreService",
                                   "@mozilla.org/datastore-service;1",
                                   "nsIDataStoreService");

XPCOMUtils.defineLazyServiceGetter(this, "systemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

var kSysMsgOnChangeShortTimeoutSec =
    Services.prefs.getIntPref("dom.datastore.sysMsgOnChangeShortTimeoutSec");
var kSysMsgOnChangeLongTimeoutSec =
    Services.prefs.getIntPref("dom.datastore.sysMsgOnChangeLongTimeoutSec");

this.DataStoreChangeNotifier = {
  children: [],
  messages: [ "DataStore:Changed", "DataStore:RegisterForMessages",
              "DataStore:UnregisterForMessages",
              "child-process-shutdown" ],

  
  
  
  
  
  
  
  
  
  sysMsgOnChangeShortTimers: {},
  sysMsgOnChangeLongTimers: {},

  init: function() {
    debug("init");

    this.messages.forEach((function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }).bind(this));

    Services.obs.addObserver(this, 'xpcom-shutdown', false);
  },

  observe: function(aSubject, aTopic, aData) {
    debug("observe");

    switch (aTopic) {
      case 'xpcom-shutdown':
        this.messages.forEach((function(msgName) {
          ppmm.removeMessageListener(msgName, this);
        }).bind(this));

        Services.obs.removeObserver(this, 'xpcom-shutdown');
        ppmm = null;
        break;

      default:
        debug("Wrong observer topic: " + aTopic);
        break;
    }
  },

  broadcastMessage: function broadcastMessage(aData) {
    debug("broadcast");

    this.children.forEach(function(obj) {
      if (obj.store == aData.store && obj.owner == aData.owner) {
        obj.mm.sendAsyncMessage("DataStore:Changed:Return:OK", aData);
      }
    });
  },

  broadcastSystemMessage: function(aStore, aOwner) {
    debug("broadcastSystemMessage");

    
    var storeKey = aStore + "|" + aOwner;
    var shortTimer = this.sysMsgOnChangeShortTimers[storeKey];
    if (shortTimer) {
      shortTimer.cancel();
      delete this.sysMsgOnChangeShortTimers[storeKey];
    }
    var longTimer = this.sysMsgOnChangeLongTimers[storeKey];
    if (longTimer) {
      longTimer.cancel();
      delete this.sysMsgOnChangeLongTimers[storeKey];
    }

    
    var manifestURLs = dataStoreService.getAppManifestURLsForDataStore(aStore);
    var enumerate = manifestURLs.enumerate();
    while (enumerate.hasMoreElements()) {
      var manifestURL = enumerate.getNext().QueryInterface(Ci.nsISupportsString);
      debug("Notify app " + manifestURL + " of datastore updates");
      
      
      
      systemMessenger.sendMessage("datastore-update-" + aStore,
                                  { owner: aOwner },
                                  null,
                                  Services.io.newURI(manifestURL, null, null));
    }
  },

  
  
  
  
  
  setSystemMessageTimeout: function(aStore, aOwner) {
    debug("setSystemMessageTimeout");

    var storeKey = aStore + "|" + aOwner;

    
    var shortTimer = this.sysMsgOnChangeShortTimers[storeKey];
    if (!shortTimer) {
      shortTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.sysMsgOnChangeShortTimers[storeKey] = shortTimer;
    } else {
      shortTimer.cancel();
    }
    shortTimer.initWithCallback({ notify: this.broadcastSystemMessage.bind(this, aStore, aOwner) },
                                kSysMsgOnChangeShortTimeoutSec * 1000,
                                Ci.nsITimer.TYPE_ONE_SHOT);

    
    if (!this.sysMsgOnChangeLongTimers[storeKey]) {
      var longTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.sysMsgOnChangeLongTimers[storeKey] = longTimer;
      longTimer.initWithCallback({ notify: this.broadcastSystemMessage.bind(this, aStore, aOwner) },
                                 kSysMsgOnChangeLongTimeoutSec * 1000,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
    }
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage ");

    
    if (aMessage.name != "child-process-shutdown") {
      if (!("principal" in aMessage)) {
        return;
      }
      let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"]
                     .getService(Ci.nsIScriptSecurityManager);
      let uri = Services.io.newURI(aMessage.principal.origin, null, null);
      let principal = secMan.getAppCodebasePrincipal(uri,
        aMessage.principal.appId, aMessage.principal.isInBrowserElement);
      if (!principal || !dataStoreService.checkPermission(principal)) {
        return;
      }
    }

    switch (aMessage.name) {
      case "DataStore:Changed":
        this.broadcastMessage(aMessage.data);
        if (Services.prefs.getBoolPref("dom.sysmsg.enabled")) {
          this.setSystemMessageTimeout(aMessage.data.store, aMessage.data.owner);
        }
        break;

      case "DataStore:RegisterForMessages":
        debug("Register!");

        for (let i = 0; i < this.children.length; ++i) {
          if (this.children[i].mm == aMessage.target &&
              this.children[i].store == aMessage.data.store &&
              this.children[i].owner == aMessage.data.owner) {
            debug("Register on existing index: " + i);
            this.children[i].windows.push(aMessage.data.innerWindowID);
            return;
          }
        }

        this.children.push({ mm: aMessage.target,
                             store: aMessage.data.store,
                             owner: aMessage.data.owner,
                             windows: [ aMessage.data.innerWindowID ]});
        break;

      case "DataStore:UnregisterForMessages":
        debug("Unregister");

        for (let i = 0; i < this.children.length; ++i) {
          if (this.children[i].mm == aMessage.target) {
            debug("Unregister index: " + i);

            var pos = this.children[i].windows.indexOf(aMessage.data.innerWindowID);
            if (pos != -1) {
              this.children[i].windows.splice(pos, 1);
            }

            if (this.children[i].windows.length) {
              continue;
            }

            debug("Unregister delete index: " + i);
            this.children.splice(i, 1);
            --i;
          }
        }
        break;

      case "child-process-shutdown":
        debug("Child process shutdown");

        for (let i = 0; i < this.children.length; ++i) {
          if (this.children[i].mm == aMessage.target) {
            debug("Unregister index: " + i);
            this.children.splice(i, 1);
            --i;
          }
        }
        break;

      default:
        debug("Wrong message: " + aMessage.name);
    }
  }
}

DataStoreChangeNotifier.init();
