



let EXPORTED_SYMBOLS = ["SessionStorage"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
  "resource:///modules/sessionstore/SessionStore.jsm");

let SessionStorage = {
  












  serialize: function ssto_serialize(aTabData, aHistory, aDocShell, aFullData,
                                     aIsPinned) {
    let storageData = {};
    let hasContent = false;

    for (let i = 0; i < aHistory.count; i++) {
      let uri;
      try {
        uri = aHistory.getEntryAtIndex(i, false).URI;
      }
      catch (ex) {
        
        
        continue;
      }
      
      let domain = uri.spec;
      try {
        if (uri.host)
          domain = uri.prePath;
      }
      catch (ex) {  }
      if (storageData[domain] ||
          !(aFullData || SessionStore.checkPrivacyLevel(uri.schemeIs("https"), aIsPinned)))
        continue;

      let storage, storageItemCount = 0;
      try {
        var principal = Services.scriptSecurityManager.getCodebasePrincipal(uri);

        
        
        
        
        
        storage = aDocShell.getSessionStorageForPrincipal(principal, "", false);
        if (storage)
          storageItemCount = storage.length;
      }
      catch (ex) {  }
      if (storageItemCount == 0)
        continue;

      let data = storageData[domain] = {};
      for (let j = 0; j < storageItemCount; j++) {
        try {
          let key = storage.key(j);
          let item = storage.getItem(key);
          data[key] = item;
        }
        catch (ex) {  }
      }
      hasContent = true;
    }

    if (hasContent)
      aTabData.storage = storageData;
  },

  






  deserialize: function ssto_deserialize(aStorageData, aDocShell) {
    for (let url in aStorageData) {
      let uri = Services.io.newURI(url, null, null);
      let storage = aDocShell.getSessionStorageForURI(uri, "");
      for (let key in aStorageData[url]) {
        try {
          storage.setItem(key, aStorageData[url][key]);
        }
        catch (ex) { Cu.reportError(ex); } 
      }
    }
  }
};

Object.freeze(SessionStorage);
