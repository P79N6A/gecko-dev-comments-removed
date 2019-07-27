


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;


try {
  Cu.importGlobalProperties(["indexedDB"]);
} catch (ex) {
  
  if (typeof window !== 'undefined' && "console" in window) {
    console.log("Failed to import indexedDB; if this isn't a unit test," +
                " something is wrong", ex);
  }
}

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyGetter(this, "eventEmitter", function() {
  const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js", {});
  return new EventEmitter();
});

this.EXPORTED_SYMBOLS = ["LoopStorage"];

const kDatabasePrefix = "loop-";
const kDefaultDatabaseName = "default";
let gDatabaseName = kDatabasePrefix + kDefaultDatabaseName;
const kDatabaseVersion = 1;

let gWaitForOpenCallbacks = new Set();
let gDatabase = null;
let gClosed = false;




const closeDatabase = function() {
  Services.obs.removeObserver(closeDatabase, "quit-application");
  if (!gDatabase) {
    return;
  }
  gDatabase.close();
  gDatabase = null;
  gClosed = true;
};













const ensureDatabaseOpen = function(onOpen) {
  if (gClosed) {
    onOpen(new Error("Database already closed"));
    return;
  }

  if (gDatabase) {
    onOpen(null, gDatabase);
    return;
  }

  if (!gWaitForOpenCallbacks.has(onOpen)) {
    gWaitForOpenCallbacks.add(onOpen);

    if (gWaitForOpenCallbacks.size !== 1) {
      return;
    }
  }

  let invokeCallbacks = err => {
    for (let callback of gWaitForOpenCallbacks) {
      callback(err, gDatabase);
    }
    gWaitForOpenCallbacks.clear();
  };

  let openRequest = indexedDB.open(gDatabaseName, kDatabaseVersion);

  openRequest.onblocked = function(event) {
    invokeCallbacks(new Error("Database cannot be upgraded cause in use: " + event.target.error));
  };

  openRequest.onerror = function(event) {
    
    
    indexedDB.deleteDatabase(gDatabaseName);
    invokeCallbacks(new Error("Error while opening database: " + event.target.errorCode));
  };

  openRequest.onupgradeneeded = function(event) {
    let db = event.target.result;
    eventEmitter.emit("upgrade", db, event.oldVersion, kDatabaseVersion);
  };

  openRequest.onsuccess = function(event) {
    gDatabase = event.target.result;
    invokeCallbacks();
    
    Services.obs.addObserver(closeDatabase, "quit-application", false);
  };
};








const switchDatabase = function(name) {
  if (!name) {
    name = kDefaultDatabaseName;
  }
  name = kDatabasePrefix + name;
  if (name == gDatabaseName) {
    
    return;
  }

  gDatabaseName = name;
  if (gDatabase) {
    try {
      gDatabase.close();
    } finally {
      gDatabase = null;
    }
  }
};















const getTransaction = function(store, callback, mode) {
  ensureDatabaseOpen((err, db) => {
    if (err) {
      callback(err);
      return;
    }

    let trans;
    try {
      trans = db.transaction(store, mode);
    } catch(ex) {
      callback(ex);
      return;
    }
    callback(null, trans);
  });
};















const getStore = function(store, callback, mode) {
  getTransaction(store, (err, trans) => {
    if (err) {
      callback(err);
      return;
    }

    callback(null, trans.objectStore(store));
  }, mode);
};













this.LoopStorage = Object.freeze({
  



  get databaseName() {
    return gDatabaseName.substr(kDatabasePrefix.length);
  },

  







  getSingleton: function(callback) {
    ensureDatabaseOpen(callback);
  },

  





  switchDatabase: function(name = kDefaultDatabaseName) {
    switchDatabase(name);
  },

  















  getTransaction: function(store, callback, mode = "readonly") {
    getTransaction(store, callback, mode);
  },

  















  getStore: function(store, callback, mode = "readonly") {
    getStore(store, callback, mode);
  },

  

















  asyncForEach: function(list, onItem, onDone) {
    let i = 0;
    let len = list.length;

    if (!len) {
      onDone(new Error("Argument error: empty list"));
      return;
    }

    onItem(list[i], function handler(err) {
      if (err) {
        onDone(err);
        return;
      }

      i++;
      if (i < len) {
        onItem(list[i], handler, i);
      } else {
        onDone();
      }
    }, i);
  },

  

















  asyncParallel: function(list, onItem, onDone) {
    let i = 0;
    let done = 0;
    let callbackCalled = false;
    let len = list.length;

    if (!len) {
      onDone(new Error("Argument error: empty list"));
      return;
    }

    function handleCallback(err) {
      if (callbackCalled) {
        return;
      }

      if (err) {
        onDone(err);
        callbackCalled = true;
        return;
      }

      if (++done === len) {
        onDone();
        callbackCalled = true;
      }
    }

    for (; i < len; ++i) {
      onItem(list[i], handleCallback, i);
    }
  },

  on: (...params) => eventEmitter.on(...params),

  off: (...params) => eventEmitter.off(...params)
});
