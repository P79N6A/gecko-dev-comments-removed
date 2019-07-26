



"use strict"

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["DataStoreChangeNotifier"];

function debug(s) {
  
}



Cu.import('resource://gre/modules/DataStoreServiceInternal.jsm');
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

this.DataStoreChangeNotifier = {
  children: [],
  messages: [ "DataStore:Changed", "DataStore:RegisterForMessages",
              "DataStore:UnregisterForMessages",
              "child-process-shutdown" ],

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
    debug("Broadast");
    this.children.forEach(function(obj) {
      if (obj.store == aData.store && obj.owner == aData.owner) {
        obj.mm.sendAsyncMessage("DataStore:Changed:Return:OK", aData);
      }
    });
  },


  receiveMessage: function(aMessage) {
    debug("receiveMessage");

    
    
    
    let prefName = 'dom.testing.datastore_enabled_for_hosted_apps';
    if (aMessage.name != 'child-process-shutdown' &&
        (Services.prefs.getPrefType(prefName) == Services.prefs.PREF_INVALID ||
         !Services.prefs.getBoolPref(prefName)) &&
        !aMessage.target.assertAppHasStatus(Ci.nsIPrincipal.APP_STATUS_CERTIFIED)) {
      return;
    }

    switch (aMessage.name) {
      case "DataStore:Changed":
        this.broadcastMessage(aMessage.data);
        break;

      case "DataStore:RegisterForMessages":
        debug("Register!");

        for (let i = 0; i < this.children.length; ++i) {
          if (this.children[i].mm == aMessage.target &&
              this.children[i].store == aMessage.data.store &&
              this.children[i].owner == aMessage.data.owner) {
            debug("Register on existing index: " + i);
            ++this.children[i].count;
            return;
          }
        }

        this.children.push({ mm: aMessage.target,
                             store: aMessage.data.store,
                             owner: aMessage.data.owner,
                             count: 1 });
        break;

      case "child-process-shutdown":
      case "DataStore:UnregisterForMessages":
        debug("Unregister");

        for (let i = 0; i < this.children.length;) {
          if (this.children[i].mm == aMessage.target) {
            debug("Unregister index: " + i);
            if (!--this.children[i].count) {
              debug("Unregister delete index: " + i);
              this.children.splice(i, 1);
            }
            break;
          } else {
            ++i;
          }
        }
        break;

      default:
        debug("Wrong message: " + aMessage.name);
    }
  }
}

DataStoreChangeNotifier.init();
