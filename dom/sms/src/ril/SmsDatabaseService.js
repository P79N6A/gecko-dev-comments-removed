



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");

const RIL_SMSDATABASESERVICE_CONTRACTID = "@mozilla.org/sms/rilsmsdatabaseservice;1";
const RIL_SMSDATABASESERVICE_CID = Components.ID("{a1fa610c-eb6c-4ac2-878f-b005d5e89249}");

const DEBUG = true;
const DB_NAME = "sms";
const DB_VERSION = 7;
const STORE_NAME = "sms";
const MOST_RECENT_STORE_NAME = "most-recent";

const DELIVERY_SENT = "sent";
const DELIVERY_RECEIVED = "received";

const DELIVERY_STATUS_NOT_APPLICABLE = "not-applicable";
const DELIVERY_STATUS_SUCCESS = "success";
const DELIVERY_STATUS_PENDING = "pending";
const DELIVERY_STATUS_ERROR = "error";

const MESSAGE_CLASS_NORMAL = "normal";

const FILTER_TIMESTAMP = "timestamp";
const FILTER_NUMBERS = "numbers";
const FILTER_DELIVERY = "delivery";
const FILTER_READ = "read";



const FILTER_READ_UNREAD = 0;
const FILTER_READ_READ = 1;

const READ_ONLY = "readonly";
const READ_WRITE = "readwrite";
const PREV = "prev";
const NEXT = "next";

XPCOMUtils.defineLazyServiceGetter(this, "gSmsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");

XPCOMUtils.defineLazyServiceGetter(this, "gIDBManager",
                                   "@mozilla.org/dom/indexeddb/manager;1",
                                   "nsIIndexedDatabaseManager");

const GLOBAL_SCOPE = this;

function numberFromMessage(message) {
  return message.delivery == DELIVERY_SENT ? message.receiver : message.sender;
}




