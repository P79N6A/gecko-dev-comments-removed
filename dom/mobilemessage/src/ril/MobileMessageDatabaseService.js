



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");

const RIL_MOBILEMESSAGEDATABASESERVICE_CONTRACTID = "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1";
const RIL_MOBILEMESSAGEDATABASESERVICE_CID = Components.ID("{29785f90-6b5b-11e2-9201-3b280170b2ec}");

const DEBUG = false;
const DB_NAME = "sms";
const DB_VERSION = 7;
const STORE_NAME = "sms";
const MOST_RECENT_STORE_NAME = "most-recent";

const DELIVERY_SENDING = "sending";
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

function getNumberFromRecord(aRecord) {
  return aRecord.delivery == DELIVERY_RECEIVED ? aRecord.sender : aRecord.receiver;
}




function MobileMessageDatabaseService() {
  
  
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
          debug("Could not get the last key from mobile message database. " +
                "Probably empty database");
        }
        return;
      }
      that.lastKey = cursor.key || 0;
      if (DEBUG) debug("Last assigned message ID was " + that.lastKey);
    };
    request.onerror = function onerror(event) {
      if (DEBUG) {
        debug("Could not get the last key from mobile message database " +
              event.target.errorCode);
      }
    };
  });

  this.messageLists = {};
}
MobileMessageDatabaseService.prototype = {

  classID: RIL_MOBILEMESSAGEDATABASESERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRilMobileMessageDatabaseService,
                                         Ci.nsIMobileMessageDatabaseService,
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
            if (DEBUG) debug("Upgrade to version 6. Use PhonenumberJS.");
            self.upgradeSchema5(event.target.transaction);
            break;
          case 6:
            if (DEBUG) debug("Upgrade to version 7. Use multiple entry indexes.");
            self.upgradeSchema6(event.target.transaction);
            break;
          default:
            event.target.transaction.abort();
            callback("Old database version: " + event.oldVersion, null);
            break;
        }
        currentVersion++;
      }
    };
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

      let record = cursor.value;
      record.messageClass = MESSAGE_CLASS_NORMAL;
      record.deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
      cursor.update(record);
      cursor.continue();
    };
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
    let mobileMessageStore = transaction.objectStore(STORE_NAME);
    let mostRecentStore = transaction.objectStore(MOST_RECENT_STORE_NAME);

    mobileMessageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        for (let thread in threads) {
          mostRecentStore.put(threads[thread]);
        }
        return;
      }

      let record = cursor.value;
      let contact = record.sender || record.receiver;

      if (contact in threads) {
        let thread = threads[contact];
        if (!record.read) {
          thread.unreadCount++;
        }
        if (record.timestamp > thread.timestamp) {
          thread.id = record.id;
          thread.body = record.body;
          thread.timestamp = record.timestamp;
        }
      } else {
        threads[contact] = {
          senderOrReceiver: contact,
          id: record.id,
          timestamp: record.timestamp,
          body: record.body,
          unreadCount: record.read ? 0 : 1
        };
      }
      cursor.continue();
    };
  },

  upgradeSchema5: function upgradeSchema5(transaction) {
    
  },

  upgradeSchema6: function upgradeSchema6(transaction) {
    let objectStore = transaction.objectStore(STORE_NAME);

    
    if (objectStore.indexNames.contains("delivery")) {
      objectStore.deleteIndex("delivery");
    }
    
    if (objectStore.indexNames.contains("sender")) {
      objectStore.deleteIndex("sender");
    }
    
    if (objectStore.indexNames.contains("receiver")) {
      objectStore.deleteIndex("receiver");
    }
    
    if (objectStore.indexNames.contains("read")) {
      objectStore.deleteIndex("read");
    }

    
    objectStore.createIndex("delivery", "deliveryIndex");
    objectStore.createIndex("number", "numberIndex", { multiEntry: true });
    objectStore.createIndex("read", "readIndex");

    
    objectStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        return;
      }

      let record = cursor.value;
      let timestamp = record.timestamp;
      record.deliveryIndex = [record.delivery, timestamp];
      record.numberIndex = [
        [record.sender, timestamp],
        [record.receiver, timestamp]
      ];
      record.readIndex = [record.read, timestamp];
      cursor.update(record);
      cursor.continue();
    };
  },

  createSmsMessageFromRecord: function createSmsMessageFromRecord(aRecord) {
    if (DEBUG) debug("createSmsMessageFromRecord: " + JSON.stringify(aRecord));
    return gSmsService.createSmsMessage(aRecord.id,
                                        aRecord.delivery,
                                        aRecord.deliveryStatus,
                                        aRecord.sender,
                                        aRecord.receiver,
                                        aRecord.body,
                                        aRecord.messageClass,
                                        aRecord.timestamp,
                                        aRecord.read);
  },

  



  onNextMessageInListGot: function onNextMessageInListGot(
      aObjectStore, aMessageList, aMessageId) {

    if (DEBUG) {
      debug("onNextMessageInListGot - listId: "
            + aMessageList.listId + ", messageId: " + aMessageId);
    }
    if (aMessageId) {
      
      aMessageList.results.push(aMessageId);
    }
    if (aMessageId <= 0) {
      
      aMessageList.processing = false;
    }

    if (!aMessageList.requestWaiting) {
      if (DEBUG) debug("Cursor.continue() not called yet");
      return;
    }

    
    
    
    
    let smsRequest = aMessageList.requestWaiting;
    aMessageList.requestWaiting = null;

    if (!aMessageList.results.length) {
      
      if (!aMessageList.processing) {
        if (DEBUG) debug("No messages matching the filter criteria");
        smsRequest.notifyNoMessageInList();
      }
      
      return;
    }

    if (aMessageList.results[0] < 0) {
      
      
      if (DEBUG) debug("An previous error found");
      smsRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      return;
    }

    let firstMessageId = aMessageList.results.shift();
    if (DEBUG) debug ("Fetching message " + firstMessageId);

    let getRequest = aObjectStore.get(firstMessageId);
    let self = this;
    getRequest.onsuccess = function onsuccess(event) {
      let sms = self.createSmsMessageFromRecord(event.target.result);
      if (aMessageList.listId >= 0) {
        if (DEBUG) {
          debug("notifyNextMessageInListGot - listId: "
                + aMessageList.listId + ", messageId: " + firstMessageId);
        }
        smsRequest.notifyNextMessageInListGot(sms);
      } else {
        self.lastMessageListId += 1;
        aMessageList.listId = self.lastMessageListId;
        self.messageLists[self.lastMessageListId] = aMessageList;
        if (DEBUG) {
          debug("notifyMessageListCreated - listId: "
                + aMessageList.listId + ", messageId: " + firstMessageId);
        }
        smsRequest.notifyMessageListCreated(aMessageList.listId, sms);
      }
    };
    getRequest.onerror = function onerror(event) {
      if (DEBUG) {
        debug("notifyReadMessageListFailed - listId: "
              + aMessageList.listId + ", messageId: " + firstMessageId);
      }
      smsRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
    };
  },

  




  onNextMessageInMultiFiltersGot: function onNextMessageInMultiFiltersGot(
      aObjectStore, aMessageList, aContextIndex, aMessageId, aTimestamp) {

    if (DEBUG) {
      debug("onNextMessageInMultiFiltersGot: "
            + aContextIndex + ", " + aMessageId + ", " + aTimestamp);
    }
    let contexts = aMessageList.contexts;

    if (!aMessageId) {
      contexts[aContextIndex].processing = false;
      for (let i = 0; i < contexts.length; i++) {
        if (contexts[i].processing) {
          return false;
        }
      }

      this.onNextMessageInListGot(aObjectStore, aMessageList, 0);
      return false;
    }

    
    
    
    
    
    for (let i = 0; i < contexts.length; i++) {
      if (i == aContextIndex) {
        continue;
      }

      let ctx = contexts[i];
      let results = ctx.results;
      let found = false;
      for (let j = 0; j < results.length; j++) {
        let result = results[j];
        if (result.id == aMessageId) {
          found = true;
          break;
        }
        if ((!aMessageList.reverse && (result.timestamp > aTimestamp)) ||
            (aMessageList.reverse && (result.timestamp < aTimestamp))) {
          
          return true;
        }
      }

      if (!found) {
        if (!ctx.processing) {
          
          if (results.length) {
            let lastResult = results[results.length - 1];
            if ((!aMessageList.reverse && (lastResult.timestamp >= aTimestamp)) ||
                (aMessageList.reverse && (lastResult.timestamp <= aTimestamp))) {
              
              return true;
            }
          }

          
          
          return this.onNextMessageInMultiFiltersGot(aObjectStore, aMessageList,
                                                     aContextIndex, 0, 0);
        }

        
        contexts[aContextIndex].results.push({
          id: aMessageId,
          timestamp: aTimestamp
        });
        return true;
      }
    }

    
    this.onNextMessageInListGot(aObjectStore, aMessageList, aMessageId);
    return true;
  },

  onNextMessageInMultiNumbersGot: function onNextMessageInMultiNumbersGot(
      aObjectStore, aMessageList, aContextIndex,
      aQueueIndex, aMessageId, aTimestamp) {

    if (DEBUG) {
      debug("onNextMessageInMultiNumbersGot: "
            + aQueueIndex + ", " + aMessageId + ", " + aTimestamp);
    }
    let queues = aMessageList.numberQueues;
    let q = queues[aQueueIndex];
    if (aMessageId) {
      if (!aQueueIndex) {
        
        q.results.push({
          id: aMessageId,
          timestamp: aTimestamp
        });
      } else {
        
        q.results.push(aMessageId);
      }
      return true;
    }

    q.processing -= 1;
    if (queues[0].processing || queues[1].processing) {
      
      
      
      return false;
    }

    let tres = queues[0].results;
    let qres = queues[1].results;
    tres = tres.filter(function (element) {
      return qres.indexOf(element.id) != -1;
    });
    if (aContextIndex < 0) {
      for (let i = 0; i < tres.length; i++) {
        this.onNextMessageInListGot(aObjectStore, aMessageList, tres[i].id);
      }
      this.onNextMessageInListGot(aObjectStore, aMessageList, 0);
    } else {
      for (let i = 0; i < tres.length; i++) {
        this.onNextMessageInMultiFiltersGot(aObjectStore, aMessageList,
                                            aContextIndex,
                                            tres[i].id, tres[i].timestamp);
      }
      this.onNextMessageInMultiFiltersGot(aObjectStore, aMessageList,
                                          aContextIndex, 0, 0);
    }
    return false;
  },

  saveRecord: function saveRecord(aRecord, aCallback) {
    this.lastKey += 1;
    aRecord.id = this.lastKey;
    if (DEBUG) debug("Going to store " + JSON.stringify(aRecord));

    let self = this;
    function notifyResult(rv) {
      if (!aCallback) {
        return;
      }
      aCallback.notify(rv, aRecord);
    }

    this.newTxn(READ_WRITE, function(error, txn, stores) {
      if (error) {
        
        notifyResult(Cr.NS_ERROR_FAILURE);
        return;
      }
      txn.oncomplete = function oncomplete(event) {
        notifyResult(Cr.NS_OK);
      };
      txn.onabort = function onabort(event) {
        
        notifyResult(Cr.NS_ERROR_FAILURE);
      };

      
      stores[0].put(aRecord);

      
      let number = getNumberFromRecord(aRecord);
      stores[1].get(number).onsuccess = function onsuccess(event) {
        let mostRecentEntry = event.target.result;
        if (mostRecentEntry) {
          let needsUpdate = false;

          if (mostRecentEntry.timestamp <= aRecord.timestamp) {
            mostRecentEntry.timestamp = aRecord.timestamp;
            mostRecentEntry.body = aRecord.body;
            needsUpdate = true;
          }

          if (!aRecord.read) {
            mostRecentEntry.unreadCount++;
            needsUpdate = true;
          }

          if (needsUpdate) {
            event.target.source.put(mostRecentEntry);
          }
        } else {
          event.target.source.add({ senderOrReceiver: number,
                                    timestamp: aRecord.timestamp,
                                    body: aRecord.body,
                                    id: aRecord.id,
                                    unreadCount: aRecord.read ? 0 : 1 });
        }
      };
    }, [STORE_NAME, MOST_RECENT_STORE_NAME]);
    
    return aRecord.id;
  },

  getRilIccInfoMsisdn: function getRilIccInfoMsisdn() {
    let iccInfo = this.mRIL.rilContext.iccInfo;
    let number = iccInfo ? iccInfo.msisdn : null;

    
    
    if (number === undefined || number === "undefined") {
      return null;
    }
    return number;
  },

  makePhoneNumberInternational: function makePhoneNumberInternational(aNumber) {
    if (!aNumber) {
      return aNumber;
    }
    let parsedNumber = PhoneNumberUtils.parse(aNumber.toString());
    if (!parsedNumber || !parsedNumber.internationalNumber) {
      return aNumber;
    }
    return parsedNumber.internationalNumber;
  },

  



  saveReceivedMessage: function saveReceivedMessage(aMessage, aCallback) {
    if (aMessage.type === undefined ||
        aMessage.sender === undefined ||
        aMessage.messageClass === undefined ||
        aMessage.timestamp === undefined) {
      if (aCallback) {
        aCallback.notify(Cr.NS_ERROR_FAILURE, null);
      }
      return;
    }

    let receiver = this.getRilIccInfoMsisdn();
    receiver = this.makePhoneNumberInternational(receiver);

    let sender = aMessage.sender =
      this.makePhoneNumberInternational(aMessage.sender);

    let timestamp = aMessage.timestamp;

    
    aMessage.deliveryIndex = [DELIVERY_RECEIVED, timestamp];
    aMessage.numberIndex = [[sender, timestamp], [receiver, timestamp]];
    aMessage.readIndex = [FILTER_READ_UNREAD, timestamp];
    aMessage.delivery = DELIVERY_RECEIVED;
    aMessage.deliveryStatus = DELIVERY_STATUS_SUCCESS;
    aMessage.receiver = receiver;
    aMessage.read = FILTER_READ_UNREAD;

    return this.saveRecord(aMessage, aCallback);
  },

  saveSendingMessage: function saveSendingMessage(aMessage, aCallback) {
    if (aMessage.type === undefined ||
        aMessage.receiver === undefined ||
        aMessage.deliveryStatus === undefined ||
        aMessage.timestamp === undefined) {
      if (aCallback) {
        aCallback.notify(Cr.NS_ERROR_FAILURE, null);
      }
      return;
    }

    let sender = this.getRilIccInfoMsisdn();
    let receiver = aMessage.receiver;

    let rilContext = this.mRIL.rilContext;
    if (rilContext.voice.network.mcc === rilContext.iccInfo.mcc) {
      receiver = aMessage.receiver = this.makePhoneNumberInternational(receiver);
      sender = this.makePhoneNumberInternational(sender);
    }

    let timestamp = aMessage.timestamp;

    
    aMessage.deliveryIndex = [DELIVERY_SENDING, timestamp];
    aMessage.numberIndex = [[sender, timestamp], [receiver, timestamp]];
    aMessage.readIndex = [FILTER_READ_READ, timestamp];
    aMessage.delivery = DELIVERY_SENDING;
    aMessage.sender = sender;
    aMessage.messageClass = MESSAGE_CLASS_NORMAL;
    aMessage.read = FILTER_READ_READ;

    return this.saveRecord(aMessage, aCallback);
  },

  setMessageDelivery: function setMessageDelivery(
      messageId, delivery, deliveryStatus, callback) {
    if (DEBUG) {
      debug("Setting message " + messageId + " delivery to " + delivery
            + ", and deliveryStatus to " + deliveryStatus);
    }

    let self = this;
    let record;
    function notifyResult(rv) {
      if (!callback) {
        return;
      }
      callback.notify(rv, record);
    }

    this.newTxn(READ_WRITE, function (error, txn, store) {
      if (error) {
        
        notifyResult(Cr.NS_ERROR_FAILURE);
        return;
      }
      txn.oncomplete = function oncomplete(event) {
        notifyResult(Cr.NS_OK);
      };
      txn.onabort = function onabort(event) {
        
        notifyResult(Cr.NS_ERROR_FAILURE);
      };

      let getRequest = store.get(messageId);
      getRequest.onsuccess = function onsuccess(event) {
        record = event.target.result;
        if (!record) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          return;
        }
        if (record.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          return;
        }
        
        if ((record.delivery == delivery)
            && (record.deliveryStatus == deliveryStatus)) {
          if (DEBUG) {
            debug("The values of attribute delivery and deliveryStatus are the"
                  + " the same with given parameters.");
          }
          return;
        }
        record.delivery = delivery;
        record.deliveryIndex = [delivery, record.timestamp];
        record.deliveryStatus = deliveryStatus;
        if (DEBUG) {
          debug("Message.delivery set to: " + delivery
                + ", and Message.deliveryStatus set to: " + deliveryStatus);
        }
        store.put(record);
      };
    });
  },

  



  getMessage: function getMessage(messageId, aRequest) {
    if (DEBUG) debug("Retrieving message with ID " + messageId);
    let self = this;
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      let request = store.mozGetAll(messageId);

      txn.oncomplete = function oncomplete() {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (request.result.length > 1) {
          if (DEBUG) debug("Got too many results for id " + messageId);
          aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        let record = request.result[0];
        if (!record) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
          return;
        }
        if (record.id != messageId) {
          if (DEBUG) {
            debug("Requested message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        let sms = self.createSmsMessageFromRecord(record);
        aRequest.notifyMessageGot(sms);
      };

      txn.onerror = function onerror(event) {
        if (DEBUG) {
          if (event.target)
            debug("Caught error on transaction", event.target.errorCode);
        }
        
        aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      };
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

      const mobileMessageStore = stores[0];
      const mruStore = stores[1];

      let deleted = false;

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        aRequest.notifyMessageDeleted(deleted);
      };

      mobileMessageStore.get(messageId).onsuccess = function(event) {
        let record = event.target.result;
        if (record) {
          if (DEBUG) debug("Deleting message id " + messageId);

          
          event.target.source.delete(messageId).onsuccess = function(event) {
            deleted = true;

            
            let number = getNumberFromRecord(record);

            mruStore.get(number).onsuccess = function(event) {
              
              let mostRecentEntry = event.target.result;

              if (!record.read) {
                mostRecentEntry.unreadCount--;
              }

              if (mostRecentEntry.id == messageId) {
                
                let numberRange = IDBKeyRange.bound([number, 0], [number, ""]);
                let numberRequest = mobileMessageStore.index("number")
                                                      .openCursor(numberRange, PREV);
                numberRequest.onsuccess = function(event) {
                  let cursor = event.target.result;
                  if (!cursor) {
                    if (DEBUG) {
                      debug("Deleting mru entry for number '" + number + "'");
                    }
                    mruStore.delete(number);
                    return;
                  }

                  let nextMsg = cursor.value;
                  mostRecentEntry.id = nextMsg.id;
                  mostRecentEntry.timestamp = nextMsg.timestamp;
                  mostRecentEntry.body = nextMsg.body;
                  if (DEBUG) {
                    debug("Updating mru entry: " +
                          JSON.stringify(mostRecentEntry));
                  }
                  mruStore.put(mostRecentEntry);
                };
              } else if (!record.read) {
                
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

    let self = this;
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (error) {
        
        if (DEBUG) debug("IDBRequest error " + error.target.errorCode);
        aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }

      let messageList = {
        listId: -1,
        reverse: reverse,
        processing: true,
        stop: false,
        
        contexts: null,
        
        numberQueues: null,
        
        requestWaiting: aRequest,
        results: []
      };

      let onNextMessageInListGotCb =
        self.onNextMessageInListGot.bind(self, store, messageList);

      let singleFilterSuccessCb = function onsfsuccess(event) {
        if (messageList.stop) {
          return;
        }

        let cursor = event.target.result;
        
        
        if (cursor) {
          onNextMessageInListGotCb(cursor.primaryKey);
          cursor.continue();
        } else {
          onNextMessageInListGotCb(0);
        }
      };

      let singleFilterErrorCb = function onsferror(event) {
        if (messageList.stop) {
          return;
        }

        if (DEBUG) debug("IDBRequest error " + event.target.errorCode);
        onNextMessageInListGotCb(-1);
      };

      let direction = reverse ? PREV : NEXT;

      
      
      if (filter.delivery || filter.numbers || filter.read != undefined) {
        let multiFiltersGotCb = self.onNextMessageInMultiFiltersGot
                                    .bind(self, store, messageList);

        let multiFiltersSuccessCb = function onmfsuccess(contextIndex, event) {
          if (messageList.stop) {
            return;
          }

          let cursor = event.target.result;
          if (cursor) {
            if (multiFiltersGotCb(contextIndex,
                                  cursor.primaryKey, cursor.key[1])) {
              cursor.continue();
            }
          } else {
            multiFiltersGotCb(contextIndex, 0, 0);
          }
        };

        let multiFiltersErrorCb = function onmferror(contextIndex, event) {
          if (messageList.stop) {
            return;
          }

          
          multiFiltersGotCb(contextIndex, 0, 0);
        };

        
        
        let startDate = 0, endDate = "";
        if (filter.startDate != null) {
          startDate = filter.startDate.getTime();
        }
        if (filter.endDate != null) {
          endDate = filter.endDate.getTime();
        }

        let singleFilter;
        {
          let numberOfContexts = 0;
          if (filter.delivery) numberOfContexts++;
          if (filter.numbers) numberOfContexts++;
          if (filter.read != undefined) numberOfContexts++;
          singleFilter = numberOfContexts == 1;
        }

        if (!singleFilter) {
          messageList.contexts = [];
        }

        let numberOfContexts = 0;

        let createRangedRequest = function crr(indexName, key) {
          let range = IDBKeyRange.bound([key, startDate], [key, endDate]);
          return store.index(indexName).openKeyCursor(range, direction);
        };

        let createSimpleRangedRequest = function csrr(indexName, key) {
          let request = createRangedRequest(indexName, key);
          if (singleFilter) {
            request.onsuccess = singleFilterSuccessCb;
            request.onerror = singleFilterErrorCb;
          } else {
            let contextIndex = numberOfContexts++;
            messageList.contexts.push({
              processing: true,
              results: []
            });
            request.onsuccess = multiFiltersSuccessCb.bind(null, contextIndex);
            request.onerror = multiFiltersErrorCb.bind(null, contextIndex);
          }
        };

        
        
        if (filter.delivery) {
          if (DEBUG) debug("filter.delivery " + filter.delivery);
          createSimpleRangedRequest("delivery", filter.delivery);
        }

        
        
        if (filter.numbers) {
          if (DEBUG) debug("filter.numbers " + filter.numbers.join(", "));
          let multiNumbers = filter.numbers.length > 1;
          if (!multiNumbers) {
            createSimpleRangedRequest("number", filter.numbers[0]);
          } else {
            let contextIndex = -1;
            if (!singleFilter) {
              contextIndex = numberOfContexts++;
              messageList.contexts.push({
                processing: true,
                results: []
              });
            }

            let multiNumbersGotCb =
              self.onNextMessageInMultiNumbersGot
                  .bind(self, store, messageList, contextIndex);

            let multiNumbersSuccessCb = function onmnsuccess(queueIndex, event) {
              if (messageList.stop) {
                return;
              }

              let cursor = event.target.result;
              if (cursor) {
                
                
                let key = queueIndex ? cursor.key[1] : cursor.key;
                if (multiNumbersGotCb(queueIndex, cursor.primaryKey, key)) {
                  cursor.continue();
                }
              } else {
                multiNumbersGotCb(queueIndex, 0, 0);
              }
            };

            let multiNumbersErrorCb = function onmnerror(queueIndex, event) {
              if (messageList.stop) {
                return;
              }

              
              multiNumbersGotCb(queueIndex, 0, 0);
            };

            messageList.numberQueues = [{
              
              processing: 1,
              results: []
            }, {
              
              processing: filter.numbers.length,
              results: []
            }];

            let timeRange = null;
            if (filter.startDate != null && filter.endDate != null) {
              timeRange = IDBKeyRange.bound(filter.startDate.getTime(),
                                            filter.endDate.getTime());
            } else if (filter.startDate != null) {
              timeRange = IDBKeyRange.lowerBound(filter.startDate.getTime());
            } else if (filter.endDate != null) {
              timeRange = IDBKeyRange.upperBound(filter.endDate.getTime());
            }

            let timeRequest = store.index("timestamp")
                                   .openKeyCursor(timeRange, direction);
            timeRequest.onsuccess = multiNumbersSuccessCb.bind(null, 0);
            timeRequest.onerror = multiNumbersErrorCb.bind(null, 0);

            for (let i = 0; i < filter.numbers.length; i++) {
              let request = createRangedRequest("number", filter.numbers[i]);
              request.onsuccess = multiNumbersSuccessCb.bind(null, 1);
              request.onerror = multiNumbersErrorCb.bind(null, 1);
            }
          }
        }

        
        
        if (filter.read != undefined) {
          let read = filter.read ? FILTER_READ_READ : FILTER_READ_UNREAD;
          if (DEBUG) debug("filter.read " + read);
          createSimpleRangedRequest("read", read);
        }
      } else {
        
        if (DEBUG) {
          debug("filter.timestamp " + filter.startDate + ", " + filter.endDate);
        }

        let range = null;
        if (filter.startDate != null && filter.endDate != null) {
          range = IDBKeyRange.bound(filter.startDate.getTime(),
                                    filter.endDate.getTime());
        } else if (filter.startDate != null) {
          range = IDBKeyRange.lowerBound(filter.startDate.getTime());
        } else if (filter.endDate != null) {
          range = IDBKeyRange.upperBound(filter.endDate.getTime());
        }

        let request = store.index("timestamp").openKeyCursor(range, direction);
        request.onsuccess = singleFilterSuccessCb;
        request.onerror = singleFilterErrorCb;
      }

      if (DEBUG) {
        txn.oncomplete = function oncomplete(event) {
          debug("Transaction " + txn + " completed.");
        };
      }

      txn.onerror = singleFilterErrorCb;
    });
  },

  getNextMessageInList: function getNextMessageInList(listId, aRequest) {
    if (DEBUG) debug("Getting next message in list " + listId);
    let messageId;
    let list = this.messageLists[listId];
    if (!list) {
      if (DEBUG) debug("Wrong list id");
      aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
      return;
    }
    if (list.processing) {
      
      
      if (list.requestWaiting) {
        if (DEBUG) debug("Already waiting for another request!");
        return;
      }
      list.requestWaiting = aRequest;
      return;
    }
    if (!list.results.length) {
      if (DEBUG) debug("Reached the end of the list!");
      aRequest.notifyNoMessageInList();
      return;
    }
    if (list.results[0] < 0) {
      aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      return;
    }
    messageId = list.results.shift();
    let self = this;
    this.newTxn(READ_ONLY, function (error, txn, store) {
      if (DEBUG) debug("Fetching message " + messageId);
      let request = store.get(messageId);
      let record;
      request.onsuccess = function onsuccess(event) {
        record = request.result;
      };

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (!record) {
          if (DEBUG) debug("Could not get message id " + messageId);
          aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
        }
        let sms = self.createSmsMessageFromRecord(record);
        aRequest.notifyNextMessageInListGot(sms);
      };

      txn.onerror = function onerror(event) {
        
        if (DEBUG) {
          debug("Error retrieving message id: " + messageId +
                ". Error code: " + event.target.errorCode);
        }
        aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      };
    });
  },

  clearMessageList: function clearMessageList(listId) {
    if (DEBUG) debug("Clearing message list: " + listId);
    if (this.messageLists[listId]) {
      this.messageLists[listId].stop = true;
      delete this.messageLists[listId];
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
        let record = event.target.result;
        if (!record) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
          return;
        }
        if (record.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        
        
        if (record.read == value) {
          if (DEBUG) debug("The value of record.read is already " + value);
          aRequest.notifyMessageMarkedRead(record.read);
          return;
        }
        record.read = value ? FILTER_READ_READ : FILTER_READ_UNREAD;
        record.readIndex = [record.read, record.timestamp];
        if (DEBUG) debug("Message.read set to: " + value);
        event.target.source.put(record).onsuccess = function onsuccess(event) {
          if (DEBUG) {
            debug("Update successfully completed. Message: " +
                  JSON.stringify(event.target.result));
          }

          
          let number = getNumberFromRecord(record);

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
              aRequest.notifyMessageMarkedRead(record.read);
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

XPCOMUtils.defineLazyServiceGetter(MobileMessageDatabaseService.prototype, "mRIL",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MobileMessageDatabaseService]);

function debug() {
  dump("MobileMessageDatabaseService: " + Array.slice(arguments).join(" ") + "\n");
}
