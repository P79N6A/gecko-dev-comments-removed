



"use strict";

this.EXPORTED_SYMBOLS = ["SessionStorage"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");

this.SessionStorage = Object.freeze({
  







  collect: function (aDocShell) {
    return SessionStorageInternal.collect(aDocShell);
  },

  








  restore: function (aDocShell, aStorageData) {
    SessionStorageInternal.restore(aDocShell, aStorageData);
  }
});

let SessionStorageInternal = {
  







  collect: function (aDocShell) {
    let data = {};
    let webNavigation = aDocShell.QueryInterface(Ci.nsIWebNavigation);
    let shistory = webNavigation.sessionHistory;

    for (let i = 0; shistory && i < shistory.count; i++) {
      let principal = History.getPrincipalForEntry(shistory, i, aDocShell);
      if (!principal) {
        continue;
      }

      
      
      let origin = principal.jarPrefix + principal.origin;
      if (data.hasOwnProperty(origin)) {
        
        continue;
      }

      let originData = this._readEntry(principal, aDocShell);
      if (Object.keys(originData).length) {
        data[origin] = originData;
      }
    }

    return Object.keys(data).length ? data : null;
  },

  








  restore: function (aDocShell, aStorageData) {
    for (let [host, data] in Iterator(aStorageData)) {
      let uri = Services.io.newURI(host, null, null);
      let principal = Services.scriptSecurityManager.getDocShellCodebasePrincipal(uri, aDocShell);
      let storageManager = aDocShell.QueryInterface(Components.interfaces.nsIDOMStorageManager);

      
      
      
      let storage = storageManager.createStorage(principal, "", aDocShell.usePrivateBrowsing);

      for (let [key, value] in Iterator(data)) {
        try {
          storage.setItem(key, value);
        } catch (e) {
          
          Cu.reportError(e);
        }
      }
    }
  },

  






  _readEntry: function (aPrincipal, aDocShell) {
    let hostData = {};
    let storage;

    try {
      let storageManager = aDocShell.QueryInterface(Components.interfaces.nsIDOMStorageManager);
      storage = storageManager.getStorage(aPrincipal);
    } catch (e) {
      
    }

    if (storage && storage.length) {
       for (let i = 0; i < storage.length; i++) {
        try {
          let key = storage.key(i);
          hostData[key] = storage.getItem(key);
        } catch (e) {
          
        }
      }
    }

    return hostData;
  }
};

let History = {
  








  getPrincipalForEntry: function History_getPrincipalForEntry(aHistory,
                                                              aIndex,
                                                              aDocShell) {
    try {
      return Services.scriptSecurityManager.getDocShellCodebasePrincipal(
        aHistory.getEntryAtIndex(aIndex, false).URI, aDocShell);
    } catch (e) {
      
    }
  },
};
