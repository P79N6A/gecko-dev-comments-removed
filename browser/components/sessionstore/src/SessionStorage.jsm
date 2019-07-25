



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
      let uri = History.getUriForEntry(shistory, i);

      if (uri) {
        
        let isHTTPS = uri.schemeIs("https");
        if (aFullData || SessionStore.checkPrivacyLevel(isHTTPS, isPinned)) {
          let host = History.getHostForURI(uri);

          
          if (!(host in data)) {
            let hostData = this._readEntry(uri, aDocShell);
            if (Object.keys(hostData).length) {
              data[host] = hostData;
            }
          }
        }
      }
    }

    return data;
  },

  






  write: function DomStorage_write(aDocShell, aStorageData) {
    for (let [host, data] in Iterator(aStorageData)) {
      let uri = Services.io.newURI(host, null, null);
      let storage = aDocShell.getSessionStorageForURI(uri, "");

      for (let [key, value] in Iterator(data)) {
        try {
          storage.setItem(key, value);
        } catch (e) {
          
          Cu.reportError(e);
        }
      }
    }
  },

  






  _readEntry: function DomStorage_readEntry(aURI, aDocShell) {
    let hostData = {};
    let storage;

    try {
      let principal = Services.scriptSecurityManager.getCodebasePrincipal(aURI);

      
      
      
      
      
      storage = aDocShell.getSessionStorageForPrincipal(principal, "", false);
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
  






  getUriForEntry: function History_getUriForEntry(aHistory, aIndex) {
    try {
      return aHistory.getEntryAtIndex(aIndex, false).URI;
    } catch (e) {
      
    }
  },

  




  getHostForURI: function History_getHostForURI(aURI) {
    let host = aURI.spec;

    try {
      if (aURI.host)
        host = aURI.prePath;
    } catch (e) {
      
    }

    return host;
  }
};
