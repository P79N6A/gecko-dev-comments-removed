





'use strict'

this.EXPORTED_SYMBOLS = ["DataStore"];

function debug(s) {
  
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const REVISION_ADDED = "added";
const REVISION_UPDATED = "updated";
const REVISION_REMOVED = "removed";
const REVISION_VOID = "void";



const MAX_REQUESTS = 25;

Cu.import("resource://gre/modules/DataStoreCursor.jsm");
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


this.DataStore = function(aWindow, aName, aOwner, aReadOnly) {
  debug("DataStore created");
  this.init(aWindow, aName, aOwner, aReadOnly);
}

this.DataStore.prototype = {
  classDescription: "DataStore XPCOM Component",
  classID: Components.ID("{db5c9602-030f-4bff-a3de-881a8de370f2}"),
  contractID: "@mozilla.org/dom/datastore;1",
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsISupports]),

  callbacks: [],

  _window: null,
  _name: null,
  _owner: null,
  _readOnly: null,
  _revisionId: null,
  _exposedObject: null,
  _cursor: null,

  init: function(aWindow, aName, aOwner, aReadOnly) {
    debug("DataStore init");

    this._window = aWindow;
    this._name = aName;
    this._owner = aOwner;
    this._readOnly = aReadOnly;

    this._db = new DataStoreDB();
    this._db.init(aOwner, aName);

    let self = this;
    Services.obs.addObserver(function(aSubject, aTopic, aData) {
      let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (wId == self._innerWindowID) {
        cpmm.removeMessageListener("DataStore:Changed:Return:OK", self);
        self._db.close();
      }
    }, "inner-window-destroyed", false);

    let util = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils);
    this._innerWindowID = util.currentInnerWindowID;

    cpmm.addMessageListener("DataStore:Changed:Return:OK", this);
    cpmm.sendAsyncMessage("DataStore:RegisterForMessages",
                          { store: this._name, owner: this._owner });
  },

  newDBPromise: function(aTxnType, aFunction) {
    let self = this;
    return new this._window.Promise(function(aResolve, aReject) {
      debug("DBPromise started");
      self._db.txn(
        aTxnType,
        function(aTxn, aStore, aRevisionStore) {
          debug("DBPromise success");
          aFunction(aResolve, aReject, aTxn, aStore, aRevisionStore);
        },
        function(aEvent) {
          debug("DBPromise error");
          aReject(createDOMError(self._window, aEvent));
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
      self._db.clearRevisions(aRevisionStore,
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
    this._db.addRevision(aRevisionStore, aId, aType,
      function(aRevisionId) {
        self._revisionId = aRevisionId;
        self.sendNotification(aId, aType, aRevisionId);
        aSuccessCb();
      }
    );
  },

  retrieveRevisionId: function(aSuccessCb) {
    let self = this;
    this._db.revisionTxn(
      'readonly',
      function(aTxn, aRevisionStore) {
        debug("RetrieveRevisionId transaction success");

        let request = aRevisionStore.openCursor(null, 'prev');
        request.onsuccess = function(aEvent) {
          let cursor = aEvent.target.result;
          if (cursor) {
            self._revisionId = cursor.value.revisionId;
          }

          aSuccessCb(self._revisionId);
        };
      }
    );
  },

  sendNotification: function(aId, aOperation, aRevisionId) {
    debug("SendNotification");
    if (aOperation != REVISION_VOID) {
      cpmm.sendAsyncMessage("DataStore:Changed",
                            { store: this.name, owner: this.owner,
                              message: { revisionId: aRevisionId, id: aId,
                                         operation: aOperation } } );
    }
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage");

    if (aMessage.name != "DataStore:Changed:Return:OK") {
      debug("Wrong message: " + aMessage.name);
      return;
    }

    let self = this;

    this.retrieveRevisionId(
      function() {
        
        if (self._cursor) {
          return;
        }

        let event = new self._window.DataStoreChangeEvent('change', aMessage.data);
        self.__DOM_IMPL__.dispatchEvent(event);
      }
    );
  },

  get exposedObject() {
    debug("get exposedObject");
    return this._exposedObject;
  },

  set exposedObject(aObject) {
    debug("set exposedObject");
    this._exposedObject = aObject;
  },

  syncTerminated: function(aCursor) {
    
    if (this._cursor == aCursor) {
      this._cursor = null;
    }
  },

  

  get name() {
    return this._name;
  },

  get owner() {
    return this._owner;
  },

  get readOnly() {
    return this._readOnly;
  },

  get: function(aId) {
    aId = parseIds(aId);
    if (aId === null) {
      return throwInvalidArg(this._window);
    }

    let self = this;

    
    return this.newDBPromise("readonly",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
               self.getInternal(aStore,
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
      return throwInvalidArg(this._window);
    }

    if (this._readOnly) {
      return throwReadOnly(this._window);
    }

    let self = this;

    
    return this.newDBPromise("readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.updateInternal(aResolve, aStore, aRevisionStore, aId, aObj);
      }
    );
  },

  add: function(aObj) {
    if (this._readOnly) {
      return throwReadOnly(this._window);
    }

    let self = this;

    
    return this.newDBPromise("readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.addInternal(aResolve, aStore, aRevisionStore, aObj);
      }
    );
  },

  remove: function(aId) {
    aId = parseInt(aId);
    if (isNaN(aId) || aId <= 0) {
      return throwInvalidArg(this._window);
    }

    if (this._readOnly) {
      return throwReadOnly(this._window);
    }

    let self = this;

    
    return this.newDBPromise("readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.removeInternal(aResolve, aStore, aRevisionStore, aId);
      }
    );
  },

  clear: function() {
    if (this._readOnly) {
      return throwReadOnly(this._window);
    }

    let self = this;

    
    return this.newDBPromise("readwrite",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.clearInternal(aResolve, aStore, aRevisionStore);
      }
    );
  },

  get revisionId() {
    return this._revisionId;
  },

  getChanges: function(aRevisionId) {
    debug("GetChanges: " + aRevisionId);

    if (aRevisionId === null || aRevisionId === undefined) {
      return this._window.Promise.reject(
        new this._window.DOMError("SyntaxError", "Invalid revisionId"));
    }

    let self = this;

    
    return new this._window.Promise(function(aResolve, aReject) {
      debug("GetChanges promise started");
      self._db.revisionTxn(
        'readonly',
        function(aTxn, aStore) {
          debug("GetChanges transaction success");

          let request = self._db.getInternalRevisionId(
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

              let request = aStore.mozGetAll(IDBKeyRange.lowerBound(aInternalRevisionId, true));
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

                let wrappedObject = ObjectWrapper.wrap(changes, self._window);
                aResolve(wrappedObject);
              };
            }
          );
        },
        function(aEvent) {
          debug("GetChanges transaction failed");
          aReject(createDOMError(self._window, aEvent));
        }
      );
    });
  },

  getLength: function() {
    let self = this;

    
    return this.newDBPromise("readonly",
      function(aResolve, aReject, aTxn, aStore, aRevisionStore) {
        self.getLengthInternal(aResolve, aStore);
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
  },

  sync: function(aRevisionId) {
    debug("Sync");
    this._cursor = new DataStoreCursor(this._window, this, aRevisionId);
    return this._window.DataStoreCursor._create(this._window, this._cursor);
  }
};
