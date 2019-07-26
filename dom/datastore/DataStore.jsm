





'use strict'

var EXPORTED_SYMBOLS = ["DataStore"];

function debug(s) {
  
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/DataStoreDB.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");
Cu.import('resource://gre/modules/Services.jsm');



function DataStore(aAppId, aName, aOwner, aReadOnly, aGlobalScope) {
  this.appId = aAppId;
  this.name = aName;
  this.owner = aOwner;
  this.readOnly = aReadOnly;

  this.db = new DataStoreDB();
  this.db.init(aOwner, aName, aGlobalScope);
}

DataStore.prototype = {
  appId: null,
  name: null,
  owner: null,
  readOnly: null,

  newDBPromise: function(aWindow, aTxnType, aFunction) {
    let db = this.db;
    return new aWindow.Promise(function(resolver) {
      debug("DBPromise started");
      db.txn(
        aTxnType,
        function(aTxn, aStore) {
          debug("DBPromise success");
          aFunction(resolver, aTxn, aStore);
        },
        function() {
          debug("DBPromise error");
          resolver.reject(new aWindow.DOMError("InvalidStateError"));
        }
      );
    });
  },

  getInternal: function(aWindow, aResolver, aStore, aId) {
    debug("GetInternal " + aId);

    let request = aStore.get(aId);
    request.onsuccess = function(aEvent) {
      debug("GetInternal success. Record: " + aEvent.target.result);
      aResolver.resolve(ObjectWrapper.wrap(aEvent.target.result, aWindow));
    };

    request.onerror = function(aEvent) {
      debug("GetInternal error");
      aResolver.reject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  updateInternal: function(aWindow, aResolver, aStore, aId, aObj) {
    debug("UpdateInternal " + aId);

    let request = aStore.put(aObj, aId);
    request.onsuccess = function(aEvent) {
      debug("UpdateInternal success");
      
      aResolver.resolve(aEvent.target.result);
    };
    request.onerror = function(aEvent) {
      debug("UpdateInternal error");
      aResolver.reject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  addInternal: function(aWindow, aResolver, aStore, aObj) {
    debug("AddInternal");

    let request = aStore.put(aObj);
    request.onsuccess = function(aEvent) {
      debug("Request successful. Id: " + aEvent.target.result);
      
      aResolver.resolve(aEvent.target.result);
    };
    request.onerror = function(aEvent) {
      debug("AddInternal error");
      aResolver.reject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  removeInternal: function(aResolver, aStore, aId) {
    debug("RemoveInternal");

    let request = aStore.delete(aId);
    request.onsuccess = function() {
      debug("RemoveInternal success");
      aResolver.resolve();
    };
    request.onerror = function(aEvent) {
      debug("RemoveInternal error");
      aResolver.reject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  clearInternal: function(aResolver, aStore) {
    debug("ClearInternal");

    let request = aStore.clear();
    request.onsuccess = function() {
      debug("ClearInternal success");
      aResolver.resolve();
    };
    request.onerror = function(aEvent) {
      debug("ClearInternal error");
      aResolver.reject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  throwInvalidArg: function(aWindow) {
    return aWindow.Promise.reject(
      new aWindow.DOMError("SyntaxError", "Non-numeric or invalid id"));
  },

  throwReadOnly: function(aWindow) {
    return aWindow.Promise.reject(
      new aWindow.DOMError("ReadOnlyError", "DataStore in readonly mode"));
  },

  exposeObject: function(aWindow) {
    let self = this;
    let object = {

      

      get name() {
        return self.name;
      },

      get owner() {
        return self.owner;
      },

      get readOnly() {
        return self.readOnly;
      },

      get: function DS_get(aId) {
        aId = parseInt(aId);
        if (isNaN(aId) || aId <= 0) {
          return self.throwInvalidArg(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readonly",
          function(aResolver, aTxn, aStore) {
            self.getInternal(aWindow, aResolver, aStore, aId);
          }
        );
      },

      update: function DS_update(aId, aObj) {
        aId = parseInt(aId);
        if (isNaN(aId) || aId <= 0) {
          return self.throwInvalidArg(aWindow);
        }

        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolver, aTxn, aStore) {
            self.updateInternal(aWindow, aResolver, aStore, aId, aObj);
          }
        );
      },

      add: function DS_add(aObj) {
        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolver, aTxn, aStore) {
            self.addInternal(aWindow, aResolver, aStore, aObj);
          }
        );
      },

      remove: function DS_remove(aId) {
        aId = parseInt(aId);
        if (isNaN(aId) || aId <= 0) {
          return self.throwInvalidArg(aWindow);
        }

        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolver, aTxn, aStore) {
            self.removeInternal(aResolver, aStore, aId);
          }
        );
      },

      clear: function DS_clear() {
        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolver, aTxn, aStore) {
            self.clearInternal(aResolver, aStore);
          }
        );
      },

      






      __exposedProps__: {
        name: 'r',
        owner: 'r',
        readOnly: 'r',
        get: 'r',
        update: 'r',
        add: 'r',
        remove: 'r',
        clear: 'r'
      }
    };

    return object;
  },

  delete: function() {
    this.db.delete();
  }
};
