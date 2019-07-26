



let EXPORTED_SYMBOLS = ["SessionStorage"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
  "resource:///modules/sessionstore/SessionStore.jsm");

let SessionStorage = {
  






  serialize: function ssto_serialize(aDocShell, aFullData) {
    return DomStorage.read(aDocShell, aFullData);
  },

  






  deserialize: function ssto_deserialize(aDocShell, aStorageData) {
    DomStorage.write(aDocShell, aStorageData);
  }
};

Object.freeze(SessionStorage);

let DomStorage = {
  






  read: function DomStorage_read(aDocShell, aFullData) {
    let data = {};
    let isPinned = aDocShell.isAppTab;
    let shistory = aDocShell.sessionHistory;

    for (let i = 0; i < shistory.count; i++) {
      let principal = History.getPrincipalForEntry(shistory, i, aDocShell);
      if (!principal)
        continue;

      
      let isHTTPS = principal.URI && principal.URI.schemeIs("https");
      if (aFullData || SessionStore.checkPrivacyLevel(isHTTPS, isPinned)) {
        let origin = principal.extendedOrigin;

        
        if (!(origin in data)) {
          let originData = this._readEntry(principal, aDocShell);
          if (Object.keys(originData).length) {
            data[origin] = originData;
          }
        }
      }
    }

    return data;
  },

  






  write: function DomStorage_write(aDocShell, aStorageData) {
    for (let [host, data] in Iterator(aStorageData)) {
      let uri = Services.io.newURI(host, null, null);
      let principal = Services.scriptSecurityManager.getDocShellCodebasePrincipal(uri, aDocShell);
      let storage = aDocShell.getSessionStorageForPrincipal(principal, "", true);

      for (let [key, value] in Iterator(data)) {
        try {
          storage.setItem(key, value);
        } catch (e) {
          
          Cu.reportError(e);
        }
      }
    }
  },

  






  _readEntry: function DomStorage_readEntry(aPrincipal, aDocShell) {
    let hostData = {};
    let storage;

    try {
      
      
      
      
      
      storage = aDocShell.getSessionStorageForPrincipal(aPrincipal, "", false);
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
