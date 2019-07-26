



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
const MESSAGE_STORE_NAME = "sms";
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

function getNumberFromRecord(aMessageRecord) {
  return aMessageRecord.delivery == DELIVERY_RECEIVED ? aMessageRecord.sender
                                                      : aMessageRecord.receiver;
}




function MobileMessageDatabaseService() {
  
  
  Services.dirsvc.get("ProfD", Ci.nsIFile);

  gIDBManager.initWindowless(GLOBAL_SCOPE);

  let that = this;
  this.newTxn(READ_ONLY, function(error, txn, messageStore){
    if (error) {
      return;
    }
    
    
    let request = messageStore.openCursor(null, PREV);
    request.onsuccess = function onsuccess(event) {
      let cursor = event.target.result;
      if (!cursor) {
        if (DEBUG) {
          debug("Could not get the last key from mobile message database. " +
                "Probably empty database");
        }
        return;
      }
      that.lastMessageId = cursor.key || 0;
      if (DEBUG) debug("Last assigned message ID was " + that.lastMessageId);
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

  


  lastMessageId: 0,

  


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
            self.upgradeSchema(event.target.transaction);
            break;
          case 2:
            if (DEBUG) debug("Upgrade to version 3. Fix existing entries.");
            self.upgradeSchema2(event.target.transaction);
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

  










  newTxn: function newTxn(txn_type, callback, storeNames) {
    if (!storeNames) {
      storeNames = [MESSAGE_STORE_NAME];
    }
    if (DEBUG) debug("Opening transaction for object stores: " + storeNames);
    this.ensureDB(function (error, db) {
      if (error) {
        if (DEBUG) debug("Could not open database: " + error);
        callback(error);
        return;
      }
      let txn = db.transaction(storeNames, txn_type);
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
      if (storeNames.length == 1) {
        if (DEBUG) debug("Retrieving object store " + storeNames[0]);
        stores = txn.objectStore(storeNames[0]);
      } else {
        stores = [];
        for each (let storeName in storeNames) {
          if (DEBUG) debug("Retrieving object store " + storeName);
          stores.push(txn.objectStore(storeName));
        }
      }
      callback(null, txn, stores);
    });
  },

  





  createSchema: function createSchema(db) {
    
    let messageStore = db.createObjectStore(MESSAGE_STORE_NAME, { keyPath: "id" });
    messageStore.createIndex("timestamp", "timestamp", { unique: false });
    if (DEBUG) debug("Created object stores and indexes");
  },

  


  upgradeSchema: function upgradeSchema(transaction) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.createIndex("read", "read", { unique: false });
  },

  upgradeSchema2: function upgradeSchema2(transaction) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        return;
      }

      let messageRecord = cursor.value;
      messageRecord.messageClass = MESSAGE_CLASS_NORMAL;
      messageRecord.deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  upgradeSchema3: function upgradeSchema3(db, transaction) {
    
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    if (messageStore.indexNames.contains("id")) {
      messageStore.deleteIndex("id");
    }

    










    let mostRecentStore = db.createObjectStore(MOST_RECENT_STORE_NAME,
                                               { keyPath: "senderOrReceiver" });
    mostRecentStore.createIndex("timestamp", "timestamp");
  },

  upgradeSchema4: function upgradeSchema4(transaction) {
    let threads = {};
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    let mostRecentStore = transaction.objectStore(MOST_RECENT_STORE_NAME);

    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        for (let thread in threads) {
          mostRecentStore.put(threads[thread]);
        }
        return;
      }

      let messageRecord = cursor.value;
      let contact = messageRecord.sender || messageRecord.receiver;

      if (contact in threads) {
        let thread = threads[contact];
        if (!messageRecord.read) {
          thread.unreadCount++;
        }
        if (messageRecord.timestamp > thread.timestamp) {
          thread.id = messageRecord.id;
          thread.body = messageRecord.body;
          thread.timestamp = messageRecord.timestamp;
        }
      } else {
        threads[contact] = {
          senderOrReceiver: contact,
          id: messageRecord.id,
          timestamp: messageRecord.timestamp,
          body: messageRecord.body,
          unreadCount: messageRecord.read ? 0 : 1
        };
      }
      cursor.continue();
    };
  },

  upgradeSchema5: function upgradeSchema5(transaction) {
    
  },

  upgradeSchema6: function upgradeSchema6(transaction) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    
    if (messageStore.indexNames.contains("delivery")) {
      messageStore.deleteIndex("delivery");
    }
    
    if (messageStore.indexNames.contains("sender")) {
      messageStore.deleteIndex("sender");
    }
    
    if (messageStore.indexNames.contains("receiver")) {
      messageStore.deleteIndex("receiver");
    }
    
    if (messageStore.indexNames.contains("read")) {
      messageStore.deleteIndex("read");
    }

    
    messageStore.createIndex("delivery", "deliveryIndex");
    messageStore.createIndex("number", "numberIndex", { multiEntry: true });
    messageStore.createIndex("read", "readIndex");

    
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        return;
      }

      let messageRecord = cursor.value;
      let timestamp = messageRecord.timestamp;
      messageRecord.deliveryIndex = [messageRecord.delivery, timestamp];
      messageRecord.numberIndex = [
        [messageRecord.sender, timestamp],
        [messageRecord.receiver, timestamp]
      ];
      messageRecord.readIndex = [messageRecord.read, timestamp];
      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  createSmsMessageFromRecord: function createSmsMessageFromRecord(aMessageRecord) {
    if (DEBUG) {
      debug("createSmsMessageFromRecord: " + JSON.stringify(aMessageRecord));
    }
    return gSmsService.createSmsMessage(aMessageRecord.id,
                                        aMessageRecord.delivery,
                                        aMessageRecord.deliveryStatus,
                                        aMessageRecord.sender,
                                        aMessageRecord.receiver,
                                        aMessageRecord.body,
                                        aMessageRecord.messageClass,
                                        aMessageRecord.timestamp,
                                        aMessageRecord.read);
  },

  



  onNextMessageInListGot: function onNextMessageInListGot(
      aMessageStore, aMessageList, aMessageId) {

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

    let getRequest = aMessageStore.get(firstMessageId);
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
      aMessageStore, aMessageList, aContextIndex, aMessageId, aTimestamp) {

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

      this.onNextMessageInListGot(aMessageStore, aMessageList, 0);
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

          
          
          return this.onNextMessageInMultiFiltersGot(aMessageStore, aMessageList,
                                                     aContextIndex, 0, 0);
        }

        
        contexts[aContextIndex].results.push({
          id: aMessageId,
          timestamp: aTimestamp
        });
        return true;
      }
    }

    
    this.onNextMessageInListGot(aMessageStore, aMessageList, aMessageId);
    return true;
  },

  onNextMessageInMultiNumbersGot: function onNextMessageInMultiNumbersGot(
      aMessageStore, aMessageList, aContextIndex,
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
        this.onNextMessageInListGot(aMessageStore, aMessageList, tres[i].id);
      }
      this.onNextMessageInListGot(aMessageStore, aMessageList, 0);
    } else {
      for (let i = 0; i < tres.length; i++) {
        this.onNextMessageInMultiFiltersGot(aMessageStore, aMessageList,
                                            aContextIndex,
                                            tres[i].id, tres[i].timestamp);
      }
      this.onNextMessageInMultiFiltersGot(aMessageStore, aMessageList,
                                          aContextIndex, 0, 0);
    }
    return false;
  },

  saveRecord: function saveRecord(aMessageRecord, aCallback) {
    this.lastMessageId += 1;
    aMessageRecord.id = this.lastMessageId;
    if (DEBUG) debug("Going to store " + JSON.stringify(aMessageRecord));

    let self = this;
    function notifyResult(rv) {
      if (!aCallback) {
        return;
      }
      aCallback.notify(rv, aMessageRecord);
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

      let messageStore = stores[0];
      let mostRecentStore = stores[1];

      
      messageStore.put(aMessageRecord);

      
      let number = getNumberFromRecord(aMessageRecord);
      mostRecentStore.get(number).onsuccess = function onsuccess(event) {
        let mostRecentRecord = event.target.result;
        if (mostRecentRecord) {
          let needsUpdate = false;

          if (mostRecentRecord.timestamp <= aMessageRecord.timestamp) {
            mostRecentRecord.timestamp = aMessageRecord.timestamp;
            mostRecentRecord.body = aMessageRecord.body;
            needsUpdate = true;
          }

          if (!aMessageRecord.read) {
            mostRecentRecord.unreadCount++;
            needsUpdate = true;
          }

          if (needsUpdate) {
            mostRecentStore.put(mostRecentRecord);
          }
        } else {
          mostRecentStore.add({
            senderOrReceiver: number,
            timestamp: aMessageRecord.timestamp,
            body: aMessageRecord.body,
            id: aMessageRecord.id,
            unreadCount: aMessageRecord.read ? 0 : 1
          });
        }
      };
    }, [MESSAGE_STORE_NAME, MOST_RECENT_STORE_NAME]);
    
    return aMessageRecord.id;
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
    let messageRecord;
    function notifyResult(rv) {
      if (!callback) {
        return;
      }
      callback.notify(rv, messageRecord);
    }

    this.newTxn(READ_WRITE, function (error, txn, messageStore) {
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

      let getRequest = messageStore.get(messageId);
      getRequest.onsuccess = function onsuccess(event) {
        messageRecord = event.target.result;
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          return;
        }
        if (messageRecord.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          return;
        }
        
        if ((messageRecord.delivery == delivery)
            && (messageRecord.deliveryStatus == deliveryStatus)) {
          if (DEBUG) {
            debug("The values of attribute delivery and deliveryStatus are the"
                  + " the same with given parameters.");
          }
          return;
        }
        messageRecord.delivery = delivery;
        messageRecord.deliveryIndex = [delivery, messageRecord.timestamp];
        messageRecord.deliveryStatus = deliveryStatus;
        if (DEBUG) {
          debug("Message.delivery set to: " + delivery
                + ", and Message.deliveryStatus set to: " + deliveryStatus);
        }
        messageStore.put(messageRecord);
      };
    });
  },

  



  getMessage: function getMessage(messageId, aRequest) {
    if (DEBUG) debug("Retrieving message with ID " + messageId);
    let self = this;
    this.newTxn(READ_ONLY, function (error, txn, messageStore) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      let request = messageStore.mozGetAll(messageId);

      txn.oncomplete = function oncomplete() {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (request.result.length > 1) {
          if (DEBUG) debug("Got too many results for id " + messageId);
          aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        let messageRecord = request.result[0];
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
          return;
        }
        if (messageRecord.id != messageId) {
          if (DEBUG) {
            debug("Requested message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aRequest.notifyGetMessageFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        let sms = self.createSmsMessageFromRecord(messageRecord);
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

      const messageStore = stores[0];
      const mostRecentStore = stores[1];

      let deleted = false;

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        aRequest.notifyMessageDeleted(deleted);
      };

      messageStore.get(messageId).onsuccess = function(event) {
        let messageRecord = event.target.result;
        if (messageRecord) {
          if (DEBUG) debug("Deleting message id " + messageId);

          
          messageStore.delete(messageId).onsuccess = function(event) {
            deleted = true;

            
            let number = getNumberFromRecord(messageRecord);

            mostRecentStore.get(number).onsuccess = function(event) {
              
              let mostRecentRecord = event.target.result;

              if (!messageRecord.read) {
                mostRecentRecord.unreadCount--;
              }

              if (mostRecentRecord.id == messageId) {
                
                let numberRange = IDBKeyRange.bound([number, 0], [number, ""]);
                let numberRequest = messageStore.index("number")
                                                .openCursor(numberRange, PREV);
                numberRequest.onsuccess = function(event) {
                  let cursor = event.target.result;
                  if (!cursor) {
                    if (DEBUG) {
                      debug("Deleting mru entry for number '" + number + "'");
                    }
                    mostRecentStore.delete(number);
                    return;
                  }

                  let nextMsg = cursor.value;
                  mostRecentRecord.id = nextMsg.id;
                  mostRecentRecord.timestamp = nextMsg.timestamp;
                  mostRecentRecord.body = nextMsg.body;
                  if (DEBUG) {
                    debug("Updating mru entry: " +
                          JSON.stringify(mostRecentRecord));
                  }
                  mostRecentStore.put(mostRecentRecord);
                };
              } else if (!messageRecord.read) {
                
                if (DEBUG) {
                  debug("Updating unread count for number '" + number + "': " +
                        (mostRecentRecord.unreadCount + 1) + " -> " +
                        mostRecentRecord.unreadCount);
                }
                mostRecentStore.put(mostRecentRecord);
              }
            };
          };
        } else if (DEBUG) {
          debug("Message id " + messageId + " does not exist");
        }
      };
    }, [MESSAGE_STORE_NAME, MOST_RECENT_STORE_NAME]);
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
    this.newTxn(READ_ONLY, function (error, txn, messageStore) {
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
        self.onNextMessageInListGot.bind(self, messageStore, messageList);

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
                                    .bind(self, messageStore, messageList);

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
          return messageStore.index(indexName).openKeyCursor(range, direction);
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
                  .bind(self, messageStore, messageList, contextIndex);

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

            let timeRequest = messageStore.index("timestamp")
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

        let request = messageStore.index("timestamp").openKeyCursor(range, direction);
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
    this.newTxn(READ_ONLY, function (error, txn, messageStore) {
      if (DEBUG) debug("Fetching message " + messageId);
      let request = messageStore.get(messageId);
      let messageRecord;
      request.onsuccess = function onsuccess(event) {
        messageRecord = request.result;
      };

      txn.oncomplete = function oncomplete(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (!messageRecord) {
          if (DEBUG) debug("Could not get message id " + messageId);
          aRequest.notifyReadMessageListFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
        }
        let sms = self.createSmsMessageFromRecord(messageRecord);
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
      let messageStore = stores[0];
      let mostRecentStore = stores[1];
      messageStore.get(messageId).onsuccess = function onsuccess(event) {
        let messageRecord = event.target.result;
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.NOT_FOUND_ERROR);
          return;
        }
        if (messageRecord.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aRequest.notifyMarkMessageReadFailed(Ci.nsISmsRequest.UNKNOWN_ERROR);
          return;
        }
        
        
        if (messageRecord.read == value) {
          if (DEBUG) debug("The value of messageRecord.read is already " + value);
          aRequest.notifyMessageMarkedRead(messageRecord.read);
          return;
        }
        messageRecord.read = value ? FILTER_READ_READ : FILTER_READ_UNREAD;
        messageRecord.readIndex = [messageRecord.read, messageRecord.timestamp];
        if (DEBUG) debug("Message.read set to: " + value);
        messageStore.put(messageRecord).onsuccess = function onsuccess(event) {
          if (DEBUG) {
            debug("Update successfully completed. Message: " +
                  JSON.stringify(event.target.result));
          }

          
          let number = getNumberFromRecord(messageRecord);

          mostRecentStore.get(number).onsuccess = function(event) {
            let mostRecentRecord = event.target.result;
            mostRecentRecord.unreadCount += value ? -1 : 1;
            if (DEBUG) {
              debug("Updating unreadCount for '" + number + "': " +
                    (value ?
                     mostRecentRecord.unreadCount + 1 :
                     mostRecentRecord.unreadCount - 1) +
                    " -> " + mostRecentRecord.unreadCount);
            }
            mostRecentStore.put(mostRecentRecord).onsuccess = function(event) {
              aRequest.notifyMessageMarkedRead(messageRecord.read);
            };
          };
        };
      };
    }, [MESSAGE_STORE_NAME, MOST_RECENT_STORE_NAME]);
  },
  getThreadList: function getThreadList(aRequest) {
    if (DEBUG) debug("Getting thread list");
    this.newTxn(READ_ONLY, function (error, txn, mostRecentStore) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyThreadListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.errorCode);
        aRequest.notifyThreadListFailed(Ci.nsISmsRequest.INTERNAL_ERROR);
      };
      mostRecentStore.index("timestamp").mozGetAll().onsuccess = function(event) {
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
