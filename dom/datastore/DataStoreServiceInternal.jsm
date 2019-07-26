



"use strict"

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["DataStoreServiceInternal"];

function debug(s) {
  
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "dataStoreService",
                                   "@mozilla.org/datastore-service;1",
                                   "nsIDataStoreService");

this.DataStoreServiceInternal = {
  init: function() {
    debug("init");

    let inParent = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
                      .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
    if (inParent) {
      ppmm.addMessageListener("DataStore:Get", this);
    }
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage");

    if (aMessage.name != 'DataStore:Get') {
      return;
    }

    let msg = aMessage.data;

    
    msg.stores = dataStoreService.getDataStoresInfo(msg.name, msg.appId);
    aMessage.target.sendAsyncMessage("DataStore:Get:Return", msg);
  }
}

DataStoreServiceInternal.init();
