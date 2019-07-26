



"use strict"

function debug(s) {

}

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

let EXPORTED_SYMBOLS = ["SettingsChangeNotifier"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const kXpcomShutdownObserverTopic      = "xpcom-shutdown";
const kMozSettingsChangedObserverTopic = "mozsettings-changed";
const kFromSettingsChangeNotifier      = "fromSettingsChangeNotifier";

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

let SettingsChangeNotifier = {
  init: function() {
    debug("init");
    ppmm.addMessageListener("Settings:Changed", this);
    Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
    Services.obs.addObserver(this, kMozSettingsChangedObserverTopic, false);
  },

  observe: function(aSubject, aTopic, aData) {
    debug("observe");
    switch (aTopic) {
      case kXpcomShutdownObserverTopic:
        ppmm.removeMessageListener("Settings:Changed", this);
        Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
        Services.obs.removeObserver(this, kMozSettingsChangedObserverTopic);
        ppmm = null;
        break;
      case kMozSettingsChangedObserverTopic:
      {
        let setting = JSON.parse(aData);
        
        
        
        if (setting.message && setting.message === kFromSettingsChangeNotifier)
          return;
        ppmm.broadcastAsyncMessage("Settings:Change:Return:OK",
          { key: setting.key, value: setting.value });
        break;
      }
      default:
        debug("Wrong observer topic: " + aTopic);
        break;
    }
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage");
    let msg = aMessage.json;
    switch (aMessage.name) {
      case "Settings:Changed":
        ppmm.broadcastAsyncMessage("Settings:Change:Return:OK",
          { key: msg.key, value: msg.value });
        Services.obs.notifyObservers(this, kMozSettingsChangedObserverTopic,
          JSON.stringify({
            key: msg.key,
            value: msg.value,
            message: kFromSettingsChangeNotifier
          }));
        break;
      default:
        debug("Wrong message: " + aMessage.name);
    }
  }
}

SettingsChangeNotifier.init();
