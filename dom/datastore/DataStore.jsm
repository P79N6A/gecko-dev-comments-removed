





'use strict'

var EXPORTED_SYMBOLS = ["DataStore"];

function debug(s) {
  
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const REVISION_ADDED = "added";
const REVISION_UPDATED = "updated";
const REVISION_REMOVED = "removed";
const REVISION_VOID = "void";

Cu.import("resource://gre/modules/DataStoreDB.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");
Cu.import('resource://gre/modules/Services.jsm');


function createDOMError(aWindow, aEvent) {
  return new aWindow.DOMError(aEvent.target.error.name);
}



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
  revisionId: null,

  newDBPromise: function(aWindow, aTxnType, aFunction) {
    let db = this.db;
    return new aWindow.Promise(function(aResolver) {
      debug("DBPromise started");
      db.txn(
        aTxnType,
        function(aTxn, aStore, aRevisionStore) {
          debug("DBPromise success");
          aFunction(aResolver, aTxn, aStore, aRevisionStore);
        },
        function(aEvent) {
          debug("DBPromise error");
          aResolver.reject(createDOMError(aWindow, aEvent));
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
  },

  updateInternal: function(aWindow, aResolver, aStore, aRevisionStore, aId, aObj) {
    debug("UpdateInternal " + aId);

    let self = this;
    let request = aStore.put(aObj, aId);
    request.onsuccess = function(aEvent) {
      debug("UpdateInternal success");

      self.addRevision(aRevisionStore, aId, REVISION_UPDATED,
        function() {
          debug("UpdateInternal - revisionId increased");
          
          aResolver.resolve(aEvent.target.result);
        }
      );
    };
  },

  addInternal: function(aWindow, aResolver, aStore, aRevisionStore, aObj) {
    debug("AddInternal");

    let self = this;
    let request = aStore.put(aObj);
    request.onsuccess = function(aEvent) {
      debug("Request successful. Id: " + aEvent.target.result);
      self.addRevision(aRevisionStore, aEvent.target.result, REVISION_ADDED,
        function() {
          debug("AddInternal - revisionId increased");
          
          aResolver.resolve(aEvent.target.result);
        }
      );
    };
  },

  removeInternal: function(aResolver, aStore, aRevisionStore, aId) {
    debug("RemoveInternal");

    let self = this;
    let request = aStore.get(aId);
    request.onsuccess = function(aEvent) {
      debug("RemoveInternal success. Record: " + aEvent.target.result);
      if (aEvent.target.result === undefined) {
        aResolver.resolve(false);
        return;
      }

      let deleteRequest = aStore.delete(aId);
      deleteRequest.onsuccess = function() {
        debug("RemoveInternal success");
        self.addRevision(aRevisionStore, aId, REVISION_REMOVED,
          function() {
            aResolver.resolve(true);
          }
        );
      };
    };
  },

  clearInternal: function(aWindow, aResolver, aStore, aRevisionStore) {
    debug("ClearInternal");

    let self = this;
    let request = aStore.clear();
    request.onsuccess = function() {
      debug("ClearInternal success");
      self.db.clearRevisions(aRevisionStore,
        function() {
          debug("Revisions cleared");

          self.addRevision(aRevisionStore, 0, REVISION_VOID,
            function() {
              debug("ClearInternal - revisionId increased");
              aResolver.resolve();
            }
          );
        }
      );
    };
  },

  addRevision: function(aRevisionStore, aId, aType, aSuccessCb) {
    let self = this;
    this.db.addRevision(aRevisionStore, aId, aType,
      function(aRevisionId) {
        self.revisionId = aRevisionId;
        aSuccessCb();
      }
    );
  },

  retrieveRevisionId: function(aSuccessCb) {
    if (this.revisionId != null) {
      aSuccessCb();
      return;
    }

    let self = this;
    this.db.revisionTxn(
      'readwrite',
      function(aTxn, aRevisionStore) {
        debug("RetrieveRevisionId transaction success");

        let request = aRevisionStore.openCursor(null, 'prev');
        request.onsuccess = function(aEvent) {
          let cursor = aEvent.target.result;
          if (!cursor) {
            
            self.addRevision(aRevisionStore, 0, REVISION_VOID, aSuccessCb);
            return;
          }

          self.revisionId = cursor.value.revisionId;
          aSuccessCb();
        };
      }
    );
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
          function(aResolver, aTxn, aStore, aRevisionStore) {
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
          function(aResolver, aTxn, aStore, aRevisionStore) {
            self.updateInternal(aWindow, aResolver, aStore, aRevisionStore, aId, aObj);
          }
        );
      },

      add: function DS_add(aObj) {
        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolver, aTxn, aStore, aRevisionStore) {
            self.addInternal(aWindow, aResolver, aStore, aRevisionStore, aObj);
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
          function(aResolver, aTxn, aStore, aRevisionStore) {
            self.removeInternal(aResolver, aStore, aRevisionStore, aId);
          }
        );
      },

      clear: function DS_clear() {
        if (self.readOnly) {
          return self.throwReadOnly(aWindow);
        }

        
        return self.newDBPromise(aWindow, "readwrite",
          function(aResolver, aTxn, aStore, aRevisionStore) {
            self.clearInternal(aWindow, aResolver, aStore, aRevisionStore);
          }
        );
      },

      get revisionId() {
        return self.revisionId;
      },

      getChanges: function(aRevisionId) {
        debug("GetChanges: " + aRevisionId);

        if (aRevisionId === null || aRevisionId === undefined) {
          return aWindow.Promise.reject(
            new aWindow.DOMError("SyntaxError", "Invalid revisionId"));
        }

        
        return new aWindow.Promise(function(aResolver) {
          debug("GetChanges promise started");
          self.db.revisionTxn(
            'readonly',
            function(aTxn, aStore) {
              debug("GetChanges transaction success");

              let request = self.db.getInternalRevisionId(
                aRevisionId,
                aStore,
                function(aInternalRevisionId) {
                  if (aInternalRevisionId == undefined) {
                    aResolver.resolve(undefined);
                    return;
                  }

                  
                  
                  let changes = {
                    revisionId: '',
                    addedIds: {},
                    updatedIds: {},
                    removedIds: {}
                  };

                  let request = aStore.mozGetAll(aWindow.IDBKeyRange.lowerBound(aInternalRevisionId, true));
                  request.onsuccess = function(aEvent) {
                    for (let i = 0; i < aEvent.target.result.length; ++i) {
                      let data = aEvent.target.result[i];

                      switch (data.operation) {
                        case REVISION_ADDED:
                          changes.addedIds[data.objectId] = true;
                          break;

                        case REVISION_UPDATED:
                          
                          
                          
                          if (!(data.objectId in changes.addedIds) &&
                              !(data.objectId in changes.updatedIds)) {
                            changes.updatedIds[data.objectId] = true;
                          }
                          break;

                        case REVISION_REMOVED:
                          let id = data.objectId;

                          
                          
                          if (id in changes.addedIds) {
                            delete changes.addedIds[id];
                          } else {
                            changes.removedIds[id] = true;
                          }

                          if (id in changes.updatedIds) {
                            delete changes.updatedIds[id];
                          }
                          break;
                      }
                    }

                    
                    if (aEvent.target.result.length) {
                      changes.revisionId = aEvent.target.result[aEvent.target.result.length - 1].revisionId;
                    }

                    
                    changes.addedIds = Object.keys(changes.addedIds).map(function(aKey) { return parseInt(aKey, 10); });
                    changes.updatedIds = Object.keys(changes.updatedIds).map(function(aKey) { return parseInt(aKey, 10); });
                    changes.removedIds = Object.keys(changes.removedIds).map(function(aKey) { return parseInt(aKey, 10); });

                    let wrappedObject = ObjectWrapper.wrap(changes, aWindow);
                    aResolver.resolve(wrappedObject);
                  };
                }
              );
            },
            function(aEvent) {
              debug("GetChanges transaction failed");
              aResolver.reject(createDOMError(aWindow, aEvent));
            }
          );
        });
      },

      




      __exposedProps__: {
        name: 'r',
        owner: 'r',
        readOnly: 'r',
        get: 'r',
        update: 'r',
        add: 'r',
        remove: 'r',
        clear: 'r',
        revisionId: 'r',
        getChanges: 'r'
      }
    };

    return object;
  },

  delete: function() {
    this.db.delete();
  }
};
