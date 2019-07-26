





'use strict'

this.EXPORTED_SYMBOLS = ["DataStore", "DataStoreAccess"];

function debug(s) {
  
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const REVISION_ADDED = "added";
const REVISION_UPDATED = "updated";
const REVISION_REMOVED = "removed";
const REVISION_VOID = "void";



const MAX_REQUESTS = 25;

Cu.import("resource://gre/modules/DataStoreDB.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");


function createDOMError(aWindow, aEvent) {
  return new aWindow.DOMError(aEvent.target.error.name);
}

function throwInvalidArg(aWindow) {
  return aWindow.Promise.reject(
    new aWindow.DOMError("SyntaxError", "Non-numeric or invalid id"));
}

function throwReadOnly(aWindow) {
  return aWindow.Promise.reject(
    new aWindow.DOMError("ReadOnlyError", "DataStore in readonly mode"));
}

function parseIds(aId) {
  function parseId(aId) {
    aId = parseInt(aId);
    return (isNaN(aId) || aId <= 0) ? null : aId;
  }

  if (!Array.isArray(aId)) {
    return parseId(aId);
  }

  for (let i = 0; i < aId.length; ++i) {
    aId[i] = parseId(aId[i]);
    if (aId[i] === null) {
      return null;
    }
  }

  return aId;
}


function ExposedDataStore(aWindow, aDataStore, aReadOnly) {
  debug("ExposedDataStore created");
  this.init(aWindow, aDataStore, aReadOnly);
}

ExposedDataStore.prototype = {
  classDescription: "DataStore XPCOM Component",
  classID: Components.ID("{db5c9602-030f-4bff-a3de-881a8de370f2}"),
  contractID: "@mozilla.org/dom/datastore;1",
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsISupports]),

  callbacks: [],

  init: function(aWindow, aDataStore, aReadOnly) {
    debug("ExposedDataStore init");

    this.window = aWindow;
    this.dataStore = aDataStore;
    this.isReadOnly = aReadOnly;
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage");

    if (aMessage.name != "DataStore:Changed:Return:OK") {
      debug("Wrong message: " + aMessage.name);
      return;
    }

    let self = this;

    this.dataStore.retrieveRevisionId(
      function() {
        let event = new self.window.DataStoreChangeEvent('change', aMessage.data);
        self.__DOM_IMPL__.dispatchEvent(event);
      },
      
      true
    );
  },

  

  get name() {
    return this.dataStore.name;
  },

  get owner() {
    return this.dataStore.owner;
  },

  get readOnly() {
    return this.isReadOnly;
  },

  get: function(aId) {
    aId = parseIds(aId);
    if (aId === null) {
      return throwInvalidArg(this.window);
    }

    let self = this;

    
    return this.dataStore.newDBPromise(this.window, "readonly",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
               self.dataStore.getInternal(aStore,
                                          Array.isArray(aId) ?  aId : [ aId ],
                                          function(aResults) {
          aResolve(Array.isArray(aId) ? aResults : aResults[0]);
        });
      }
    );
  },

  update: function(aId, aObj) {
    aId = parseInt(aId);
    if (isNaN(aId) || aId <= 0) {
      return throwInvalidArg(this.window);
    }

    if (this.isReadOnly) {
      return throwReadOnly(this.window);
    }

    let self = this;

    
    return this.dataStore.newDBPromise(this.window, "readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.dataStore.updateInternal(aResolve, aStore, aRevisionStore, aId, aObj);
      }
    );
  },

  add: function(aObj) {
    if (this.isReadOnly) {
      return throwReadOnly(this.window);
    }

    let self = this;

    
    return this.dataStore.newDBPromise(this.window, "readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.dataStore.addInternal(aResolve, aStore, aRevisionStore, aObj);
      }
    );
  },

  remove: function(aId) {
    aId = parseInt(aId);
    if (isNaN(aId) || aId <= 0) {
      return throwInvalidArg(this.window);
    }

    if (this.isReadOnly) {
      return throwReadOnly(this.window);
    }

    let self = this;

    
    return this.dataStore.newDBPromise(this.window, "readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.dataStore.removeInternal(aResolve, aStore, aRevisionStore, aId);
      }
    );
  },

  clear: function() {
    if (this.isReadOnly) {
      return throwReadOnly(this.window);
    }

    let self = this;

    
    return this.dataStore.newDBPromise(this.window, "readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.dataStore.clearInternal(aResolve, aStore, aRevisionStore);
      }
    );
  },

  get revisionId() {
    return this.dataStore.revisionId;
  },

  getChanges: function(aRevisionId) {
    debug("GetChanges: " + aRevisionId);

    if (aRevisionId === null || aRevisionId === undefined) {
      return this.window.Promise.reject(
        new this.window.DOMError("SyntaxError", "Invalid revisionId"));
    }

    let self = this;

    
    return new this.window.Promise(function(aResolve, aReject) {
      debug("GetChanges promise started");
      self.dataStore.db.revisionTxn(
        'readonly',
        function(aTxn, aStore) {
          debug("GetChanges transaction success");

          let request = self.dataStore.db.getInternalRevisionId(
            aRevisionId,
            aStore,
            function(aInternalRevisionId) {
              if (aInternalRevisionId == undefined) {
                aResolve(undefined);
                return;
              }

              
              
              let changes = {
                revisionId: '',
                addedIds: {},
                updatedIds: {},
                removedIds: {}
              };

              let request = aStore.mozGetAll(self.window.IDBKeyRange.lowerBound(aInternalRevisionId, true));
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

                let wrappedObject = ObjectWrapper.wrap(changes, self.window);
                aResolve(wrappedObject);
              };
            }
          );
        },
        function(aEvent) {
          debug("GetChanges transaction failed");
          aReject(createDOMError(self.window, aEvent));
        }
      );
    });
  },

  getLength: function() {
    let self = this;

    
    return this.dataStore.newDBPromise(this.window, "readonly",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.dataStore.getLengthInternal(aResolve, aStore);
      }
    );
  },

  set onchange(aCallback) {
    debug("Set OnChange");
    this.__DOM_IMPL__.setEventHandler("onchange", aCallback);
  },

  get onchange() {
    debug("Get OnChange");
    return this.__DOM_IMPL__.getEventHandler("onchange");
  }
};



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
    return new aWindow.Promise(function(aResolve, aReject) {
      debug("DBPromise started");
      db.txn(
        aTxnType,
        function(aTxn, aStore, aRevisionStore) {
          debug("DBPromise success");
          aFunction(aResolve, aReject, aTxn, aStore, aRevisionStore);
        },
        function(aEvent) {
          debug("DBPromise error");
          aReject(createDOMError(aWindow, aEvent));
        }
      );
    });
  },

  getInternal: function(aStore, aIds, aCallback) {
    debug("GetInternal: " + aIds.toSource());

    
    let results = new Array(aIds.length);

    
    let pendingIds = aIds.length;
    let indexPos = 0;

    function getInternalSuccess(aEvent, aPos) {
      debug("GetInternal success. Record: " + aEvent.target.result);
      results[aPos] = aEvent.target.result;
      if (!--pendingIds) {
        aCallback(results);
        return;
      }

      if (indexPos < aIds.length) {
        
        let count = 0;
        while (indexPos < aIds.length && ++count < MAX_REQUESTS) {
          getInternalRequest();
        }
      }
    }

    function getInternalRequest() {
      let currentPos = indexPos++;
      let request = aStore.get(aIds[currentPos]);
      request.onsuccess = function(aEvent) {
        getInternalSuccess(aEvent, currentPos);
      }
    }

    getInternalRequest();
  },

  updateInternal: function(aResolve, aStore, aRevisionStore, aId, aObj) {
    debug("UpdateInternal " + aId);

    let self = this;
    let request = aStore.put(aObj, aId);
    request.onsuccess = function(aEvent) {
      debug("UpdateInternal success");

      self.addRevision(aRevisionStore, aId, REVISION_UPDATED,
        function() {
          debug("UpdateInternal - revisionId increased");
          
          aResolve(aEvent.target.result);
        }
      );
    };
  },

  addInternal: function(aResolve, aStore, aRevisionStore, aObj) {
    debug("AddInternal");

    let self = this;
    let request = aStore.put(aObj);
    request.onsuccess = function(aEvent) {
      debug("Request successful. Id: " + aEvent.target.result);
      self.addRevision(aRevisionStore, aEvent.target.result, REVISION_ADDED,
        function() {
          debug("AddInternal - revisionId increased");
          
          aResolve(aEvent.target.result);
        }
      );
    };
  },

  removeInternal: function(aResolve, aStore, aRevisionStore, aId) {
    debug("RemoveInternal");

    let self = this;
    let request = aStore.get(aId);
    request.onsuccess = function(aEvent) {
      debug("RemoveInternal success. Record: " + aEvent.target.result);
      if (aEvent.target.result === undefined) {
        aResolve(false);
        return;
      }

      let deleteRequest = aStore.delete(aId);
      deleteRequest.onsuccess = function() {
        debug("RemoveInternal success");
        self.addRevision(aRevisionStore, aId, REVISION_REMOVED,
          function() {
            aResolve(true);
          }
        );
      };
    };
  },

  clearInternal: function(aResolve, aStore, aRevisionStore) {
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
              aResolve();
            }
          );
        }
      );
    };
  },

  getLengthInternal: function(aResolve, aStore) {
    debug("GetLengthInternal");

    let request = aStore.count();
    request.onsuccess = function(aEvent) {
      debug("GetLengthInternal success: " + aEvent.target.result);
      
      aResolve(aEvent.target.result);
    };
  },

  addRevision: function(aRevisionStore, aId, aType, aSuccessCb) {
    let self = this;
    this.db.addRevision(aRevisionStore, aId, aType,
      function(aRevisionId) {
        self.revisionId = aRevisionId;
        self.sendNotification(aId, aType, aRevisionId);
        aSuccessCb();
      }
    );
  },

  retrieveRevisionId: function(aSuccessCb, aForced) {
    if (this.revisionId != null && !aForced) {
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
            
            self.addRevision(aRevisionStore, 0, REVISION_VOID,
              function(aRevisionId) {
                self.revisionId = aRevisionId;
                aSuccessCb();
              }
            );
            return;
          }

          self.revisionId = cursor.value.revisionId;
          aSuccessCb();
        };
      }
    );
  },

  exposeObject: function(aWindow, aReadOnly) {
    debug("Exposing Object");

    let exposedObject = new ExposedDataStore(aWindow, this, aReadOnly);
    let object = aWindow.DataStore._create(aWindow, exposedObject);

    Services.obs.addObserver(function(aSubject, aTopic, aData) {
      let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (wId == exposedObject.innerWindowID) {
        cpmm.removeMessageListener("DataStore:Changed:Return:OK", exposedObject);
      }
    }, "inner-window-destroyed", false);

    let util = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils);
    exposedObject.innerWindowID = util.currentInnerWindowID;

    cpmm.addMessageListener("DataStore:Changed:Return:OK", exposedObject);
    cpmm.sendAsyncMessage("DataStore:RegisterForMessages",
                          { store: this.name, owner: this.owner });

    return object;
  },

  delete: function() {
    this.db.delete();
  },

  sendNotification: function(aId, aOperation, aRevisionId) {
    debug("SendNotification");
    if (aOperation != REVISION_VOID) {
      cpmm.sendAsyncMessage("DataStore:Changed",
                            { store: this.name, owner: this.owner,
                              message: { revisionId: aRevisionId, id: aId,
                                         operation: aOperation } } );
    }
  }
};



function DataStoreAccess(aAppId, aName, aOrigin, aReadOnly) {
  this.appId = aAppId;
  this.name = aName;
  this.origin = aOrigin;
  this.readOnly = aReadOnly;
}

DataStoreAccess.prototype = {};
