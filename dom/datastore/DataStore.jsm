





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
    return new aWindow.Promise(function(aResolve, aReject) {
      debug("DBPromise started");
      db.txn(
        aTxnType,
        function(aTxn, aStore) {
          debug("DBPromise success");
          aFunction(aResolve, aReject, aTxn, aStore);
        },
        function() {
          debug("DBPromise error");
          aReject(new aWindow.DOMError("InvalidStateError"));
        }
      );
    });
  },

  getInternal: function(aWindow, aResolve, aReject, aStore, aId) {
    debug("GetInternal " + aId);

    let request = aStore.get(aId);
    request.onsuccess = function(aEvent) {
      debug("GetInternal success. Record: " + aEvent.target.result);
      aResolve(ObjectWrapper.wrap(aEvent.target.result, aWindow));
    };

    request.onerror = function(aEvent) {
      debug("GetInternal error");
      aReject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  updateInternal: function(aWindow, aResolve, aReject, aStore, aId, aObj) {
    debug("UpdateInternal " + aId);

    let request = aStore.put(aObj, aId);
    request.onsuccess = function(aEvent) {
      debug("UpdateInternal success");
      
      aResolve(aEvent.target.result);
    };
    request.onerror = function(aEvent) {
      debug("UpdateInternal error");
      aReject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  addInternal: function(aWindow, aResolve, aReject, aStore, aObj) {
    debug("AddInternal");

    let request = aStore.put(aObj);
    request.onsuccess = function(aEvent) {
      debug("Request successful. Id: " + aEvent.target.result);
      
      aResolve(aEvent.target.result);
    };
    request.onerror = function(aEvent) {
      debug("AddInternal error");
      aReject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  removeInternal: function(aResolve, aReject, aStore, aId) {
    debug("RemoveInternal");

    let request = aStore.delete(aId);
    request.onsuccess = function() {
      debug("RemoveInternal success");
      aResolve();
    };
    request.onerror = function(aEvent) {
      debug("RemoveInternal error");
      aReject(new aWindow.DOMError(aEvent.target.error.name));
    };
  },

  clearInternal: function(aResolve, aReject, aStore) {
    debug("ClearInternal");

    let request = aStore.clear();
    request.onsuccess = function() {
      debug("ClearInternal success");
      aResolve();
    };
    request.onerror = function(aEvent) {
      debug("ClearInternal error");
      aReject(new aWindow.DOMError(aEvent.target.error.name));
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
          function(aResolve, aReject, aTxn, aStore) {
            self.getInternal(aWindow, aResolve, aReject, aStore, aId);
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
          function(aResolve, aReject, aTxn, aStore) {
            self.updateInternal(aWindow, aResolve, aReject, aStore, aId, aObj);
          }
        );
      },

      add: function DS_add(aObj) {
        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolve, aReject, aTxn, aStore) {
            self.addInternal(aWindow, aResolve, aReject, aStore, aObj);
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
          function(aResolve, aReject, aTxn, aStore) {
            self.removeInternal(aResolve, aReject, aStore, aId);
          }
        );
      },

      clear: function DS_clear() {
        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolve, aReject, aTxn, aStore) {
            self.clearInternal(aResolve, aReject, aStore);
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
