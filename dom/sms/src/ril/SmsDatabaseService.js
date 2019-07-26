



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const RIL_SMSDATABASESERVICE_CONTRACTID = "@mozilla.org/sms/rilsmsdatabaseservice;1";
const RIL_SMSDATABASESERVICE_CID = Components.ID("{a1fa610c-eb6c-4ac2-878f-b005d5e89249}");

const DEBUG = false;
const DB_NAME = "sms";
const DB_VERSION = 3;
const STORE_NAME = "sms";

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

XPCOMUtils.defineLazyServiceGetter(this, "gSmsRequestManager",
                                   "@mozilla.org/sms/smsrequestmanager;1",
                                   "nsISmsRequestManager");

XPCOMUtils.defineLazyServiceGetter(this, "gIDBManager",
                                   "@mozilla.org/dom/indexeddb/manager;1",
                                   "nsIIndexedDatabaseManager");

const GLOBAL_SCOPE = this;




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
}
SmsDatabaseService.prototype = {

  classID:   RIL_SMSDATABASESERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISmsDatabaseService,
                                         Ci.nsIObserver]),

  


  db: null,

  



  messageLists: null,

  lastMessageListId: 0,

  


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

      switch (event.oldVersion) {
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
          if (DEBUG) debug("Upgrade to version 3. Fix existing entries.")
          objectStore = event.target.transaction.objectStore(STORE_NAME);
          self.upgradeSchema2(objectStore);
          break;

        default:
          event.target.transaction.abort();
          callback("Old database version: " + event.oldVersion, null);
          break;
      }
    };
    request.onerror = function (event) {
      
      callback("Error opening database!", null);
    };
    request.onblocked = function (event) {
      callback("Opening database request is blocked.", null);
    };
  },

  








  newTxn: function newTxn(txn_type, callback) {
    this.ensureDB(function (error, db) {
      if (error) {
        if (DEBUG) debug("Could not open database: " + error);
        callback(error);
        return;
      }
      let txn = db.transaction([STORE_NAME], txn_type);
      if (DEBUG) debug("Started transaction " + txn + " of type " + txn_type);
      if (DEBUG) {
        txn.oncomplete = function oncomplete(event) {
          debug("Transaction " + txn + " completed.");
        };
        txn.onerror = function onerror(event) {
          
          
          debug("Error occurred during transaction: " + event.target.errorCode);
        };
      }
      if (DEBUG) debug("Retrieving object store", STORE_NAME);
      let store = txn.objectStore(STORE_NAME);
      callback(null, txn, store);
    });
  },

  





  createSchema: function createSchema(db) {
    let objectStore = db.createObjectStore(STORE_NAME, { keyPath: "id" });
    objectStore.createIndex("id", "id", { unique: true });
    objectStore.createIndex("delivery", "delivery", { unique: false });
    objectStore.createIndex("sender", "sender", { unique: false });
    objectStore.createIndex("receiver", "receiver", { unique: false });
    objectStore.createIndex("timestamp", "timestamp", { unique: false });
    objectStore.createIndex("read", "read", { unique: false });
    if (DEBUG) debug("Created object stores and indexes");
  },

  


  upgradeSchema: function upgradeSchema(objectStore) {
    
    objectStore.createIndex("read", "read", { unique: false });  
  },

  upgradeSchema2: function upgradeSchema2(objectStore) {
    if (!objectStore) {
      objectStore = aTransaction.objectStore(STORE_NAME);
    }
    objectStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        return;
      }

      let message = cursor.value;
      if (DEBUG) debug("upgrade before: " + JSON.stringify(message));
      if (!message.messageClass) {
        message.messageClass = MESSAGE_CLASS_NORMAL;
      }
      if (!message.deliveryStatus) {
        message.deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
      }
      cursor.update(message);
      if (DEBUG) debug("upgrade after:  " + JSON.stringify(message));
      cursor.continue();
    }
  },

  











  keyIntersection: function keyIntersection(keys, filter) {
    
    
    let result = keys[FILTER_TIMESTAMP];
    if (keys[FILTER_NUMBERS].length || filter.numbers) {
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

  











  onMessageListCreated: function onMessageListCreated(messageList, requestId) {
    if (DEBUG) debug("Message list created: " + messageList);
    let self = this;
    self.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        gSmsRequestManager.notifyReadMessageListFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
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
          gSmsRequestManager.notifyReadMessageListFailed(
            requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
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
        gSmsRequestManager.notifyCreateMessageList(requestId,
                                                   self.lastMessageListId,
                                                   sms);
      };
    });
  },

  saveMessage: function saveMessage(message) {
    this.lastKey += 1;
    message.id = this.lastKey;
    if (DEBUG) debug("Going to store " + JSON.stringify(message));
    this.newTxn(READ_WRITE, function(error, txn, store) {
      if (error) {
        return;
      }
      let request = store.put(message);
    });
    
    return message.id;
  },


  



  saveReceivedMessage: function saveReceivedMessage(sender, body, messageClass, date) {
    let receiver = this.mRIL.rilContext.icc ? this.mRIL.rilContext.icc.msisdn : null;

    let message = {delivery:       DELIVERY_RECEIVED,
                   deliveryStatus: DELIVERY_STATUS_SUCCESS,
                   sender:         sender,
                   receiver:       receiver,
                   body:           body,
                   messageClass:   messageClass,
                   timestamp:      date,
                   read:           FILTER_READ_UNREAD};
    return this.saveMessage(message);
  },

  saveSentMessage: function saveSentMessage(receiver, body, date) {
    let sender = this.mRIL.rilContext.icc ? this.mRIL.rilContext.icc.msisdn : null;

    let message = {delivery:       DELIVERY_SENT,
                   deliveryStatus: DELIVERY_STATUS_PENDING,
                   sender:         sender,
                   receiver:       receiver,
                   body:           body,
                   messageClass:   MESSAGE_CLASS_NORMAL,
                   timestamp:      date,
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

  getMessage: function getMessage(messageId, requestId) {
    if (DEBUG) debug("Retrieving message with ID " + messageId);
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug(error);
        gSmsRequestManager.notifyGetSmsFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
        return;
      }
      let request = store.mozGetAll(messageId);

      txn.oncomplete = function oncomplete() {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (request.result.length > 1) {
          if (DEBUG) debug("Got too many results for id " + messageId);
          gSmsRequestManager.notifyGetSmsFailed(
            requestId, Ci.nsISmsRequestManager.UNKNOWN_ERROR);
          return;
        }
        let data = request.result[0];
        if (!data) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          gSmsRequestManager.notifyGetSmsFailed(
            requestId, Ci.nsISmsRequestManager.NOT_FOUND_ERROR);
          return;
        }
        if (data.id != messageId) {
          if (DEBUG) {
            debug("Requested message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          gSmsRequestManager.notifyGetSmsFailed(
            requestId, Ci.nsISmsRequestManager.UNKNOWN_ERROR);
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
        gSmsRequestManager.notifyGotSms(requestId, message);
      };

      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction", event.target.errorCode);
        
        gSmsRequestManager.notifyGetSmsFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
      };
    });
  },

  deleteMessage: function deleteMessage(messageId, requestId) {
    let deleted = false;
    let self = this;
    this.newTxn(READ_WRITE, function (error, txn, store) {
      if (error) {
        gSmsRequestManager.notifySmsDeleteFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
        return;
      }
      let request = store.count(messageId);

      request.onsuccess = function onsuccess(event) {        
        let count = event.target.result;
        if (DEBUG) debug("Count for messageId " + messageId + ": " + count);
        deleted = (count == 1);
        if (deleted) {
          store.delete(messageId);
        }
      };

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        gSmsRequestManager.notifySmsDeleted(requestId, deleted);
      };

      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction", event.target.errorCode);
        
        gSmsRequestManager.notifySmsDeleteFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
      };
    });
  },

  createMessageList: function createMessageList(filter, reverse, requestId) {
    if (DEBUG) {
      debug("Creating a message list. Filters:" +
            " startDate: " + filter.startDate +
            " endDate: " + filter.endDate +
            " delivery: " + filter.delivery +
            " numbers: " + filter.numbers +
            " read: " + filter.read +
            " reverse: " + reverse);
    }
    
    
    
    
    let filteredKeys = {};
    filteredKeys[FILTER_TIMESTAMP] = [];
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
      gSmsRequestManager.notifyReadMessageListFailed(
        requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
      return;
    };

    let self = this;
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        errorCb(error);
        return;
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
      let direction = reverse ? PREV : NEXT;
      let timeRequest = store.index("timestamp").openKeyCursor(timeKeyRange,
                                                               direction);

      timeRequest.onsuccess = function onsuccess(event) {
        successCb(event.target.result, FILTER_TIMESTAMP);
      };
      timeRequest.onerror = errorCb;

      
      
      if (filter.delivery) {
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
          let numberKeyRange = IDBKeyRange.only(filter.numbers[i]);
          let senderRequest = store.index("sender")
                                   .openKeyCursor(numberKeyRange);
          let receiverRequest = store.index("receiver")
                                     .openKeyCursor(numberKeyRange);
          senderRequest.onsuccess = receiverRequest.onsuccess =
            function onsuccess(event){
              successCb(event.target.result, FILTER_NUMBERS);
            };
          senderRequest.onerror = receiverRequest.onerror = errorCb;
        }
      }

      
      
      if (filter.read != undefined) {
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

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        
        
        let result =  self.keyIntersection(filteredKeys, filter);
        if (!result.length) {
          if (DEBUG) debug("No messages matching the filter criteria");
          gSmsRequestManager.notifyNoMessageInList(requestId);
          return;
        }

        
        
        
        
        self.onMessageListCreated(result, requestId);
      };

      txn.onerror = function onerror(event) {
        errorCb(event);
      };
    });
  },

  getNextMessageInList: function getNextMessageInList(listId, requestId) {
    if (DEBUG) debug("Getting next message in list " + listId);
    let messageId;
    let list = this.messageLists[listId];
    if (!list) {
      if (DEBUG) debug("Wrong list id");
      gSmsRequestManager.notifyReadMessageListFailed(
        requestId, Ci.nsISmsRequestManager.NOT_FOUND_ERROR);
      return;
    }
    messageId = list.shift();
    if (messageId == null) {
      if (DEBUG) debug("Reached the end of the list!");
      gSmsRequestManager.notifyNoMessageInList(requestId);
      return;
    }
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (DEBUG) debug("Fetching message " + messageId);
      let request = store.get(messageId);
      let message;
      request.onsuccess = function onsuccess(event) {
        message = request.result;
      };

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (!message) {
          if (DEBUG) debug("Could not get message id " + messageId);
          gSmsRequestManager.notifyReadMessageListFailed(
            requestId, Ci.nsISmsRequestManager.NOT_FOUND_ERROR);
        }
        let sms = gSmsService.createSmsMessage(message.id,
                                               message.delivery,
                                               message.deliveryStatus,
                                               message.sender,
                                               message.receiver,
                                               message.body,
                                               message.messageClass,
                                               message.timestamp,
                                               message.read);
        gSmsRequestManager.notifyGotNextMessage(requestId, sms);
      };

      txn.onerror = function onerror(event) {
        
        if (DEBUG) {
          debug("Error retrieving message id: " + messageId +
                ". Error code: " + event.target.errorCode);
        }
        gSmsRequestManager.notifyReadMessageListFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
      };
    });
  },

  clearMessageList: function clearMessageList(listId) {
    if (DEBUG) debug("Clearing message list: " + listId);
    delete this.messageLists[listId];
  },

  markMessageRead: function markMessageRead(messageId, value, requestId) {
    if (DEBUG) debug("Setting message " + messageId + " read to " + value);
    this.newTxn(READ_WRITE, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug(error);
        gSmsRequestManager.notifyMarkMessageReadFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
        return;
      }
      let getRequest = store.get(messageId);

      getRequest.onsuccess = function onsuccess(event) {
        let message = event.target.result;
        if (DEBUG) debug("Message ID " + messageId + " not found");
        if (!message) {
          gSmsRequestManager.notifyMarkMessageReadFailed(
            requestId, Ci.nsISmsRequestManager.NOT_FOUND_ERROR);
          return;
        }
        if (message.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          gSmsRequestManager.notifyMarkMessageReadFailed(
            requestId, Ci.nsISmsRequestManager.UNKNOWN_ERROR);
          return;
        }
        
        
        if (message.read == value) {
          if (DEBUG) debug("The value of message.read is already " + value);
          gSmsRequestManager.notifyMarkedMessageRead(requestId, message.read);
          return;
        }
        message.read = value ? FILTER_READ_READ : FILTER_READ_UNREAD;
        if (DEBUG) debug("Message.read set to: " + value);
        let putRequest = store.put(message);
        putRequest.onsuccess = function onsuccess(event) {
          if (DEBUG) {
            debug("Update successfully completed. Message: " +
                  JSON.stringify(event.target.result));
          }
          let checkRequest = store.get(message.id);
          checkRequest.onsuccess = function onsuccess(event) {
            gSmsRequestManager.notifyMarkedMessageRead(
              requestId, event.target.result.read);
          };
        }
      };

      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.errorCode);
        gSmsRequestManager.notifyMarkMessageReadFailed(
          requestId, Ci.nsISmsRequestManager.INTERNAL_ERROR);
      };
    });
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
