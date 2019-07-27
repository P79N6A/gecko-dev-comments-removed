



"use strict";

this.EXPORTED_SYMBOLS = ["SessionStorage"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");


function getPrincipalForFrame(docShell, frame) {
  let ssm = Services.scriptSecurityManager;
  let uri = frame.document.documentURIObject;
  return ssm.getDocShellCodebasePrincipal(uri, docShell);
}

this.SessionStorage = Object.freeze({
  









  collect: function (docShell, frameTree) {
    return SessionStorageInternal.collect(docShell, frameTree);
  },

  








  restore: function (aDocShell, aStorageData) {
    SessionStorageInternal.restore(aDocShell, aStorageData);
  }
});

let SessionStorageInternal = {
  









  collect: function (docShell, frameTree) {
    let data = {};
    let visitedOrigins = new Set();

    frameTree.forEach(frame => {
      let principal = getPrincipalForFrame(docShell, frame);
      if (!principal) {
        return;
      }

      
      
      let origin = principal.jarPrefix + principal.origin;
      if (visitedOrigins.has(origin)) {
        
        return;
      }

      
      visitedOrigins.add(origin);

      let originData = this._readEntry(principal, docShell);
      if (Object.keys(originData).length) {
        data[origin] = originData;
      }
    });

    return Object.keys(data).length ? data : null;
  },

  








  restore: function (aDocShell, aStorageData) {
    for (let host of Object.keys(aStorageData)) {
      let data = aStorageData[host];
      let uri = Services.io.newURI(host, null, null);
      let principal = Services.scriptSecurityManager.getDocShellCodebasePrincipal(uri, aDocShell);
      let storageManager = aDocShell.QueryInterface(Ci.nsIDOMStorageManager);
      let window = aDocShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);

      
      
      
      let storage = storageManager.createStorage(window, principal, "", aDocShell.usePrivateBrowsing);

      for (let key of Object.keys(data)) {
        try {
          storage.setItem(key, data[key]);
        } catch (e) {
          
          console.error(e);
        }
      }
    }
  },

  






  _readEntry: function (aPrincipal, aDocShell) {
    let hostData = {};
    let storage;

    let window = aDocShell.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);

    try {
      let storageManager = aDocShell.QueryInterface(Ci.nsIDOMStorageManager);
      storage = storageManager.getStorage(window, aPrincipal);
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