function SmsDatabaseService() {
  
  
  Services.dirsvc.get("ProfD", Ci.nsIFile);

  gIDBManager.initWindowless(GLOBAL_SCOPE);

  let that = this;
  this.newTxn(READ_ONLY, function(error, txn, store){
    if (error) {
      return;
    }
    
    
    let request = store.openCursor(null, PREV);
    request.onsuccess = function onsuccess(event) {
      let cursor = event.target.result;
      if (!cursor) {
        if (DEBUG) {
          debug("Could not get the last key from sms database. " +
                "Probably empty database");
        }
        return;
      }
      that.lastKey = cursor.key || 0;
      if (DEBUG) debug("Last assigned message ID was " + that.lastKey);
    };
    request.onerror = function onerror(event) {
      if (DEBUG) {
        debug("Could not get the last key from sms database " +
              event.target.errorCode);
      }
    };
  });

  this.messageLists = {};

  
  
  
  
  
  this.cursorReqs = {};
}
SmsDatabaseService.prototype = {

  classID:   RIL_SMSDATABASESERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISmsDatabaseService,
                                         Ci.nsIObserver]),

  


  db: null,

  



  messageLists: null,
  cursorReqs: null,

  lastMessageListId: 0,
  lastCursorReqId: 0,

  


  lastKey: 0,

  


  observe: function observe() {},

  










  ensureDB: function ensureDB(callback) {
    if (this.db) {
      if (DEBUG) debug("ensureDB: already have a database, returning early.");
      callback(null, this.db);
      return;
    }

    let self = this;
    function gotDB(db) {
      self.db = db;
      callback(null, db);
    }

    let request = GLOBAL_SCOPE.indexedDB.open(DB_NAME, DB_VERSION);
    request.onsuccess = function (event) {
      if (DEBUG) debug("Opened database:", DB_NAME, DB_VERSION);
      gotDB(event.target.result);
    };
    request.onupgradeneeded = function (event) {
      if (DEBUG) {
        debug("Database needs upgrade:", DB_NAME,
              event.oldVersion, event.newVersion);
        debug("Correct new database version:", event.newVersion == DB_VERSION);
      }

      let db = event.target.result;

      let currentVersion = event.oldVersion;
      while (currentVersion != event.newVersion) {
        switch (currentVersion) {
          case 0:
            if (DEBUG) debug("New database");
            self.createSchema(db);
            break;
          case 1:
            if (DEBUG) debug("Upgrade to version 2. Including `read` index");
            let objectStore = event.target.transaction.objectStore(STORE_NAME);
            self.upgradeSchema(objectStore);
            break;
          case 2:
            if (DEBUG) debug("Upgrade to version 3. Fix existing entries.");
            objectStore = event.target.transaction.objectStore(STORE_NAME);
            self.upgradeSchema2(objectStore);
            break;
          case 3:
            if (DEBUG) debug("Upgrade to version 4. Add quick threads view.");
            self.upgradeSchema3(db, event.target.transaction);
            break;
          case 4:
            if (DEBUG) debug("Upgrade to version 5. Populate quick threads view.");
            self.upgradeSchema4(event.target.transaction);
            break;
          case 5:
            if (DEBUG) debug("Upgrade to version 6. Use PhonenumberJS.")
            self.upgradeSchema5(event.target.transaction);
            break;
          case 6:
            if (DEBUG) debug("Upgrade to version 7. Add a `senderOrReceiver` field.");
            objectStore = event.target.transaction.objectStore(STORE_NAME);
            self.upgradeSchema6(objectStore);
            break;
          default:
            event.target.transaction.abort();
            callback("Old database version: " + event.oldVersion, null);
            break;
        }
        currentVersion++;
      }
    }
    request.onerror = function (event) {
      
      callback("Error opening database!", null);
    };
    request.onblocked = function (event) {
      callback("Opening database request is blocked.", null);
    };
  },

  











  newTxn: function newTxn(txn_type, callback, objectStores) {
    if (!objectStores) {
      objectStores = [STORE_NAME];
    }
    if (DEBUG) debug("Opening transaction for objectStores: " + objectStores);
    this.ensureDB(function (error, db) {
      if (error) {
        if (DEBUG) debug("Could not open database: " + error);
        callback(error);
        return;
      }
      let txn = db.transaction(objectStores, txn_type);
      if (DEBUG) debug("Started transaction " + txn + " of type " + txn_type);
      if (DEBUG) {
        txn.oncomplete = function oncomplete(event) {
          debug("Transaction " + txn + " completed.");
        };
        txn.onerror = function onerror(event) {
          
          
          debug("Error occurred during transaction: " + event.target.errorCode);
        };
      }
      let stores;
      if (objectStores.length == 1) {
        if (DEBUG) debug("Retrieving object store " + objectStores[0]);
        stores = txn.objectStore(objectStores[0]);
      } else {
        stores = [];
        for each (let storeName in objectStores) {
          if (DEBUG) debug("Retrieving object store " + storeName);
          stores.push(txn.objectStore(storeName));
        }
      }
      callback(null, txn, stores);
    });
  },

  





  createSchema: function createSchema(db) {
    
    let objectStore = db.createObjectStore(STORE_NAME, { keyPath: "id" });
    objectStore.createIndex("delivery", "delivery", { unique: false });
    objectStore.createIndex("sender", "sender", { unique: false });
    objectStore.createIndex("receiver", "receiver", { unique: false });
    objectStore.createIndex("timestamp", "timestamp", { unique: false });
    if (DEBUG) debug("Created object stores and indexes");
  },

  


  upgradeSchema: function upgradeSchema(objectStore) {
    objectStore.createIndex("read", "read", { unique: false });
  },

  upgradeSchema2: function upgradeSchema2(objectStore) {
    objectStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        return;
      }

      let message = cursor.value;
      message.messageClass = MESSAGE_CLASS_NORMAL;
      message.deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
      cursor.update(message);
      cursor.continue();
    }
  },

  upgradeSchema3: function upgradeSchema3(db, transaction) {
    
    let objectStore = transaction.objectStore(STORE_NAME);
    if (objectStore.indexNames.contains("id")) {
      objectStore.deleteIndex("id");
    }

    










    objectStore = db.createObjectStore(MOST_RECENT_STORE_NAME,
                                       { keyPath: "senderOrReceiver" });
    objectStore.createIndex("timestamp", "timestamp");
  },

  upgradeSchema4: function upgradeSchema4(transaction) {
    let threads = {};
    let smsStore = transaction.objectStore(STORE_NAME);
    let mostRecentStore = transaction.objectStore(MOST_RECENT_STORE_NAME);

    smsStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        for (let thread in threads) {
          mostRecentStore.put(threads[thread]);
        }
        return;
      }

      let message = cursor.value;
      let contact = message.sender || message.receiver;

      if (contact in threads) {
        let thread = threads[contact];
        if (!message.read) {
          thread.unreadCount++;
        }
        if (message.timestamp > thread.timestamp) {
          thread.id = message.id;
          thread.body = message.body;
          thread.timestamp = message.timestamp;
        }
      } else {
        threads[contact] = {
          senderOrReceiver: contact,
          id: message.id,
          timestamp: message.timestamp,
          body: message.body,
          unreadCount: message.read ? 0 : 1
        }
      }
      cursor.continue();
    }
  },

  upgradeSchema5: function upgradeSchema5(transaction) {
    
  },

  upgradeSchema6: function upgradeSchema6(objectStore) {
    objectStore.createIndex("senderOrReceiver", "senderOrReceiver",
                            { unique: false });

    objectStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        if (DEBUG) debug("updgradeSchema6 done");
        return;
      }

      let message = cursor.value;
      message.senderOrReceiver =
        message.delivery == DELIVERY_SENT ? message.receiver
                                          : message.sender;

      cursor.update(message);
      cursor.continue();
    }
  },

  











  keyIntersection: function keyIntersection(keys, filter) {
    
    
    let result = keys[FILTER_TIMESTAMP];
    if (!result) {
      result = keys[FILTER_NUMBERS];
    } else if (keys[FILTER_NUMBERS].length || filter.numbers) {
      result = result.filter(function(i) {
        return keys[FILTER_NUMBERS].indexOf(i) != -1;
      });
    }
    if (keys[FILTER_DELIVERY].length || filter.delivery) {
      result = result.filter(function(i) {
        return keys[FILTER_DELIVERY].indexOf(i) != -1;
      });
    }
    if (keys[FILTER_READ].length || filter.read) {
      result = result.filter(function(i) {
        return keys[FILTER_READ].indexOf(i) != -1;
      });
    }
    return result;
  },

  











  onMessageListCreated: function onMessageListCreated(messageList, aRequest) {
    if (DEBUG) debug("Message list created: " + messageList);
    let self = this;
    self.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }

      let messageId = messageList.shift();
      if (DEBUG) debug ("Fetching message " + messageId);
      let request = store.get(messageId);
      let message;
      request.onsuccess = function (event) {
        message = request.result;
      };

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (!message) {
          aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
          return;
        }
        self.lastMessageListId += 1;
        self.messageLists[self.lastMessageListId] = messageList;
        let sms = gSmsService.createSmsMessage(message.id,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.read);
        aRequest.notifyMessageListCreated(self.lastMessageListId, sms);
      };
    });
  },

  saveMessage: function saveMessage(message) {
    this.lastKey += 1;
    message.id = this.lastKey;
    message.senderOrReceiver = message.sender || message.receiver;

    if (DEBUG) debug("Going to store " + JSON.stringify(message));
    this.newTxn(READ_WRITE, function(error, txn, stores) {
      if (error) {
        return;
      }
      
      stores[0].put(message);

      let number = numberFromMessage(message);

      
      stores[1].get(number).onsuccess = function(event) {
        let mostRecentEntry = event.target.result;
        if (mostRecentEntry) {
          let needsUpdate = false;

          if (mostRecentEntry.timestamp <= message.timestamp) {
            mostRecentEntry.timestamp = message.timestamp;
            mostRecentEntry.body = message.body;
            needsUpdate = true;
          }

          if (!message.read) {
            mostRecentEntry.unreadCount++;
            needsUpdate = true;
          }

          if (needsUpdate) {
            event.target.source.put(mostRecentEntry);
          }
        } else {
          event.target.source.add({ senderOrReceiver: number,
                                    timestamp: message.timestamp,
                                    body: message.body,
                                    id: message.id,
                                    unreadCount: message.read ? 0 : 1 });
        }
      }
    }, [STORE_NAME, MOST_RECENT_STORE_NAME]);
    
    return message.id;
  },


  



  saveReceivedMessage: function saveReceivedMessage(aSender, aBody, aMessageClass, aDate) {
    let sender = aSender;
    if (sender) {
      let parsedNumber = PhoneNumberUtils.parse(sender);
      sender = (parsedNumber && parsedNumber.internationalNumber)
               ? parsedNumber.internationalNumber
               : sender;
    }

    let message = {delivery:       DELIVERY_RECEIVED,
                   deliveryStatus: DELIVERY_STATUS_SUCCESS,
                   sender:         sender,
                   receiver:       null,
                   body:           aBody,
                   messageClass:   aMessageClass,
                   timestamp:      aDate,
                   read:           FILTER_READ_UNREAD};
    return this.saveMessage(message);
  },

  saveSentMessage: function saveSentMessage(aReceiver, aBody, aDate) {
    let receiver = aReceiver
    if (receiver) {
      let parsedNumber = PhoneNumberUtils.parse(receiver.toString());
      receiver = (parsedNumber && parsedNumber.internationalNumber)
                 ? parsedNumber.internationalNumber
                 : receiver;
    }

    let message = {delivery:       DELIVERY_SENT,
                   deliveryStatus: DELIVERY_STATUS_PENDING,
                   sender:         null,
                   receiver:       receiver,
                   body:           aBody,
                   messageClass:   MESSAGE_CLASS_NORMAL,
                   timestamp:      aDate,
                   read:           FILTER_READ_READ};
    return this.saveMessage(message);
  },

  setMessageDeliveryStatus: function setMessageDeliveryStatus(messageId, deliveryStatus) {
    if ((deliveryStatus != DELIVERY_STATUS_SUCCESS)
        && (deliveryStatus != DELIVERY_STATUS_ERROR)) {
      if (DEBUG) {
        debug("Setting message " + messageId + " deliveryStatus to values other"
              + " than 'success' and 'error'");
      }
      return;
    }
    if (DEBUG) {
      debug("Setting message " + messageId + " deliveryStatus to "
            + deliveryStatus);
    }
    this.newTxn(READ_WRITE, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug(error);
        return;
      }

      let getRequest = store.get(messageId);
      getRequest.onsuccess = function onsuccess(event) {
        let message = event.target.result;
        if (!message) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          return;
        }
        if (message.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          return;
        }
        
        if (message.deliveryStatus != DELIVERY_STATUS_PENDING) {
          if (DEBUG) {
            debug("The value of message.deliveryStatus is not 'pending' but "
                  + message.deliveryStatus);
          }
          return;
        }
        message.deliveryStatus = deliveryStatus;
        if (DEBUG) debug("Message.deliveryStatus set to: " + deliveryStatus);
        store.put(message);
      };
    });
  },

  
  
  
  _getMessageInternal: function getMessageInternal(messageId, aSuccess, aError) {
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug(error);
        aError(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      let request = store.mozGetAll(messageId);

      txn.oncomplete = function oncomplete() {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (request.result.length > 1) {
          if (DEBUG) debug("Got too many results for id " + messageId);
          aError(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        let data = request.result[0];
        if (!data) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aError(Ci.nsISmsRequest.NOT_FOUND_ERROR);
          return;
        }
        if (data.id != messageId) {
          if (DEBUG) {
            debug("Requested message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aError(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        let message = gSmsService.createSmsMessage(data.id,
                                                   data.delivery,
                                                   data.deliveryStatus,
                                                   data.sender,
                                                   data.receiver,
                                                   data.body,
                                                   data.messageClass,
                                                   data.timestamp,
                                                   data.read);
        aSuccess(message);
      };

      txn.onerror = function onerror(event) {
        if (DEBUG) {
          if (event.target)
            debug("Caught error on transaction", event.target.errorCode);
        }
        
        aError(Ci.nsISmsRequest.INTERNAL_ERROR);
      }
    });
  },

  getMessage: function getMessage(messageId, aRequest) {
    this._getMessageInternal(messageId,
      function(aMessage) {
        aRequest.notifyMessageGot(aMessage);
      },
      function(aError) {
        aRequest.notifyGetMessageFailed(aError);
      });
  },

  deleteMessage: function deleteMessage(messageId, aRequest) {
    let deleted = false;
    let self = this;
    this.newTxn(READ_WRITE, function (error, txn, stores) {
      if (error) {
        aRequest.notifyDeleteMessageFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction", event.target.errorCode);
        
        aRequest.notifyDeleteMessageFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      };

      const smsStore = stores[0];
      const mruStore = stores[1];

      let deleted = false;

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        aRequest.notifyMessageDeleted(deleted);
      };

      smsStore.get(messageId).onsuccess = function(event) {
        let message = event.target.result;
        if (message) {
          if (DEBUG) debug("Deleting message id " + messageId);

          
          event.target.source.delete(messageId).onsuccess = function(event) {
            deleted = true;

            
            let number = numberFromMessage(message);

            mruStore.get(number).onsuccess = function(event) {
              
              let mostRecentEntry = event.target.result;

              if (!message.read) {
                mostRecentEntry.unreadCount--;
              }

              if (mostRecentEntry.id == messageId) {
                
                message = null;

                
                smsStore.index("sender").openCursor(number, "prev").onsuccess = function(event) {
                  let cursor = event.target.result;
                  if (cursor) {
                    message = cursor.value;
                  }
                };

                
                smsStore.index("receiver").openCursor(number, "prev").onsuccess = function(event) {
                  let cursor = event.target.result;
                  if (cursor) {
                    if (!message || cursor.value.timeStamp > message.timestamp) {
                      message = cursor.value;
                    }
                  }

                  
                  
                  if (message) {
                    mostRecentEntry.id = message.id;
                    mostRecentEntry.timestamp = message.timestamp;
                    mostRecentEntry.body = message.body;
                    if (DEBUG) {
                      debug("Updating mru entry: " +
                            JSON.stringify(mostRecentEntry));
                    }
                    mruStore.put(mostRecentEntry);
                  }
                  else {
                    if (DEBUG) {
                      debug("Deleting mru entry for number '" + number + "'");
                    }
                    mruStore.delete(number);
                  }
                };
              } else if (!message.read) {
                
                if (DEBUG) {
                  debug("Updating unread count for number '" + number + "': " +
                        (mostRecentEntry.unreadCount + 1) + " -> " +
                        mostRecentEntry.unreadCount);
                }
                mruStore.put(mostRecentEntry);
              }
            };
          };
        } else if (DEBUG) {
          debug("Message id " + messageId + " does not exist");
        }
      };
    }, [STORE_NAME, MOST_RECENT_STORE_NAME]);
  },

  startCursorRequest: function startCurReq(aIndex, aKey, aDirection, aRequest) {
    if (DEBUG) debug("Starting cursor request on " + aIndex + " " + aDirection);

    let self = this;
    let id = self.lastCursorReqId += 1;
    let firstMessage = true;

    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug("Error creating transaction: " + error);
        aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }

      let cursor = store.index(aIndex).openKeyCursor(aKey, aDirection);
      self.cursorReqs[id] = { done: false,
                              cursor: cursor,
                              ids: [] }

      cursor.onsuccess = function(aEvent) {
        let result = aEvent.target.result;
        let cursor = self.cursorReqs[id];

        if (!result) {
          cursor.done = true;
          return;
        }

        let messageId = result.primaryKey;
        if (firstMessage) {
          self._getMessageInternal(messageId,
            function(aMessage) {
              aRequest.notifyMessageListCreated(id, aMessage);
            },
            function(aError) {
              aRequest.notifyReadMessageListFailed(aError);
            }
          );
          firstMessage = false;
        } else {
          cursor.ids.push(messageId);
        }
        result.continue();
      }

      cursor.onerror = function() {
        aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      }

      txn.oncomplete = function oncomplete(event) {
        
      }

      txn.onerror = function onerror(event) {
        aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      }
    });
  },

  createMessageList: function createMessageList(filter, reverse, aRequest) {
    if (DEBUG) {
      debug("Creating a message list. Filters:" +
            " startDate: " + filter.startDate +
            " endDate: " + filter.endDate +
            " delivery: " + filter.delivery +
            " numbers: " + filter.numbers +
            " read: " + filter.read +
            " reverse: " + reverse);
    }

    
    
    
    
    
    

    let constraintCount = (filter.delivery ? 1 : 0) +
                          (filter.numbers ? filter.numbers.length : 0) +
                          (filter.read ? 1 : 0) +
                          (filter.startDate ? 1 : 0) +
                          (filter.endDate ? 1 : 0);
    if (DEBUG) debug("Constraints found: " + constraintCount);

    let direction = reverse ? PREV : NEXT;

    if (constraintCount == 1) {
      
      let indexName;
      let keyRange;
      if (filter.delivery) {
        indexName = "delivery";
        keyRange = IDBKeyRange.only(filter.delivery);
      } else if (filter.numbers) {
        indexName = "senderOrReceiver";
        keyRange = IDBKeyRange.only(filter.numbers[0]);
      } else if (filter.read) {
        indexName = "read";
        let keyRange = IDBKeyRange.only(filter.read ? FILTER_READ_READ
                                                    : FILTER_READ_UNREAD);
      } else {
        indexName = "timestamp";
        if (filter.startDate != null && filter.endDate != null) {
          keyRange = IDBKeyRange.bound(filter.startDate.getTime(),
                                       filter.endDate.getTime());
        } else if (filter.startDate != null) {
          keyRange = IDBKeyRange.lowerBound(filter.startDate.getTime());
        } else if (filter.endDate != null) {
          keyRange = IDBKeyRange.upperBound(filter.endDate.getTime());
        }
      }

      if (indexName && keyRange) {
        this.startCursorRequest(indexName, keyRange, direction, aRequest);
        return;
      }
    } else if (constraintCount == 0) {
      
      this.startCursorRequest("timestamp", null, direction, aRequest);
      return;
    }

    
    
    
    
    let filteredKeys = {};
    filteredKeys[FILTER_TIMESTAMP] = null;
    filteredKeys[FILTER_NUMBERS] = [];
    filteredKeys[FILTER_DELIVERY] = [];
    filteredKeys[FILTER_READ] = [];

    
    let successCb = function onsuccess(result, filter) {
      
      
      if (!result) {
        if (DEBUG) {
          debug("These messages match the " + filter + " filter: " +
                filteredKeys[filter]);
        }
        return;
      }
      
      
      let primaryKey = result.primaryKey;
      filteredKeys[filter].push(primaryKey);
      result.continue();
    };

    let errorCb = function onerror(event) {
      
      if (DEBUG) debug("IDBRequest error " + event.target.errorCode);
      aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      return;
    };

    let self = this;
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        errorCb(error);
        return;
      }

      let filtered = false;

      
      
      if (filter.delivery) {
        filtered = true;
        let deliveryKeyRange = IDBKeyRange.only(filter.delivery);
        let deliveryRequest = store.index("delivery")
                                   .openKeyCursor(deliveryKeyRange);
        deliveryRequest.onsuccess = function onsuccess(event) {
          successCb(event.target.result, FILTER_DELIVERY);
        };
        deliveryRequest.onerror = errorCb;
      }

      
      
      if (filter.numbers) {
        for (let i = 0; i < filter.numbers.length; i++) {
          filtered = true;
          let numberKeyRange = IDBKeyRange.only(filter.numbers[i]);
          let numberRequest = store.index("senderOrReceiver")
                                   .openKeyCursor(numberKeyRange, direction);
          numberRequest.onsuccess =
            function onsuccess(event){
              successCb(event.target.result, FILTER_NUMBERS);
            };
          numberRequest.onerror = errorCb;
        }
      }

      
      
      if (filter.read != undefined) {
        filtered = true;
        let read = filter.read ? FILTER_READ_READ : FILTER_READ_UNREAD;
        if (DEBUG) debug("filter.read " + read);
        let readKeyRange = IDBKeyRange.only(read);
        let readRequest = store.index("read")
                               .openKeyCursor(readKeyRange);
        readRequest.onsuccess = function onsuccess(event) {
          successCb(event.target.result, FILTER_READ);
        };
        readRequest.onerror = errorCb;
      }

      
      
      
      let timeKeyRange = null;
      if (filter.startDate != null && filter.endDate != null) {
        timeKeyRange = IDBKeyRange.bound(filter.startDate.getTime(),
                                         filter.endDate.getTime());
      } else if (filter.startDate != null) {
        timeKeyRange = IDBKeyRange.lowerBound(filter.startDate.getTime());
      } else if (filter.endDate != null) {
        timeKeyRange = IDBKeyRange.upperBound(filter.endDate.getTime());
      }

      if (DEBUG)
        debug("timeKeyRange: " + timeKeyRange + " filtered: " + filtered);
      if (timeKeyRange || !filtered) {
        filteredKeys[FILTER_TIMESTAMP] = [];
        let timeRequest = store.index("timestamp").openKeyCursor(timeKeyRange,
                                                                 direction);

        timeRequest.onsuccess = function onsuccess(event) {
          successCb(event.target.result, FILTER_TIMESTAMP);
        };
        timeRequest.onerror = errorCb;
      } else {
        if (DEBUG) debug("Ignoring useless date filtering");
      }

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        
        
        let result =  self.keyIntersection(filteredKeys, filter);
        if (!result.length) {
          if (DEBUG) debug("No messages matching the filter criteria");
          aRequest.notifyNoMessageInList();
          return;
        }

        
        
        
        
        self.onMessageListCreated(result, aRequest);
      };

      txn.onerror = function onerror(event) {
        errorCb(event);
      };
    });
  },

  getNextMessageInList: function getNextMessageInList(listId, aRequest) {
    let getMessage = (function getMessage(messageId) {
      this._getMessageInternal(messageId,
        function(aMessage) {
          aRequest.notifyNextMessageInListGot(aMessage);
        },
        function(aError) {
          aRequest.notifyReadMessageListFailed(aError);
        }
      );
    }).bind(this);

    if (DEBUG) debug("Getting next message in list " + listId);
    let messageId;
    let list = this.messageLists[listId];
    if (!list) {
      if (this.cursorReqs[listId]) {
        let cursor = this.cursorReqs[listId];
        if (cursor.done && cursor.ids.length == 0) {
          aRequest.notifyNoMessageInList();
          return;
        }

        messageId = cursor.ids.shift();

        
        
        if (messageId) {
          getMessage(messageId);
          return;
        }

        
        
        cursor.cursor.addEventListener("success",
          function waitForResult(aEvent) {
            cursor.cursor.removeEventListener("success", waitForResult);
            
            if (cursor.done) {
              aRequest.notifyNoMessageInList();
              return;
            }

            
            messageId = cursor.ids.shift();
            getMessage(messageId);
          });

        return;
      }
      if (DEBUG) debug("Wrong list id");
      aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
      return;
    }
    messageId = list.shift();
    if (messageId == null) {
      if (DEBUG) debug("Reached the end of the list!");
      aRequest.notifyNoMessageInList();
      return;
    }

    getMessage(messageId);
  },

  clearMessageList: function clearMessageList(listId) {
    if (DEBUG) debug("Clearing message list: " + listId);
    if (this.messageLists[listId]) {
      delete this.messageLists[listId];
    } else if (this.cursorReqs[listId]) {
      delete this.cursorReqs[listId];
    }
  },

  markMessageRead: function markMessageRead(messageId, value, aRequest) {
    if (DEBUG) debug("Setting message " + messageId + " read to " + value);
    this.newTxn(READ_WRITE, function (error, txn, stores) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.errorCode);
        aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      };
      stores[0].get(messageId).onsuccess = function onsuccess(event) {
        let message = event.target.result;
        if (!message) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
          return;
        }
        if (message.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        
        
        if (message.read == value) {
          if (DEBUG) debug("The value of message.read is already " + value);
          aRequest.notifyMessageMarkedRead(message.read);
          return;
        }
        message.read = value ? FILTER_READ_READ : FILTER_READ_UNREAD;
        if (DEBUG) debug("Message.read set to: " + value);
        event.target.source.put(message).onsuccess = function onsuccess(event) {
          if (DEBUG) {
            debug("Update successfully completed. Message: " +
                  JSON.stringify(event.target.result));
          }

          
          let number = numberFromMessage(message);

          stores[1].get(number).onsuccess = function(event) {
            let mostRecentEntry = event.target.result;
            mostRecentEntry.unreadCount += value ? -1 : 1;
            if (DEBUG) {
              debug("Updating unreadCount for '" + number + "': " +
                    (value ?
                     mostRecentEntry.unreadCount + 1 :
                     mostRecentEntry.unreadCount - 1) +
                    " -> " + mostRecentEntry.unreadCount);
            }
            event.target.source.put(mostRecentEntry).onsuccess = function(event) {
              aRequest.notifyMessageMarkedRead(message.read);
            };
          };
        };
      };
    }, [STORE_NAME, MOST_RECENT_STORE_NAME]);
  },

  getThreadList: function getThreadList(aRequest) {
    if (DEBUG) debug("Getting thread list");
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyThreadListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.errorCode);
        aRequest.notifyThreadListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      };
      store.index("timestamp").mozGetAll().onsuccess = function(event) {
        aRequest.notifyThreadList(event.target.result);
      };
    }, [MOST_RECENT_STORE_NAME]);
  }
};

XPCOMUtils.defineLazyGetter(SmsDatabaseService.prototype, "mRIL", function () {
    return Cc["@mozilla.org/telephony/system-worker-manager;1"]
              .getService(Ci.nsIInterfaceRequestor)
              .getInterface(Ci.nsIRadioInterfaceLayer);
});

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SmsDatabaseService]);

function debug() {
  dump("SmsDatabaseService: " + Array.slice(arguments).join(" ") + "\n");
}
