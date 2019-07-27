



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");
Cu.importGlobalProperties(["indexedDB"]);

XPCOMUtils.defineLazyGetter(this, "RIL", function () {
  let obj = {};
  Cu.import("resource://gre/modules/ril_consts.js", obj);
  return obj;
});

const RIL_GETMESSAGESCURSOR_CID =
  Components.ID("{484d1ad8-840e-4782-9dc4-9ebc4d914937}");
const RIL_GETTHREADSCURSOR_CID =
  Components.ID("{95ee7c3e-d6f2-4ec4-ade5-0c453c036d35}");

const DEBUG = false;
const DISABLE_MMS_GROUPING_FOR_RECEIVING = true;

const DB_VERSION = 23;

const MESSAGE_STORE_NAME = "sms";
const THREAD_STORE_NAME = "thread";
const PARTICIPANT_STORE_NAME = "participant";
const MOST_RECENT_STORE_NAME = "most-recent";
const SMS_SEGMENT_STORE_NAME = "sms-segment";

const DELIVERY_SENDING = "sending";
const DELIVERY_SENT = "sent";
const DELIVERY_RECEIVED = "received";
const DELIVERY_NOT_DOWNLOADED = "not-downloaded";
const DELIVERY_ERROR = "error";

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

const COLLECT_ID_END = 0;
const COLLECT_ID_ERROR = -1;
const COLLECT_TIMESTAMP_UNUSED = 0;


const DEFAULT_READ_AHEAD_ENTRIES = 7;

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gMMSService",
                                   "@mozilla.org/mms/rilmmsservice;1",
                                   "nsIMmsService");

XPCOMUtils.defineLazyGetter(this, "MMS", function() {
  let MMS = {};
  Cu.import("resource://gre/modules/MmsPduHelper.jsm", MMS);
  return MMS;
});




this.MobileMessageDB = function() {};
MobileMessageDB.prototype = {
  dbName: null,
  dbVersion: null,

  


  db: null,

  


  lastMessageId: 0,

  










  ensureDB: function(callback) {
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

    let request = indexedDB.open(this.dbName, this.dbVersion);
    request.onsuccess = function(event) {
      if (DEBUG) debug("Opened database:", self.dbName, self.dbVersion);
      gotDB(event.target.result);
    };
    request.onupgradeneeded = function(event) {
      if (DEBUG) {
        debug("Database needs upgrade:", self.dbName,
              event.oldVersion, event.newVersion);
        debug("Correct new database version:", event.newVersion == self.dbVersion);
      }

      let db = event.target.result;

      let currentVersion = event.oldVersion;

      function update(currentVersion) {
        if (currentVersion >= self.dbVersion) {
          if (DEBUG) debug("Upgrade finished.");
          return;
        }

        let next = update.bind(self, currentVersion + 1);
        switch (currentVersion) {
          case 0:
            if (DEBUG) debug("New database");
            self.createSchema(db, next);
            break;
          case 1:
            if (DEBUG) debug("Upgrade to version 2. Including `read` index");
            self.upgradeSchema(event.target.transaction, next);
            break;
          case 2:
            if (DEBUG) debug("Upgrade to version 3. Fix existing entries.");
            self.upgradeSchema2(event.target.transaction, next);
            break;
          case 3:
            if (DEBUG) debug("Upgrade to version 4. Add quick threads view.");
            self.upgradeSchema3(db, event.target.transaction, next);
            break;
          case 4:
            if (DEBUG) debug("Upgrade to version 5. Populate quick threads view.");
            self.upgradeSchema4(event.target.transaction, next);
            break;
          case 5:
            if (DEBUG) debug("Upgrade to version 6. Use PhonenumberJS.");
            self.upgradeSchema5(event.target.transaction, next);
            break;
          case 6:
            if (DEBUG) debug("Upgrade to version 7. Use multiple entry indexes.");
            self.upgradeSchema6(event.target.transaction, next);
            break;
          case 7:
            if (DEBUG) debug("Upgrade to version 8. Add participant/thread stores.");
            self.upgradeSchema7(db, event.target.transaction, next);
            break;
          case 8:
            if (DEBUG) debug("Upgrade to version 9. Add transactionId index for incoming MMS.");
            self.upgradeSchema8(event.target.transaction, next);
            break;
          case 9:
            if (DEBUG) debug("Upgrade to version 10. Upgrade type if it's not existing.");
            self.upgradeSchema9(event.target.transaction, next);
            break;
          case 10:
            if (DEBUG) debug("Upgrade to version 11. Add last message type into threadRecord.");
            self.upgradeSchema10(event.target.transaction, next);
            break;
          case 11:
            if (DEBUG) debug("Upgrade to version 12. Add envelopeId index for outgoing MMS.");
            self.upgradeSchema11(event.target.transaction, next);
            break;
          case 12:
            if (DEBUG) debug("Upgrade to version 13. Replaced deliveryStatus by deliveryInfo.");
            self.upgradeSchema12(event.target.transaction, next);
            break;
          case 13:
            if (DEBUG) debug("Upgrade to version 14. Fix the wrong participants.");
            
            
            
            self.needReUpgradeSchema12(event.target.transaction, function(isNeeded) {
              if (isNeeded) {
                self.upgradeSchema12(event.target.transaction, function() {
                  self.upgradeSchema13(event.target.transaction, next);
                });
              } else {
                self.upgradeSchema13(event.target.transaction, next);
              }
            });
            break;
          case 14:
            if (DEBUG) debug("Upgrade to version 15. Add deliveryTimestamp.");
            self.upgradeSchema14(event.target.transaction, next);
            break;
          case 15:
            if (DEBUG) debug("Upgrade to version 16. Add ICC ID for each message.");
            self.upgradeSchema15(event.target.transaction, next);
            break;
          case 16:
            if (DEBUG) debug("Upgrade to version 17. Add isReadReportSent for incoming MMS.");
            self.upgradeSchema16(event.target.transaction, next);
            break;
          case 17:
            if (DEBUG) debug("Upgrade to version 18. Add last message subject into threadRecord.");
            self.upgradeSchema17(event.target.transaction, next);
            break;
          case 18:
            if (DEBUG) debug("Upgrade to version 19. Add pid for incoming SMS.");
            self.upgradeSchema18(event.target.transaction, next);
            break;
          case 19:
            if (DEBUG) debug("Upgrade to version 20. Add readStatus and readTimestamp.");
            self.upgradeSchema19(event.target.transaction, next);
            break;
          case 20:
            if (DEBUG) debug("Upgrade to version 21. Add sentTimestamp.");
            self.upgradeSchema20(event.target.transaction, next);
            break;
          case 21:
            if (DEBUG) debug("Upgrade to version 22. Add sms-segment store.");
            self.upgradeSchema21(db, event.target.transaction, next);
            break;
          case 22:
            if (DEBUG) debug("Upgrade to version 23. Add type information to receivers and to");
            self.upgradeSchema22(event.target.transaction, next);
            break;
          default:
            event.target.transaction.abort();
            if (DEBUG) debug("unexpected db version: " + event.oldVersion);
            callback(Cr.NS_ERROR_FAILURE, null);
            break;
        }
      }

      update(currentVersion);
    };
    request.onerror = function(event) {
      
      if (DEBUG) debug("Error opening database!");
      callback(Cr.NS_ERROR_FAILURE, null);
    };
    request.onblocked = function(event) {
      if (DEBUG) debug("Opening database request is blocked.");
      callback(Cr.NS_ERROR_FAILURE, null);
    };
  },

  










  newTxn: function(txn_type, callback, storeNames) {
    if (!storeNames) {
      storeNames = [MESSAGE_STORE_NAME];
    }
    if (DEBUG) debug("Opening transaction for object stores: " + storeNames);
    let self = this;
    this.ensureDB(function(error, db) {
      if (error) {
        if (DEBUG) debug("Could not open database: " + error);
        callback(error);
        return;
      }
      let txn = db.transaction(storeNames, txn_type);
      if (DEBUG) debug("Started transaction " + txn + " of type " + txn_type);
      if (DEBUG) {
        txn.oncomplete = function(event) {
          debug("Transaction " + txn + " completed.");
        };
        txn.onerror = function(event) {
          
          
          debug("Error occurred during transaction: " + event.target.error.name);
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

  











  init: function(aDbName, aDbVersion, aCallback) {
    this.dbName = aDbName;
    this.dbVersion = aDbVersion || DB_VERSION;

    let self = this;
    this.newTxn(READ_ONLY, function(error, txn, messageStore){
      if (error) {
        if (aCallback) {
          aCallback(error);
        }
        return;
      }

      if (aCallback) {
        txn.oncomplete = function() {
          aCallback(null);
        };
      }

      
      
      let request = messageStore.openCursor(null, PREV);
      request.onsuccess = function(event) {
        let cursor = event.target.result;
        if (!cursor) {
          if (DEBUG) {
            debug("Could not get the last key from mobile message database. " +
                  "Probably empty database");
          }
          return;
        }
        self.lastMessageId = cursor.key || 0;
        if (DEBUG) debug("Last assigned message ID was " + self.lastMessageId);
      };
      request.onerror = function(event) {
        if (DEBUG) {
          debug("Could not get the last key from mobile message database " +
                event.target.error.name);
        }
      };
    });
  },

  close: function() {
    if (!this.db) {
      return;
    }

    this.db.close();
    this.db = null;
    this.lastMessageId = 0;
  },

  



  updatePendingTransactionToError: function(aError) {
    if (aError) {
      return;
    }

    this.newTxn(READ_WRITE, function(error, txn, messageStore) {
      if (error) {
        return;
      }

      let deliveryIndex = messageStore.index("delivery");

      
      
      let keyRange = IDBKeyRange.bound([DELIVERY_SENDING, 0], [DELIVERY_SENDING, ""]);
      let cursorRequestSending = deliveryIndex.openCursor(keyRange);
      cursorRequestSending.onsuccess = function(event) {
        let messageCursor = event.target.result;
        if (!messageCursor) {
          return;
        }

        let messageRecord = messageCursor.value;

        
        messageRecord.delivery = DELIVERY_ERROR;
        messageRecord.deliveryIndex = [DELIVERY_ERROR, messageRecord.timestamp];

        if (messageRecord.type == "sms") {
          messageRecord.deliveryStatus = DELIVERY_STATUS_ERROR;
        } else {
          
          for (let i = 0; i < messageRecord.deliveryInfo.length; i++) {
            messageRecord.deliveryInfo[i].deliveryStatus = DELIVERY_STATUS_ERROR;
          }
        }

        messageCursor.update(messageRecord);
        messageCursor.continue();
      };

      
      
      keyRange = IDBKeyRange.bound([DELIVERY_NOT_DOWNLOADED, 0], [DELIVERY_NOT_DOWNLOADED, ""]);
      let cursorRequestNotDownloaded = deliveryIndex.openCursor(keyRange);
      cursorRequestNotDownloaded.onsuccess = function(event) {
        let messageCursor = event.target.result;
        if (!messageCursor) {
          return;
        }

        let messageRecord = messageCursor.value;

        
        if (messageRecord.type == "sms") {
          messageCursor.continue();
          return;
        }

        
        let deliveryInfo = messageRecord.deliveryInfo;
        if (deliveryInfo.length == 1 &&
            deliveryInfo[0].deliveryStatus == DELIVERY_STATUS_PENDING) {
          deliveryInfo[0].deliveryStatus = DELIVERY_STATUS_ERROR;
        }

        messageCursor.update(messageRecord);
        messageCursor.continue();
      };
    });
  },

  





  createSchema: function(db, next) {
    
    let messageStore = db.createObjectStore(MESSAGE_STORE_NAME, { keyPath: "id" });
    messageStore.createIndex("timestamp", "timestamp", { unique: false });
    if (DEBUG) debug("Created object stores and indexes");
    next();
  },

  


  upgradeSchema: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.createIndex("read", "read", { unique: false });
    next();
  },

  upgradeSchema2: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      messageRecord.messageClass = MESSAGE_CLASS_NORMAL;
      messageRecord.deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  upgradeSchema3: function(db, transaction, next) {
    
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    if (messageStore.indexNames.contains("id")) {
      messageStore.deleteIndex("id");
    }

    










    let mostRecentStore = db.createObjectStore(MOST_RECENT_STORE_NAME,
                                               { keyPath: "senderOrReceiver" });
    mostRecentStore.createIndex("timestamp", "timestamp");
    next();
  },

  upgradeSchema4: function(transaction, next) {
    let threads = {};
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    let mostRecentStore = transaction.objectStore(MOST_RECENT_STORE_NAME);

    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        for (let thread in threads) {
          mostRecentStore.put(threads[thread]);
        }
        next();
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

  upgradeSchema5: function(transaction, next) {
    
    next();
  },

  upgradeSchema6: function(transaction, next) {
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
        next();
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

  















  upgradeSchema7: function(db, transaction, next) {
    







    let participantStore = db.createObjectStore(PARTICIPANT_STORE_NAME,
                                                { keyPath: "id",
                                                  autoIncrement: true });
    participantStore.createIndex("addresses", "addresses", { multiEntry: true });

    












    let threadStore = db.createObjectStore(THREAD_STORE_NAME,
                                           { keyPath: "id",
                                             autoIncrement: true });
    threadStore.createIndex("participantIds", "participantIds");
    threadStore.createIndex("lastTimestamp", "lastTimestamp");

    



    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.createIndex("threadId", "threadIdIndex");
    messageStore.createIndex("participantIds", "participantIdsIndex",
                             { multiEntry: true });

    
    let mostRecentStore = transaction.objectStore(MOST_RECENT_STORE_NAME);
    let self = this;
    let mostRecentRequest = mostRecentStore.openCursor();
    mostRecentRequest.onsuccess = function(event) {
      let mostRecentCursor = event.target.result;
      if (!mostRecentCursor) {
        db.deleteObjectStore(MOST_RECENT_STORE_NAME);

        
        
        messageStore.deleteIndex("number");
        next();
        return;
      }

      let mostRecentRecord = mostRecentCursor.value;

      
      
      
      let number = mostRecentRecord.senderOrReceiver;
      self.findParticipantRecordByPlmnAddress(participantStore, number, true,
                                              function(participantRecord) {
        
        let threadRecord = {
          participantIds: [participantRecord.id],
          participantAddresses: [number],
          lastMessageId: mostRecentRecord.id,
          lastTimestamp: mostRecentRecord.timestamp,
          subject: mostRecentRecord.body,
          unreadCount: mostRecentRecord.unreadCount,
        };
        let addThreadRequest = threadStore.add(threadRecord);
        addThreadRequest.onsuccess = function(event) {
          threadRecord.id = event.target.result;

          let numberRange = IDBKeyRange.bound([number, 0], [number, ""]);
          let messageRequest = messageStore.index("number")
                                           .openCursor(numberRange, NEXT);
          messageRequest.onsuccess = function(event) {
            let messageCursor = event.target.result;
            if (!messageCursor) {
              
              mostRecentCursor.continue();
              return;
            }

            let messageRecord = messageCursor.value;
            
            let matchSenderOrReceiver = false;
            if (messageRecord.delivery == DELIVERY_RECEIVED) {
              if (messageRecord.sender == number) {
                matchSenderOrReceiver = true;
              }
            } else if (messageRecord.receiver == number) {
              matchSenderOrReceiver = true;
            }
            if (!matchSenderOrReceiver) {
              
              messageCursor.continue();
              return;
            }

            messageRecord.threadId = threadRecord.id;
            messageRecord.threadIdIndex = [threadRecord.id,
                                           messageRecord.timestamp];
            messageRecord.participantIdsIndex = [
              [participantRecord.id, messageRecord.timestamp]
            ];
            messageCursor.update(messageRecord);
            
            messageCursor.continue();
          };
          messageRequest.onerror = function() {
            
            mostRecentCursor.continue();
          };
        };
        addThreadRequest.onerror = function() {
          
          mostRecentCursor.continue();
        };
      });
    };
  },

  


  upgradeSchema8: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    
    if (messageStore.indexNames.contains("transactionId")) {
      messageStore.deleteIndex("transactionId");
    }

    
    messageStore.createIndex("transactionId", "transactionIdIndex", { unique: true });

    
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if ("mms" == messageRecord.type &&
          (DELIVERY_NOT_DOWNLOADED == messageRecord.delivery ||
           DELIVERY_RECEIVED == messageRecord.delivery)) {
        messageRecord.transactionIdIndex =
          messageRecord.headers["x-mms-transaction-id"];
        cursor.update(messageRecord);
      }
      cursor.continue();
    };
  },

  upgradeSchema9: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == undefined) {
        messageRecord.type = "sms";
        cursor.update(messageRecord);
      }
      cursor.continue();
    };
  },

  upgradeSchema10: function(transaction, next) {
    let threadStore = transaction.objectStore(THREAD_STORE_NAME);

    
    threadStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let threadRecord = cursor.value;
      let lastMessageId = threadRecord.lastMessageId;
      let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
      let request = messageStore.mozGetAll(lastMessageId);

      request.onsuccess = function() {
        let messageRecord = request.result[0];
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + lastMessageId + " not found");
          return;
        }
        if (messageRecord.id != lastMessageId) {
          if (DEBUG) {
            debug("Requested message ID (" + lastMessageId + ") is different from" +
                  " the one we got");
          }
          return;
        }
        threadRecord.lastMessageType = messageRecord.type;
        cursor.update(threadRecord);
        cursor.continue();
      };

      request.onerror = function(event) {
        if (DEBUG) {
          if (event.target) {
            debug("Caught error on transaction", event.target.error.name);
          }
        }
        cursor.continue();
      };
    };
  },

  


  upgradeSchema11: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    
    if (messageStore.indexNames.contains("envelopeId")) {
      messageStore.deleteIndex("envelopeId");
    }

    
    messageStore.createIndex("envelopeId", "envelopeIdIndex", { unique: true });

    
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "mms" &&
          messageRecord.delivery == DELIVERY_SENT) {
        messageRecord.envelopeIdIndex = messageRecord.headers["message-id"];
        cursor.update(messageRecord);
      }
      cursor.continue();
    };
  },

  


  upgradeSchema12: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "mms") {
        messageRecord.deliveryInfo = [];

        if (messageRecord.deliveryStatus.length == 1 &&
            (messageRecord.delivery == DELIVERY_NOT_DOWNLOADED ||
             messageRecord.delivery == DELIVERY_RECEIVED)) {
          messageRecord.deliveryInfo.push({
            receiver: null,
            deliveryStatus: messageRecord.deliveryStatus[0] });
        } else {
          for (let i = 0; i < messageRecord.deliveryStatus.length; i++) {
            messageRecord.deliveryInfo.push({
              receiver: messageRecord.receivers[i],
              deliveryStatus: messageRecord.deliveryStatus[i] });
          }
        }
        delete messageRecord.deliveryStatus;
        cursor.update(messageRecord);
      }
      cursor.continue();
    };
  },

  


  needReUpgradeSchema12: function(transaction, callback) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        callback(false);
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "mms" &&
          messageRecord.deliveryInfo === undefined) {
        callback(true);
        return;
      }
      cursor.continue();
    };
  },

  


  upgradeSchema13: function(transaction, next) {
    let participantStore = transaction.objectStore(PARTICIPANT_STORE_NAME);
    let threadStore = transaction.objectStore(THREAD_STORE_NAME);
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    let self = this;

    let isInvalid = function(participantRecord) {
      let entries = [];
      for (let addr of participantRecord.addresses) {
        entries.push({
          normalized: addr,
          parsed: PhoneNumberUtils.parseWithMCC(addr, null)
        })
      }
      for (let ix = 0 ; ix < entries.length - 1; ix++) {
        let entry1 = entries[ix];
        for (let iy = ix + 1 ; iy < entries.length; iy ++) {
          let entry2 = entries[iy];
          if (!self.matchPhoneNumbers(entry1.normalized, entry1.parsed,
                                      entry2.normalized, entry2.parsed)) {
            return true;
          }
        }
      }
      return false;
    };

    let invalidParticipantIds = [];
    participantStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (cursor) {
        let participantRecord = cursor.value;
        
        if (isInvalid(participantRecord)) {
          invalidParticipantIds.push(participantRecord.id);
          cursor.delete();
        }
        cursor.continue();
        return;
      }

      
      if (!invalidParticipantIds.length) {
        next();
        return;
      }

      
      let wrongThreads = [];
      threadStore.openCursor().onsuccess = function(event) {
        let threadCursor = event.target.result;
        if (threadCursor) {
          let threadRecord = threadCursor.value;
          let participantIds = threadRecord.participantIds;
          let foundInvalid = false;
          for (let invalidParticipantId of invalidParticipantIds) {
            if (participantIds.indexOf(invalidParticipantId) != -1) {
              foundInvalid = true;
              break;
            }
          }
          if (foundInvalid) {
            wrongThreads.push(threadRecord.id);
            threadCursor.delete();
          }
          threadCursor.continue();
          return;
        }

        if (!wrongThreads.length) {
          next();
          return;
        }
        
        (function createUpdateThreadAndParticipant(ix) {
          let threadId = wrongThreads[ix];
          let range = IDBKeyRange.bound([threadId, 0], [threadId, ""]);
          messageStore.index("threadId").openCursor(range).onsuccess = function(event) {
            let messageCursor = event.target.result;
            if (!messageCursor) {
              ix++;
              if (ix === wrongThreads.length) {
                next();
                return;
              }
              createUpdateThreadAndParticipant(ix);
              return;
            }

            let messageRecord = messageCursor.value;
            let timestamp = messageRecord.timestamp;
            let threadParticipants = [];
            
            if (messageRecord.delivery === DELIVERY_RECEIVED ||
                messageRecord.delivery === DELIVERY_NOT_DOWNLOADED) {
              threadParticipants.push(messageRecord.sender);
              if (messageRecord.type == "mms") {
                this.fillReceivedMmsThreadParticipants(messageRecord, threadParticipants);
              }
            }
            
            
            
            
            else if (messageRecord.delivery === DELIVERY_SENT ||
                messageRecord.delivery === DELIVERY_ERROR) {
              if (messageRecord.type == "sms") {
                threadParticipants = [messageRecord.receiver];
              } else if (messageRecord.type == "mms") {
                threadParticipants = messageRecord.receivers;
              }
            }
            self.findThreadRecordByPlmnAddresses(threadStore, participantStore,
                                                 threadParticipants, true,
                                                 function(threadRecord,
                                                          participantIds) {
              if (!participantIds) {
                debug("participantIds is empty!");
                return;
              }

              let timestamp = messageRecord.timestamp;
              
              messageRecord.participantIdsIndex = [];
              for each (let id in participantIds) {
                messageRecord.participantIdsIndex.push([id, timestamp]);
              }
              if (threadRecord) {
                let needsUpdate = false;

                if (threadRecord.lastTimestamp <= timestamp) {
                  threadRecord.lastTimestamp = timestamp;
                  threadRecord.subject = messageRecord.body;
                  threadRecord.lastMessageId = messageRecord.id;
                  threadRecord.lastMessageType = messageRecord.type;
                  needsUpdate = true;
                }

                if (!messageRecord.read) {
                  threadRecord.unreadCount++;
                  needsUpdate = true;
                }

                if (needsUpdate) {
                  threadStore.put(threadRecord);
                }
                messageRecord.threadId = threadRecord.id;
                messageRecord.threadIdIndex = [threadRecord.id, timestamp];
                messageCursor.update(messageRecord);
                messageCursor.continue();
                return;
              }

              let threadRecord = {
                participantIds: participantIds,
                participantAddresses: threadParticipants,
                lastMessageId: messageRecord.id,
                lastTimestamp: timestamp,
                subject: messageRecord.body,
                unreadCount: messageRecord.read ? 0 : 1,
                lastMessageType: messageRecord.type
              };
              threadStore.add(threadRecord).onsuccess = function(event) {
                let threadId = event.target.result;
                
                messageRecord.threadId = threadId;
                messageRecord.threadIdIndex = [threadId, timestamp];
                messageCursor.update(messageRecord);
                messageCursor.continue();
              };
            });
          };
        })(0);
      };
    };
  },

  


  upgradeSchema14: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "sms") {
        messageRecord.deliveryTimestamp = 0;
      } else if (messageRecord.type == "mms") {
        let deliveryInfo = messageRecord.deliveryInfo;
        for (let i = 0; i < deliveryInfo.length; i++) {
          deliveryInfo[i].deliveryTimestamp = 0;
        }
      }
      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  


  upgradeSchema15: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      messageRecord.iccId = null;
      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  


  upgradeSchema16: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "mms") {
        messageRecord.isReadReportSent = false;
        cursor.update(messageRecord);
      }
      cursor.continue();
    };
  },

  upgradeSchema17: function(transaction, next) {
    let threadStore = transaction.objectStore(THREAD_STORE_NAME);
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    
    threadStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let threadRecord = cursor.value;
      
      
      threadRecord.body = threadRecord.subject;
      delete threadRecord.subject;

      
      if (threadRecord.lastMessageType != "mms") {
        threadRecord.lastMessageSubject = null;
        cursor.update(threadRecord);

        cursor.continue();
        return;
      }

      messageStore.get(threadRecord.lastMessageId).onsuccess = function(event) {
        let messageRecord = event.target.result;
        let subject = messageRecord.headers.subject;
        threadRecord.lastMessageSubject = subject || null;
        cursor.update(threadRecord);

        cursor.continue();
      };
    };
  },

  


  upgradeSchema18: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "sms") {
        messageRecord.pid = RIL.PDU_PID_DEFAULT;
        cursor.update(messageRecord);
      }
      cursor.continue();
    };
  },

  


  upgradeSchema19: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      if (messageRecord.type == "sms") {
        cursor.continue();
        return;
      }

      
      
      if (messageRecord.hasOwnProperty("transactionId")) {
        delete messageRecord.transactionId;
      }

      
      
      if (messageRecord.envelopeIdIndex === "undefined") {
        delete messageRecord.envelopeIdIndex;
      }

      
      
      for (let field of ["x-mms-cancel-status",
                         "x-mms-sender-visibility",
                         "x-mms-read-status"]) {
        let value = messageRecord.headers[field];
        if (value !== undefined) {
          messageRecord.headers[field] = value ? 128 : 129;
        }
      }

      
      
      let readReportRequested =
        messageRecord.headers["x-mms-read-report"] || false;
      for (let element of messageRecord.deliveryInfo) {
        element.readStatus = readReportRequested
                           ? MMS.DOM_READ_STATUS_PENDING
                           : MMS.DOM_READ_STATUS_NOT_APPLICABLE;
        element.readTimestamp = 0;
      }

      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  


  upgradeSchema20: function(transaction, next) {
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);
    messageStore.openCursor().onsuccess = function(event) {
      let cursor = event.target.result;
      if (!cursor) {
        next();
        return;
      }

      let messageRecord = cursor.value;
      messageRecord.sentTimestamp = 0;

      
      
      if (messageRecord.type == "mms" && messageRecord.headers["date"]) {
        messageRecord.sentTimestamp = messageRecord.headers["date"].getTime();
      }

      cursor.update(messageRecord);
      cursor.continue();
    };
  },

  


  upgradeSchema21: function(db, transaction, next) {
    















































    let smsSegmentStore = db.createObjectStore(SMS_SEGMENT_STORE_NAME,
                                               { keyPath: "id",
                                                 autoIncrement: true });
    smsSegmentStore.createIndex("hash", "hash", { unique: true });
    next();
  },

  


  upgradeSchema22: function(transaction, next) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    function getThreadParticipantsFromMessageRecord(aMessageRecord) {
      let threadParticipants;

      if (aMessageRecord.type == "sms") {
        let address;
        if (aMessageRecord.delivery == DELIVERY_RECEIVED) {
          address = aMessageRecord.sender;
        } else {
          address = aMessageRecord.receiver;
        }
        threadParticipants = [{
          address: address,
          type: MMS.Address.resolveType(address)
        }];
      } else { 
        if ((aMessageRecord.delivery == DELIVERY_RECEIVED) ||
            (aMessageRecord.delivery == DELIVERY_NOT_DOWNLOADED)) {
          
          
          threadParticipants = [{
            address: aMessageRecord.sender,
            type: MMS.Address.resolveType(aMessageRecord.sender)
          }];
        } else {
          threadParticipants = aMessageRecord.headers.to;
        }
      }

      return threadParticipants;
    }

    let participantStore = transaction.objectStore(PARTICIPANT_STORE_NAME);
    let threadStore = transaction.objectStore(THREAD_STORE_NAME);
    let messageStore = transaction.objectStore(MESSAGE_STORE_NAME);

    let invalidThreadIds = [];

    let self = this;
    let messageCursorReq = messageStore.openCursor();
    messageCursorReq.onsuccess = function(aEvent) {
      let messageCursor = aEvent.target.result;
      if (messageCursor) {
        let messageRecord = messageCursor.value;
        let threadParticipants =
          getThreadParticipantsFromMessageRecord(messageRecord);

        
        
        if (invalidThreadIds.indexOf(messageRecord.threadId) >= 0) {
          messageCursor.continue();
          return;
        }

        
        
        self.findThreadRecordByTypedAddresses(threadStore, participantStore,
                                              threadParticipants, true,
                                              function(aThreadRecord,
                                                       aParticipantIds) {
          if (!aThreadRecord || aThreadRecord.id !== messageRecord.threadId) {
            invalidThreadIds.push(messageRecord.threadId);
          }

          messageCursor.continue();
        });

        
        
        
        return;
      } 

      
      if (!invalidThreadIds.length) {
        next();
        return;
      }

      
      
      invalidThreadIds.forEach(function(aInvalidThreadId) {
        threadStore.delete(aInvalidThreadId);
      });

      
      (function redoThreading(aInvalidThreadId) {
        
        
        let range = IDBKeyRange.bound([aInvalidThreadId, 0],
                                      [aInvalidThreadId, ""]);
        let threadMessageCursorReq = messageStore.index("threadId")
                                                 .openCursor(range, NEXT);
        threadMessageCursorReq.onsuccess = function(aEvent) {
          let messageCursor = aEvent.target.result;

          
          
          
          if (!messageCursor) {
            if (invalidThreadIds.length) {
              redoThreading(invalidThreadIds.shift());
            } else {
              next();
            }
            return;
          }

          let messageRecord = messageCursor.value;
          let threadParticipants =
            getThreadParticipantsFromMessageRecord(messageRecord);

          
          
          
          
          self.findThreadRecordByTypedAddresses(threadStore, participantStore,
                                                threadParticipants, true,
                                                function(aThreadRecord,
                                                         aParticipantIds) {
            
            messageRecord.participantIdsIndex =
              aParticipantIds.map(function(aParticipantId) {
                return [aParticipantId, messageRecord.timestamp];
              });

            let threadExists = aThreadRecord ? true : false;
            if (!threadExists) {
              aThreadRecord = {
                participantIds: aParticipantIds,
                participantAddresses:
                  threadParticipants.map(function(aTypedAddress) {
                    return aTypedAddress.address;
                  }),
                unreadCount: 0,
                lastTimestamp: -1
              };
            }

            let needsUpdate = false;
            if (aThreadRecord.lastTimestamp <= messageRecord.timestamp) {
              let lastMessageSubject;
              if (messageRecord.type == "mms") {
                lastMessageSubject = messageRecord.headers.subject;
              }
              aThreadRecord.lastMessageSubject = lastMessageSubject || null;
              aThreadRecord.lastTimestamp = messageRecord.timestamp;
              aThreadRecord.body = messageRecord.body;
              aThreadRecord.lastMessageId = messageRecord.id;
              aThreadRecord.lastMessageType = messageRecord.type;
              needsUpdate = true;
            }

            if (!messageRecord.read) {
              aThreadRecord.unreadCount++;
              needsUpdate = true;
            }

            let updateMessageRecordThreadId = function(aThreadId) {
              
              messageRecord.threadId = aThreadId;
              messageRecord.threadIdIndex = [aThreadId, messageRecord.timestamp];

              messageCursor.update(messageRecord);
              messageCursor.continue();
            };

            if (threadExists) {
              if (needsUpdate) {
                threadStore.put(aThreadRecord);
              }
              updateMessageRecordThreadId(aThreadRecord.id);
            } else {
              threadStore.add(aThreadRecord).onsuccess = function(aEvent) {
                let threadId = aEvent.target.result;
                updateMessageRecordThreadId(threadId);
              };
            }
          }); 
        }; 
      })(invalidThreadIds.shift()); 
    }; 
  },

  matchParsedPhoneNumbers: function(addr1, parsedAddr1, addr2, parsedAddr2) {
    if ((parsedAddr1.internationalNumber &&
         parsedAddr1.internationalNumber === parsedAddr2.internationalNumber) ||
        (parsedAddr1.nationalNumber &&
         parsedAddr1.nationalNumber === parsedAddr2.nationalNumber)) {
      return true;
    }

    if (parsedAddr1.countryName != parsedAddr2.countryName) {
      return false;
    }

    let ssPref = "dom.phonenumber.substringmatching." + parsedAddr1.countryName;
    if (Services.prefs.getPrefType(ssPref) != Ci.nsIPrefBranch.PREF_INT) {
      return false;
    }

    let val = Services.prefs.getIntPref(ssPref);
    return addr1.length > val &&
           addr2.length > val &&
           addr1.slice(-val) === addr2.slice(-val);
  },

  matchPhoneNumbers: function(addr1, parsedAddr1, addr2, parsedAddr2) {
    if (parsedAddr1 && parsedAddr2) {
      return this.matchParsedPhoneNumbers(addr1, parsedAddr1, addr2, parsedAddr2);
    }

    if (parsedAddr1) {
      parsedAddr2 = PhoneNumberUtils.parseWithCountryName(addr2, parsedAddr1.countryName);
      if (parsedAddr2) {
        return this.matchParsedPhoneNumbers(addr1, parsedAddr1, addr2, parsedAddr2);
      }

      return false;
    }

    if (parsedAddr2) {
      parsedAddr1 = PhoneNumberUtils.parseWithCountryName(addr1, parsedAddr2.countryName);
      if (parsedAddr1) {
        return this.matchParsedPhoneNumbers(addr1, parsedAddr1, addr2, parsedAddr2);
      }
    }

    return false;
  },

  createDomMessageFromRecord: function(aMessageRecord) {
    if (DEBUG) {
      debug("createDomMessageFromRecord: " + JSON.stringify(aMessageRecord));
    }
    if (aMessageRecord.type == "sms") {
      return gMobileMessageService.createSmsMessage(aMessageRecord.id,
                                                    aMessageRecord.threadId,
                                                    aMessageRecord.iccId,
                                                    aMessageRecord.delivery,
                                                    aMessageRecord.deliveryStatus,
                                                    aMessageRecord.sender,
                                                    aMessageRecord.receiver,
                                                    aMessageRecord.body,
                                                    aMessageRecord.messageClass,
                                                    aMessageRecord.timestamp,
                                                    aMessageRecord.sentTimestamp,
                                                    aMessageRecord.deliveryTimestamp,
                                                    aMessageRecord.read);
    } else if (aMessageRecord.type == "mms") {
      let headers = aMessageRecord["headers"];
      if (DEBUG) {
        debug("MMS: headers: " + JSON.stringify(headers));
      }

      let subject = headers["subject"];
      if (subject == undefined) {
        subject = "";
      }

      let smil = "";
      let attachments = [];
      let parts = aMessageRecord.parts;
      if (parts) {
        for (let i = 0; i < parts.length; i++) {
          let part = parts[i];
          if (DEBUG) {
            debug("MMS: part[" + i + "]: " + JSON.stringify(part));
          }
          
          
          if (!part) {
            continue;
          }

          let partHeaders = part["headers"];
          let partContent = part["content"];
          
          if (partHeaders["content-type"]["media"] == "application/smil") {
            smil = partContent;
            continue;
          }
          attachments.push({
            "id": partHeaders["content-id"],
            "location": partHeaders["content-location"],
            "content": partContent
          });
        }
      }
      let expiryDate = 0;
      if (headers["x-mms-expiry"] != undefined) {
        expiryDate = aMessageRecord.timestamp + headers["x-mms-expiry"] * 1000;
      }
      let readReportRequested = headers["x-mms-read-report"] || false;
      return gMobileMessageService.createMmsMessage(aMessageRecord.id,
                                                    aMessageRecord.threadId,
                                                    aMessageRecord.iccId,
                                                    aMessageRecord.delivery,
                                                    aMessageRecord.deliveryInfo,
                                                    aMessageRecord.sender,
                                                    aMessageRecord.receivers,
                                                    aMessageRecord.timestamp,
                                                    aMessageRecord.sentTimestamp,
                                                    aMessageRecord.read,
                                                    subject,
                                                    smil,
                                                    attachments,
                                                    expiryDate,
                                                    readReportRequested);
    }
  },

  createParticipantRecord: function(aParticipantStore, aAddresses, aCallback) {
    let participantRecord = { addresses: aAddresses };
    let addRequest = aParticipantStore.add(participantRecord);
    addRequest.onsuccess = function(event) {
      participantRecord.id = event.target.result;
      if (DEBUG) {
        debug("createParticipantRecord: " + JSON.stringify(participantRecord));
      }
      aCallback(participantRecord);
    };
  },

  findParticipantRecordByPlmnAddress: function(aParticipantStore, aAddress,
                                               aCreate, aCallback) {
    if (DEBUG) {
      debug("findParticipantRecordByPlmnAddress("
            + JSON.stringify(aAddress) + ", " + aCreate + ")");
    }

    
    
    

    
    let normalizedAddress = PhoneNumberUtils.normalize(aAddress, false);
    let allPossibleAddresses = [normalizedAddress];
    let parsedAddress = PhoneNumberUtils.parse(normalizedAddress);
    if (parsedAddress && parsedAddress.internationalNumber &&
        allPossibleAddresses.indexOf(parsedAddress.internationalNumber) < 0) {
      
      
      
      allPossibleAddresses.push(parsedAddress.internationalNumber);
    }
    if (DEBUG) {
      debug("findParticipantRecordByPlmnAddress: allPossibleAddresses = " +
            JSON.stringify(allPossibleAddresses));
    }

    
    let needles = allPossibleAddresses.slice(0);
    let request = aParticipantStore.index("addresses").get(needles.pop());
    request.onsuccess = (function onsuccess(event) {
      let participantRecord = event.target.result;
      
      
      if (participantRecord) {
        if (DEBUG) {
          debug("findParticipantRecordByPlmnAddress: got "
                + JSON.stringify(participantRecord));
        }
        aCallback(participantRecord);
        return;
      }

      
      if (needles.length) {
        let request = aParticipantStore.index("addresses").get(needles.pop());
        request.onsuccess = onsuccess.bind(this);
        return;
      }

      
      aParticipantStore.openCursor().onsuccess = (function(event) {
        let cursor = event.target.result;
        if (!cursor) {
          
          if (!aCreate) {
            aCallback(null);
            return;
          }

          this.createParticipantRecord(aParticipantStore, [normalizedAddress],
                                       aCallback);
          return;
        }

        let participantRecord = cursor.value;
        for (let storedAddress of participantRecord.addresses) {
          let parsedStoredAddress = PhoneNumberUtils.parseWithMCC(storedAddress, null);
          let match = this.matchPhoneNumbers(normalizedAddress, parsedAddress,
                                             storedAddress, parsedStoredAddress);
          if (!match) {
            
            continue;
          }
          
          if (aCreate) {
            
            
            participantRecord.addresses =
              participantRecord.addresses.concat(allPossibleAddresses);
            cursor.update(participantRecord);
          }

          if (DEBUG) {
            debug("findParticipantRecordByPlmnAddress: match "
                  + JSON.stringify(cursor.value));
          }
          aCallback(participantRecord);
          return;
        }

        
        cursor.continue();
      }).bind(this);
    }).bind(this);
  },

  findParticipantRecordByOtherAddress: function(aParticipantStore, aAddress,
                                                aCreate, aCallback) {
    if (DEBUG) {
      debug("findParticipantRecordByOtherAddress(" +
            JSON.stringify(aAddress) + ", " + aCreate + ")");
    }

    
    let request = aParticipantStore.index("addresses").get(aAddress);
    request.onsuccess = (function(event) {
      let participantRecord = event.target.result;
      if (participantRecord) {
        if (DEBUG) {
          debug("findParticipantRecordByOtherAddress: got "
                + JSON.stringify(participantRecord));
        }
        aCallback(participantRecord);
        return;
      }
      if (aCreate) {
        this.createParticipantRecord(aParticipantStore, [aAddress], aCallback);
        return;
      }
      aCallback(null);
    }).bind(this);
  },

  findParticipantRecordByTypedAddress: function(aParticipantStore,
                                                aTypedAddress, aCreate,
                                                aCallback) {
    if (aTypedAddress.type == "PLMN") {
      this.findParticipantRecordByPlmnAddress(aParticipantStore,
                                              aTypedAddress.address, aCreate,
                                              aCallback);
    } else {
      this.findParticipantRecordByOtherAddress(aParticipantStore,
                                               aTypedAddress.address, aCreate,
                                               aCallback);
    }
  },

  
  findParticipantIdsByPlmnAddresses: function(aParticipantStore, aAddresses,
                                              aCreate, aSkipNonexistent, aCallback) {
    if (DEBUG) {
      debug("findParticipantIdsByPlmnAddresses("
            + JSON.stringify(aAddresses) + ", "
            + aCreate + ", " + aSkipNonexistent + ")");
    }

    if (!aAddresses || !aAddresses.length) {
      if (DEBUG) debug("findParticipantIdsByPlmnAddresses: returning null");
      aCallback(null);
      return;
    }

    let self = this;
    (function findParticipantId(index, result) {
      if (index >= aAddresses.length) {
        
        result.sort(function(a, b) {
          return a - b;
        });
        if (DEBUG) debug("findParticipantIdsByPlmnAddresses: returning " + result);
        aCallback(result);
        return;
      }

      self.findParticipantRecordByPlmnAddress(aParticipantStore,
                                              aAddresses[index++], aCreate,
                                              function(participantRecord) {
        if (!participantRecord) {
          if (!aSkipNonexistent) {
            if (DEBUG) debug("findParticipantIdsByPlmnAddresses: returning null");
            aCallback(null);
            return;
          }
        } else if (result.indexOf(participantRecord.id) < 0) {
          result.push(participantRecord.id);
        }
        findParticipantId(index, result);
      });
    }) (0, []);
  },

  findParticipantIdsByTypedAddresses: function(aParticipantStore,
                                               aTypedAddresses, aCreate,
                                               aSkipNonexistent, aCallback) {
    if (DEBUG) {
      debug("findParticipantIdsByTypedAddresses(" +
            JSON.stringify(aTypedAddresses) + ", " +
            aCreate + ", " + aSkipNonexistent + ")");
    }

    if (!aTypedAddresses || !aTypedAddresses.length) {
      if (DEBUG) debug("findParticipantIdsByTypedAddresses: returning null");
      aCallback(null);
      return;
    }

    let self = this;
    (function findParticipantId(index, result) {
      if (index >= aTypedAddresses.length) {
        
        result.sort(function(a, b) {
          return a - b;
        });
        if (DEBUG) {
          debug("findParticipantIdsByTypedAddresses: returning " + result);
        }
        aCallback(result);
        return;
      }

      self.findParticipantRecordByTypedAddress(aParticipantStore,
                                               aTypedAddresses[index++],
                                               aCreate,
                                               function(participantRecord) {
        if (!participantRecord) {
          if (!aSkipNonexistent) {
            if (DEBUG) {
              debug("findParticipantIdsByTypedAddresses: returning null");
            }
            aCallback(null);
            return;
          }
        } else if (result.indexOf(participantRecord.id) < 0) {
          result.push(participantRecord.id);
        }
        findParticipantId(index, result);
      });
    }) (0, []);
  },

  
  findThreadRecordByPlmnAddresses: function(aThreadStore, aParticipantStore,
                                            aAddresses, aCreateParticipants,
                                            aCallback) {
    if (DEBUG) {
      debug("findThreadRecordByPlmnAddresses(" + JSON.stringify(aAddresses)
            + ", " + aCreateParticipants + ")");
    }
    this.findParticipantIdsByPlmnAddresses(aParticipantStore, aAddresses,
                                           aCreateParticipants, false,
                                           function(participantIds) {
      if (!participantIds) {
        if (DEBUG) debug("findThreadRecordByPlmnAddresses: returning null");
        aCallback(null, null);
        return;
      }
      
      let request = aThreadStore.index("participantIds").get(participantIds);
      request.onsuccess = function(event) {
        let threadRecord = event.target.result;
        if (DEBUG) {
          debug("findThreadRecordByPlmnAddresses: return "
                + JSON.stringify(threadRecord));
        }
        aCallback(threadRecord, participantIds);
      };
    });
  },

  findThreadRecordByTypedAddresses: function(aThreadStore, aParticipantStore,
                                             aTypedAddresses,
                                             aCreateParticipants, aCallback) {
    if (DEBUG) {
      debug("findThreadRecordByTypedAddresses(" +
          JSON.stringify(aTypedAddresses) + ", " + aCreateParticipants + ")");
    }
    this.findParticipantIdsByTypedAddresses(aParticipantStore, aTypedAddresses,
                                            aCreateParticipants, false,
                                            function(participantIds) {
      if (!participantIds) {
        if (DEBUG) debug("findThreadRecordByTypedAddresses: returning null");
        aCallback(null, null);
        return;
      }
      
      let request = aThreadStore.index("participantIds").get(participantIds);
      request.onsuccess = function(event) {
        let threadRecord = event.target.result;
        if (DEBUG) {
          debug("findThreadRecordByTypedAddresses: return " +
                JSON.stringify(threadRecord));
        }
        aCallback(threadRecord, participantIds);
      };
    });
  },

  newTxnWithCallback: function(aCallback, aFunc, aStoreNames) {
    let self = this;
    this.newTxn(READ_WRITE, function(aError, aTransaction, aStores) {
      let notifyResult = function(aRv, aMessageRecord) {
        if (!aCallback) {
          return;
        }
        let domMessage =
          aMessageRecord && self.createDomMessageFromRecord(aMessageRecord);
        aCallback.notify(aRv, domMessage);
      };

      if (aError) {
        notifyResult(aError, null);
        return;
      }

      let capture = {};
      aTransaction.oncomplete = function(event) {
        notifyResult(Cr.NS_OK, capture.messageRecord);
      };
      aTransaction.onabort = function(event) {
        if (DEBUG) debug("transaction abort due to " + event.target.error.name);
        let error = (event.target.error.name === 'QuotaExceededError')
                    ? Cr.NS_ERROR_FILE_NO_DEVICE_SPACE
                    : Cr.NS_ERROR_FAILURE;
        notifyResult(error, null);
      };

      aFunc(capture, aStores);
    }, aStoreNames);
  },

  saveRecord: function(aMessageRecord, aThreadParticipants, aCallback) {
    if (DEBUG) debug("Going to store " + JSON.stringify(aMessageRecord));

    let self = this;
    this.newTxn(READ_WRITE, function(error, txn, stores) {
      let notifyResult = function(aRv, aMessageRecord) {
        if (!aCallback) {
          return;
        }
        let domMessage =
          aMessageRecord && self.createDomMessageFromRecord(aMessageRecord);
        aCallback.notify(aRv, domMessage);
      };

      if (error) {
        notifyResult(error, null);
        return;
      }

      let deletedInfo = { messageIds: [], threadIds: [] };

      txn.oncomplete = function(event) {
        if (aMessageRecord.id > self.lastMessageId) {
          self.lastMessageId = aMessageRecord.id;
        }
        notifyResult(Cr.NS_OK, aMessageRecord);
        self.notifyDeletedInfo(deletedInfo);
      };
      txn.onabort = function(event) {
        if (DEBUG) debug("transaction abort due to " + event.target.error.name);
        let error = (event.target.error.name === 'QuotaExceededError')
                    ? Cr.NS_ERROR_FILE_NO_DEVICE_SPACE
                    : Cr.NS_ERROR_FAILURE;
        notifyResult(error, null);
      };

      let messageStore = stores[0];
      let participantStore = stores[1];
      let threadStore = stores[2];
      self.replaceShortMessageOnSave(txn, messageStore, participantStore,
                                     threadStore, aMessageRecord,
                                     aThreadParticipants, deletedInfo);
    }, [MESSAGE_STORE_NAME, PARTICIPANT_STORE_NAME, THREAD_STORE_NAME]);
  },

  replaceShortMessageOnSave: function(aTransaction, aMessageStore,
                                      aParticipantStore, aThreadStore,
                                      aMessageRecord, aThreadParticipants,
                                      aDeletedInfo) {
    let isReplaceTypePid = (aMessageRecord.pid) &&
                           ((aMessageRecord.pid >= RIL.PDU_PID_REPLACE_SHORT_MESSAGE_TYPE_1 &&
                             aMessageRecord.pid <= RIL.PDU_PID_REPLACE_SHORT_MESSAGE_TYPE_7) ||
                            aMessageRecord.pid == RIL.PDU_PID_RETURN_CALL_MESSAGE);

    if (aMessageRecord.type != "sms" ||
        aMessageRecord.delivery != DELIVERY_RECEIVED ||
        !isReplaceTypePid) {
      this.realSaveRecord(aTransaction, aMessageStore, aParticipantStore,
                          aThreadStore, aMessageRecord, aThreadParticipants,
                          aDeletedInfo);
      return;
    }

    
    
    
    
    
    
    
    
    let self = this;
    let typedSender = {
      address: aMessageRecord.sender,
      type: MMS.Address.resolveType(aMessageRecord.sender)
    };
    this.findParticipantRecordByTypedAddress(aParticipantStore, typedSender,
                                             false,
                                             function(participantRecord) {
      if (!participantRecord) {
        self.realSaveRecord(aTransaction, aMessageStore, aParticipantStore,
                            aThreadStore, aMessageRecord, aThreadParticipants,
                            aDeletedInfo);
        return;
      }

      let participantId = participantRecord.id;
      let range = IDBKeyRange.bound([participantId, 0], [participantId, ""]);
      let request = aMessageStore.index("participantIds").openCursor(range);
      request.onsuccess = function(event) {
        let cursor = event.target.result;
        if (!cursor) {
          self.realSaveRecord(aTransaction, aMessageStore, aParticipantStore,
                              aThreadStore, aMessageRecord, aThreadParticipants,
                              aDeletedInfo);
          return;
        }

        
        
        let foundMessageRecord = cursor.value;
        if (foundMessageRecord.type != "sms" ||
            foundMessageRecord.sender != aMessageRecord.sender ||
            foundMessageRecord.pid != aMessageRecord.pid) {
          cursor.continue();
          return;
        }

        
        aMessageRecord.id = foundMessageRecord.id;
        self.realSaveRecord(aTransaction, aMessageStore, aParticipantStore,
                            aThreadStore, aMessageRecord, aThreadParticipants,
                            aDeletedInfo);
      };
    });
  },

  realSaveRecord: function(aTransaction, aMessageStore, aParticipantStore,
                           aThreadStore, aMessageRecord, aThreadParticipants,
                           aDeletedInfo) {
    let self = this;
    this.findThreadRecordByTypedAddresses(aThreadStore, aParticipantStore,
                                          aThreadParticipants, true,
                                          function(threadRecord,
                                                   participantIds) {
      if (!participantIds) {
        aTransaction.abort();
        return;
      }

      let isOverriding = (aMessageRecord.id !== undefined);
      if (!isOverriding) {
        
        aMessageRecord.id = self.lastMessageId + 1;
      }

      let timestamp = aMessageRecord.timestamp;
      let insertMessageRecord = function(threadId) {
        
        aMessageRecord.threadId = threadId;
        aMessageRecord.threadIdIndex = [threadId, timestamp];
        
        aMessageRecord.participantIdsIndex = [];
        for each (let id in participantIds) {
          aMessageRecord.participantIdsIndex.push([id, timestamp]);
        }

        if (!isOverriding) {
          
          aMessageStore.put(aMessageRecord);
          return;
        }

        
        
        
        
        aMessageStore.get(aMessageRecord.id).onsuccess = function(event) {
          let oldMessageRecord = event.target.result;
          aMessageStore.put(aMessageRecord);
          if (oldMessageRecord) {
            self.updateThreadByMessageChange(aMessageStore,
                                             aThreadStore,
                                             oldMessageRecord.threadId,
                                             [aMessageRecord.id],
                                             oldMessageRecord.read ? 0 : 1,
                                             aDeletedInfo);
          }
        };
      };

      if (threadRecord) {
        let needsUpdate = false;

        if (threadRecord.lastTimestamp <= timestamp) {
          let lastMessageSubject;
          if (aMessageRecord.type == "mms") {
            lastMessageSubject = aMessageRecord.headers.subject;
          }
          threadRecord.lastMessageSubject = lastMessageSubject || null;
          threadRecord.lastTimestamp = timestamp;
          threadRecord.body = aMessageRecord.body;
          threadRecord.lastMessageId = aMessageRecord.id;
          threadRecord.lastMessageType = aMessageRecord.type;
          needsUpdate = true;
        }

        if (!aMessageRecord.read) {
          threadRecord.unreadCount++;
          needsUpdate = true;
        }

        if (needsUpdate) {
          aThreadStore.put(threadRecord);
        }

        insertMessageRecord(threadRecord.id);
        return;
      }

      let lastMessageSubject;
      if (aMessageRecord.type == "mms") {
        lastMessageSubject = aMessageRecord.headers.subject;
      }

      threadRecord = {
        participantIds: participantIds,
        participantAddresses: aThreadParticipants.map(function(typedAddress) {
          return typedAddress.address;
        }),
        lastMessageId: aMessageRecord.id,
        lastTimestamp: timestamp,
        lastMessageSubject: lastMessageSubject || null,
        body: aMessageRecord.body,
        unreadCount: aMessageRecord.read ? 0 : 1,
        lastMessageType: aMessageRecord.type,
      };
      aThreadStore.add(threadRecord).onsuccess = function(event) {
        let threadId = event.target.result;
        insertMessageRecord(threadId);
      };
    });
  },

  forEachMatchedMmsDeliveryInfo: function(aDeliveryInfo, aNeedle, aCallback) {

    let typedAddress = {
      type: MMS.Address.resolveType(aNeedle),
      address: aNeedle
    };
    let normalizedAddress, parsedAddress;
    if (typedAddress.type === "PLMN") {
      normalizedAddress = PhoneNumberUtils.normalize(aNeedle, false);
      parsedAddress = PhoneNumberUtils.parse(normalizedAddress);
    }

    for (let element of aDeliveryInfo) {
      let typedStoredAddress = {
        type: MMS.Address.resolveType(element.receiver),
        address: element.receiver
      };
      if (typedAddress.type !== typedStoredAddress.type) {
        
        continue;
      }

      if (typedAddress.address == typedStoredAddress.address) {
        
        aCallback(element);
        continue;
      }

      if (typedAddress.type !== "PLMN") {
        
        continue;
      }

      
      let normalizedStoredAddress =
        PhoneNumberUtils.normalize(element.receiver, false);
      let parsedStoredAddress =
        PhoneNumberUtils.parseWithMCC(normalizedStoredAddress, null);
      if (this.matchPhoneNumbers(normalizedAddress, parsedAddress,
                                 normalizedStoredAddress, parsedStoredAddress)) {
        aCallback(element);
      }
    }
  },

  updateMessageDeliveryById: function(id, type, receiver, delivery,
                                      deliveryStatus, envelopeId, callback) {
    if (DEBUG) {
      debug("Setting message's delivery by " + type + " = "+ id
            + " receiver: " + receiver
            + " delivery: " + delivery
            + " deliveryStatus: " + deliveryStatus
            + " envelopeId: " + envelopeId);
    }

    let self = this;
    this.newTxnWithCallback(callback, function(aCapture, aMessageStore) {
      let getRequest;
      if (type === "messageId") {
        getRequest = aMessageStore.get(id);
      } else if (type === "envelopeId") {
        getRequest = aMessageStore.index("envelopeId").get(id);
      }

      getRequest.onsuccess = function(event) {
        let messageRecord = event.target.result;
        if (!messageRecord) {
          if (DEBUG) debug("type = " + id + " is not found");
          throw Cr.NS_ERROR_FAILURE;
        }

        let isRecordUpdated = false;

        
        if (delivery && messageRecord.delivery != delivery) {
          messageRecord.delivery = delivery;
          messageRecord.deliveryIndex = [delivery, messageRecord.timestamp];
          isRecordUpdated = true;

          
          
          
          if (delivery == DELIVERY_SENT) {
            messageRecord.sentTimestamp = Date.now();
          }
        }

        
        
        
        if (deliveryStatus) {
          
          
          let updateFunc = function(aTarget) {
            if (aTarget.deliveryStatus == deliveryStatus) {
              return;
            }

            aTarget.deliveryStatus = deliveryStatus;

            
            if (deliveryStatus == DELIVERY_STATUS_SUCCESS) {
              aTarget.deliveryTimestamp = Date.now();
            }

            isRecordUpdated = true;
          };

          if (messageRecord.type == "sms") {
            updateFunc(messageRecord);
          } else if (messageRecord.type == "mms") {
            if (!receiver) {
              
              
              messageRecord.deliveryInfo.forEach(updateFunc);
            } else {
              self.forEachMatchedMmsDeliveryInfo(messageRecord.deliveryInfo,
                                                 receiver, updateFunc);
            }
          }
        }

        
        if (envelopeId) {
          if (messageRecord.envelopeIdIndex != envelopeId) {
            messageRecord.envelopeIdIndex = envelopeId;
            isRecordUpdated = true;
          }
        }

        aCapture.messageRecord = messageRecord;
        if (!isRecordUpdated) {
          if (DEBUG) {
            debug("The values of delivery, deliveryStatus and envelopeId " +
                  "don't need to be updated.");
          }
          return;
        }

        if (DEBUG) {
          debug("The delivery, deliveryStatus or envelopeId are updated.");
        }
        aMessageStore.put(messageRecord);
      };
    });
  },

  fillReceivedMmsThreadParticipants: function(aMessage, threadParticipants) {
    let receivers = aMessage.receivers;
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (DISABLE_MMS_GROUPING_FOR_RECEIVING || receivers.length < 2) {
      return;
    }
    let isSuccess = false;
    let slicedReceivers = receivers.slice();
    if (aMessage.msisdn) {
      let found = slicedReceivers.indexOf(aMessage.msisdn);
      if (found !== -1) {
        isSuccess = true;
        slicedReceivers.splice(found, 1);
      }
    }

    if (!isSuccess) {
      
      
      
      if (DEBUG) debug("Error! Cannot strip out user's own phone number!");
    }

    threadParticipants =
      threadParticipants.concat(slicedReceivers).map(function(aAddress) {
        return {
          address: aAddress,
          type: MMS.Address.resolveType(aAddress)
        };
      });
  },

  updateThreadByMessageChange: function(messageStore, threadStore, threadId,
                                        removedMsgIds, ignoredUnreadCount, deletedInfo) {
    let self = this;
    threadStore.get(threadId).onsuccess = function(event) {
      
      let threadRecord = event.target.result;
      if (DEBUG) debug("Updating thread record " + JSON.stringify(threadRecord));

      if (ignoredUnreadCount > 0) {
        if (DEBUG) {
          debug("Updating unread count : " + threadRecord.unreadCount +
                " -> " + (threadRecord.unreadCount - ignoredUnreadCount));
        }
        threadRecord.unreadCount -= ignoredUnreadCount;
      }

      if (removedMsgIds.indexOf(threadRecord.lastMessageId) >= 0) {
        if (DEBUG) debug("MRU entry was deleted.");
        
        let range = IDBKeyRange.bound([threadId, 0], [threadId, ""]);
        let request = messageStore.index("threadId")
                                  .openCursor(range, PREV);
        request.onsuccess = function(event) {
          let cursor = event.target.result;
          if (!cursor) {
            if (DEBUG) {
              debug("All messages were deleted. Delete this thread.");
            }
            threadStore.delete(threadId);
            if (deletedInfo) {
              deletedInfo.threadIds.push(threadId);
            }
            return;
          }

          let nextMsg = cursor.value;
          let lastMessageSubject;
          if (nextMsg.type == "mms") {
            lastMessageSubject = nextMsg.headers.subject;
          }
          threadRecord.lastMessageSubject = lastMessageSubject || null;
          threadRecord.lastMessageId = nextMsg.id;
          threadRecord.lastTimestamp = nextMsg.timestamp;
          threadRecord.body = nextMsg.body;
          threadRecord.lastMessageType = nextMsg.type;
          if (DEBUG) {
            debug("Updating mru entry: " +
                  JSON.stringify(threadRecord));
          }
          threadStore.put(threadRecord);
        };
      } else if (ignoredUnreadCount > 0) {
        if (DEBUG) debug("Shortcut, just update the unread count.");
        threadStore.put(threadRecord);
      }
    };
  },

  notifyDeletedInfo: function(info) {
    if (!info ||
        (info.messageIds.length === 0 && info.threadIds.length === 0)) {
      return;
    }

    let deletedInfo =
      gMobileMessageService
      .createDeletedMessageInfo(info.messageIds,
                                info.messageIds.length,
                                info.threadIds,
                                info.threadIds.length);
    Services.obs.notifyObservers(deletedInfo, "sms-deleted", null);
  },

  



  saveReceivedMessage: function(aMessage, aCallback) {
    if ((aMessage.type != "sms" && aMessage.type != "mms") ||
        (aMessage.type == "sms" && (aMessage.messageClass == undefined ||
                                    aMessage.sender == undefined)) ||
        (aMessage.type == "mms" && (aMessage.delivery == undefined ||
                                    aMessage.deliveryStatus == undefined ||
                                    !Array.isArray(aMessage.receivers))) ||
        aMessage.timestamp == undefined) {
      if (aCallback) {
        aCallback.notify(Cr.NS_ERROR_FAILURE, null);
      }
      return;
    }

    let threadParticipants;
    if (aMessage.type == "mms") {
      if (aMessage.headers.from) {
        aMessage.sender = aMessage.headers.from.address;
      } else {
        aMessage.sender = "";
      }

      threadParticipants = [{
        address: aMessage.sender,
        type: MMS.Address.resolveType(aMessage.sender)
      }];
      this.fillReceivedMmsThreadParticipants(aMessage, threadParticipants);
    } else { 
      threadParticipants = [{
        address: aMessage.sender,
        type: MMS.Address.resolveType(aMessage.sender)
      }];
    }

    let timestamp = aMessage.timestamp;

    
    
    aMessage.readIndex = [FILTER_READ_UNREAD, timestamp];
    aMessage.read = FILTER_READ_UNREAD;

    
    if (aMessage.sentTimestamp == undefined) {
      aMessage.sentTimestamp = 0;
    }

    if (aMessage.type == "mms") {
      aMessage.transactionIdIndex = aMessage.headers["x-mms-transaction-id"];
      aMessage.isReadReportSent = false;

      
      
      
      aMessage.deliveryInfo = [{
        receiver: aMessage.phoneNumber,
        deliveryStatus: aMessage.deliveryStatus,
        deliveryTimestamp: 0,
        readStatus: MMS.DOM_READ_STATUS_NOT_APPLICABLE,
        readTimestamp: 0,
      }];

      delete aMessage.deliveryStatus;
    }

    if (aMessage.type == "sms") {
      aMessage.delivery = DELIVERY_RECEIVED;
      aMessage.deliveryStatus = DELIVERY_STATUS_SUCCESS;
      aMessage.deliveryTimestamp = 0;

      if (aMessage.pid == undefined) {
        aMessage.pid = RIL.PDU_PID_DEFAULT;
      }
    }
    aMessage.deliveryIndex = [aMessage.delivery, timestamp];

    this.saveRecord(aMessage, threadParticipants, aCallback);
  },

  saveSendingMessage: function(aMessage, aCallback) {
    if ((aMessage.type != "sms" && aMessage.type != "mms") ||
        (aMessage.type == "sms" && aMessage.receiver == undefined) ||
        (aMessage.type == "mms" && !Array.isArray(aMessage.receivers)) ||
        aMessage.deliveryStatusRequested == undefined ||
        aMessage.timestamp == undefined) {
      if (aCallback) {
        aCallback.notify(Cr.NS_ERROR_FAILURE, null);
      }
      return;
    }

    
    
    let deliveryStatus = aMessage.deliveryStatusRequested
                       ? DELIVERY_STATUS_PENDING
                       : DELIVERY_STATUS_NOT_APPLICABLE;
    if (aMessage.type == "sms") {
      aMessage.deliveryStatus = deliveryStatus;
      
      if (aMessage.deliveryTimestamp == undefined) {
        aMessage.deliveryTimestamp = 0;
      }
    } else if (aMessage.type == "mms") {
      let receivers = aMessage.receivers;
      let readStatus = aMessage.headers["x-mms-read-report"]
                     ? MMS.DOM_READ_STATUS_PENDING
                     : MMS.DOM_READ_STATUS_NOT_APPLICABLE;
      aMessage.deliveryInfo = [];
      for (let i = 0; i < receivers.length; i++) {
        aMessage.deliveryInfo.push({
          receiver: receivers[i],
          deliveryStatus: deliveryStatus,
          deliveryTimestamp: 0,
          readStatus: readStatus,
          readTimestamp: 0,
        });
      }
    }

    let timestamp = aMessage.timestamp;

    
    
    aMessage.deliveryIndex = [DELIVERY_SENDING, timestamp];
    aMessage.readIndex = [FILTER_READ_READ, timestamp];
    aMessage.delivery = DELIVERY_SENDING;
    aMessage.messageClass = MESSAGE_CLASS_NORMAL;
    aMessage.read = FILTER_READ_READ;

    
    aMessage.sentTimestamp = 0;

    let threadParticipants;
    if (aMessage.type == "sms") {
      threadParticipants = [{
        address: aMessage.receiver,
        type :MMS.Address.resolveType(aMessage.receiver)
      }];
    } else if (aMessage.type == "mms") {
      threadParticipants = aMessage.headers.to;
    }
    this.saveRecord(aMessage, threadParticipants, aCallback);
  },

  setMessageDeliveryByMessageId: function(messageId, receiver, delivery,
                                          deliveryStatus, envelopeId, callback) {
    this.updateMessageDeliveryById(messageId, "messageId",
                                   receiver, delivery, deliveryStatus,
                                   envelopeId, callback);

  },

  setMessageDeliveryStatusByEnvelopeId: function(aEnvelopeId, aReceiver,
                                                 aDeliveryStatus, aCallback) {
    this.updateMessageDeliveryById(aEnvelopeId, "envelopeId", aReceiver, null,
                                   aDeliveryStatus, null, aCallback);
  },

  setMessageReadStatusByEnvelopeId: function(aEnvelopeId, aReceiver,
                                             aReadStatus, aCallback) {
    if (DEBUG) {
      debug("Setting message's read status by envelopeId = " + aEnvelopeId +
            ", receiver: " + aReceiver + ", readStatus: " + aReadStatus);
    }

    let self = this;
    this.newTxnWithCallback(aCallback, function(aCapture, aMessageStore) {
      let getRequest = aMessageStore.index("envelopeId").get(aEnvelopeId);
      getRequest.onsuccess = function(event) {
        let messageRecord = event.target.result;
        if (!messageRecord) {
          if (DEBUG) debug("envelopeId '" + aEnvelopeId + "' not found");
          throw Cr.NS_ERROR_FAILURE;
        }

        aCapture.messageRecord = messageRecord;

        let isRecordUpdated = false;
        self.forEachMatchedMmsDeliveryInfo(messageRecord.deliveryInfo,
                                           aReceiver, function(aEntry) {
          if (aEntry.readStatus == aReadStatus) {
            return;
          }

          aEntry.readStatus = aReadStatus;
          if (aReadStatus == MMS.DOM_READ_STATUS_SUCCESS) {
            aEntry.readTimestamp = Date.now();
          } else {
            aEntry.readTimestamp = 0;
          }
          isRecordUpdated = true;
        });

        if (!isRecordUpdated) {
          if (DEBUG) {
            debug("The values of readStatus don't need to be updated.");
          }
          return;
        }

        if (DEBUG) {
          debug("The readStatus is updated.");
        }
        aMessageStore.put(messageRecord);
      };
    });
  },

  getMessageRecordByTransactionId: function(aTransactionId, aCallback) {
    if (DEBUG) debug("Retrieving message with transaction ID " + aTransactionId);
    let self = this;
    this.newTxn(READ_ONLY, function(error, txn, messageStore) {
      if (error) {
        if (DEBUG) debug(error);
        aCallback.notify(error, null, null);
        return;
      }
      let request = messageStore.index("transactionId").get(aTransactionId);

      txn.oncomplete = function(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        let messageRecord = request.result;
        if (!messageRecord) {
          if (DEBUG) debug("Transaction ID " + aTransactionId + " not found");
          aCallback.notify(Cr.NS_ERROR_FILE_NOT_FOUND, null, null);
          return;
        }
        
        
        aCallback.notify(Cr.NS_OK, messageRecord, null);
      };

      txn.onerror = function(event) {
        if (DEBUG) {
          if (event.target) {
            debug("Caught error on transaction", event.target.error.name);
          }
        }
        aCallback.notify(Cr.NS_ERROR_FAILURE, null, null);
      };
    });
  },

  getMessageRecordById: function(aMessageId, aCallback) {
    if (DEBUG) debug("Retrieving message with ID " + aMessageId);
    let self = this;
    this.newTxn(READ_ONLY, function(error, txn, messageStore) {
      if (error) {
        if (DEBUG) debug(error);
        aCallback.notify(error, null, null);
        return;
      }
      let request = messageStore.mozGetAll(aMessageId);

      txn.oncomplete = function() {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (request.result.length > 1) {
          if (DEBUG) debug("Got too many results for id " + aMessageId);
          aCallback.notify(Cr.NS_ERROR_UNEXPECTED, null, null);
          return;
        }
        let messageRecord = request.result[0];
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + aMessageId + " not found");
          aCallback.notify(Cr.NS_ERROR_FILE_NOT_FOUND, null, null);
          return;
        }
        if (messageRecord.id != aMessageId) {
          if (DEBUG) {
            debug("Requested message ID (" + aMessageId + ") is " +
                  "different from the one we got");
          }
          aCallback.notify(Cr.NS_ERROR_UNEXPECTED, null, null);
          return;
        }
        let domMessage = self.createDomMessageFromRecord(messageRecord);
        aCallback.notify(Cr.NS_OK, messageRecord, domMessage);
      };

      txn.onerror = function(event) {
        if (DEBUG) {
          if (event.target) {
            debug("Caught error on transaction", event.target.error.name);
          }
        }
        aCallback.notify(Cr.NS_ERROR_FAILURE, null, null);
      };
    });
  },

  translateCrErrorToMessageCallbackError: function(aCrError) {
    switch(aCrError) {
      case Cr.NS_OK:
        return Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR;
      case Cr.NS_ERROR_UNEXPECTED:
        return Ci.nsIMobileMessageCallback.UNKNOWN_ERROR;
      case Cr.NS_ERROR_FILE_NOT_FOUND:
        return Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR;
      case Cr.NS_ERROR_FILE_NO_DEVICE_SPACE:
        return Ci.nsIMobileMessageCallback.STORAGE_FULL_ERROR;
      default:
        return Ci.nsIMobileMessageCallback.INTERNAL_ERROR;
    }
  },

  saveSmsSegment: function(aSmsSegment, aCallback) {
    let completeMessage = null;
    this.newTxn(READ_WRITE, function(error, txn, segmentStore) {
      if (error) {
        if (DEBUG) debug(error);
        aCallback.notify(error, null);
        return;
      }

      txn.oncomplete = function(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (completeMessage) {
          
          if (completeMessage.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
            
            
            let fullDataLen = 0;
            for (let i = 1; i <= completeMessage.segmentMaxSeq; i++) {
              fullDataLen += completeMessage.segments[i].length;
            }

            completeMessage.fullData = new Uint8Array(fullDataLen);
            for (let d = 0, i = 1; i <= completeMessage.segmentMaxSeq; i++) {
              let data = completeMessage.segments[i];
              for (let j = 0; j < data.length; j++) {
                completeMessage.fullData[d++] = data[j];
              }
            }
          } else {
            completeMessage.fullBody = completeMessage.segments.join("");
          }

          
          delete completeMessage.id;
          delete completeMessage.hash;
          delete completeMessage.receivedSegments;
          delete completeMessage.segments;
        }
        aCallback.notify(Cr.NS_OK, completeMessage);
      };

      txn.onabort = function(event) {
        if (DEBUG) debug("transaction abort due to " + event.target.error.name);
        let error = (event.target.error.name === 'QuotaExceededError')
                    ? Cr.NS_ERROR_FILE_NO_DEVICE_SPACE
                    : Cr.NS_ERROR_FAILURE;
        aCallback.notify(error, null);
      };

      aSmsSegment.hash = aSmsSegment.sender + ":" +
                         aSmsSegment.segmentRef + ":" +
                         aSmsSegment.segmentMaxSeq + ":" +
                         aSmsSegment.iccId;
      let seq = aSmsSegment.segmentSeq;
      if (DEBUG) {
        debug("Saving SMS Segment: " + aSmsSegment.hash + ", seq: " + seq);
      }
      let getRequest = segmentStore.index("hash").get(aSmsSegment.hash);
      getRequest.onsuccess = function(event) {
        let segmentRecord = event.target.result;
        if (!segmentRecord) {
          if (DEBUG) {
            debug("Not found! Create a new record to store the segments.");
          }
          aSmsSegment.receivedSegments = 1;
          aSmsSegment.segments = [];
          if (aSmsSegment.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
            aSmsSegment.segments[seq] = aSmsSegment.data;
          } else {
            aSmsSegment.segments[seq] = aSmsSegment.body;
          }

          segmentStore.add(aSmsSegment);

          return;
        }

        if (DEBUG) {
          debug("Append SMS Segment into existed message object: " + segmentRecord.id);
        }

        if (segmentRecord.segments[seq]) {
          if (DEBUG) debug("Got duplicated segment no. " + seq);
          return;
        }

        segmentRecord.timestamp = aSmsSegment.timestamp;

        if (segmentRecord.encoding == RIL.PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
          segmentRecord.segments[seq] = aSmsSegment.data;
        } else {
          segmentRecord.segments[seq] = aSmsSegment.body;
        }
        segmentRecord.receivedSegments++;

        
        
        
        
        
        if (aSmsSegment.teleservice === RIL.PDU_CDMA_MSG_TELESERIVCIE_ID_WAP
            && seq === 1) {
          if (aSmsSegment.originatorPort) {
            segmentRecord.originatorPort = aSmsSegment.originatorPort;
          }

          if (aSmsSegment.destinationPort) {
            segmentRecord.destinationPort = aSmsSegment.destinationPort;
          }
        }

        if (segmentRecord.receivedSegments < segmentRecord.segmentMaxSeq) {
          if (DEBUG) debug("Message is incomplete.");
          segmentStore.put(segmentRecord);
          return;
        }

        completeMessage = segmentRecord;

        
        segmentStore.delete(segmentRecord.id);
      };
    }, [SMS_SEGMENT_STORE_NAME]);
  },

  



  getMessage: function(aMessageId, aRequest) {
    if (DEBUG) debug("Retrieving message with ID " + aMessageId);
    let self = this;
    let notifyCallback = {
      notify: function(aRv, aMessageRecord, aDomMessage) {
        if (Cr.NS_OK == aRv) {
          aRequest.notifyMessageGot(aDomMessage);
          return;
        }
        aRequest.notifyGetMessageFailed(
          self.translateCrErrorToMessageCallbackError(aRv), null);
      }
    };
    this.getMessageRecordById(aMessageId, notifyCallback);
  },

  deleteMessage: function(messageIds, length, aRequest) {
    if (DEBUG) debug("deleteMessage: message ids " + JSON.stringify(messageIds));
    let deleted = [];
    let self = this;
    this.newTxn(READ_WRITE, function(error, txn, stores) {
      if (error) {
        if (DEBUG) debug("deleteMessage: failed to open transaction");
        aRequest.notifyDeleteMessageFailed(
          self.translateCrErrorToMessageCallbackError(error));
        return;
      }

      let deletedInfo = { messageIds: [], threadIds: [] };

      txn.onabort = function(event) {
        if (DEBUG) debug("transaction abort due to " + event.target.error.name);
        let error = (event.target.error.name === 'QuotaExceededError')
                    ? Ci.nsIMobileMessageCallback.STORAGE_FULL_ERROR
                    : Ci.nsIMobileMessageCallback.INTERNAL_ERROR;
        aRequest.notifyDeleteMessageFailed(error);
      };

      const messageStore = stores[0];
      const threadStore = stores[1];

      txn.oncomplete = function(event) {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        aRequest.notifyMessageDeleted(deleted, length);
        self.notifyDeletedInfo(deletedInfo);
      };

      let threadsToUpdate = {};
      let numOfMessagesToDelete = length;
      let updateThreadInfo = function() {
        for (let threadId in threadsToUpdate) {
          let threadInfo = threadsToUpdate[threadId];
          self.updateThreadByMessageChange(messageStore,
                                           threadStore,
                                           threadInfo.threadId,
                                           threadInfo.removedMsgIds,
                                           threadInfo.ignoredUnreadCount,
                                           deletedInfo);
        }
      };

      for (let i = 0; i < length; i++) {
        let messageId = messageIds[i];
        deleted[i] = false;
        messageStore.get(messageId).onsuccess = function(messageIndex, event) {
          let messageRecord = event.target.result;
          let messageId = messageIds[messageIndex];
          if (messageRecord) {
            if (DEBUG) debug("Deleting message id " + messageId);

            
            messageStore.delete(messageId).onsuccess = function(event) {
              if (DEBUG) debug("Message id " + messageId + " deleted");

              numOfMessagesToDelete--;
              deleted[messageIndex] = true;
              deletedInfo.messageIds.push(messageId);

              
              let threadId = messageRecord.threadId;
              if (!threadsToUpdate[threadId]) {
                threadsToUpdate[threadId] = {
                  threadId: threadId,
                  removedMsgIds: [messageId],
                  ignoredUnreadCount: (!messageRecord.read) ? 1 : 0
                };
              } else {
                let threadInfo = threadsToUpdate[threadId];
                threadInfo.removedMsgIds.push(messageId);
                if (!messageRecord.read) {
                  threadInfo.ignoredUnreadCount++;
                }
              }

              
              
              if (!numOfMessagesToDelete) {
                updateThreadInfo();
              }
            };
          } else {
            if (DEBUG) debug("Message id " + messageId + " does not exist");

            numOfMessagesToDelete--;
            if (!numOfMessagesToDelete) {
              updateThreadInfo();
            }
          }
        }.bind(null, i);
      }
    }, [MESSAGE_STORE_NAME, THREAD_STORE_NAME]);
  },

  createMessageCursor: function(aHasStartDate, aStartDate, aHasEndDate,
                                aEndDate, aNumbers, aNumbersCount, aDelivery,
                                aHasRead, aRead, aThreadId, aReverse, aCallback) {
    if (DEBUG) {
      debug("Creating a message cursor. Filters:" +
            " startDate: " + (aHasStartDate ? aStartDate : "(null)") +
            " endDate: " + (aHasEndDate ? aEndDate : "(null)") +
            " delivery: " + aDelivery +
            " numbers: " + (aNumbersCount ? aNumbers : "(null)") +
            " read: " + (aHasRead ? aRead : "(null)") +
            " threadId: " + aThreadId +
            " reverse: " + aReverse);
    }

    let filter = {};
    if (aHasStartDate) {
      filter.startDate = aStartDate;
    }
    if (aHasEndDate) {
      filter.endDate = aEndDate;
    }
    if (aNumbersCount) {
      filter.numbers = aNumbers.slice();
    }
    if (aDelivery !== null) {
      filter.delivery = aDelivery;
    }
    if (aHasRead) {
      filter.read = aRead;
    }
    if (aThreadId) {
      filter.threadId = aThreadId;
    }

    let cursor = new GetMessagesCursor(this, aCallback);

    let self = this;
    self.newTxn(READ_ONLY, function(error, txn, stores) {
      let collector = cursor.collector.idCollector;
      let collect = collector.collect.bind(collector);
      FilterSearcherHelper.transact(self, txn, error, filter, aReverse, collect);
    }, [MESSAGE_STORE_NAME, PARTICIPANT_STORE_NAME]);

    return cursor;
  },

  markMessageRead: function(messageId, value, aSendReadReport, aRequest) {
    if (DEBUG) debug("Setting message " + messageId + " read to " + value);
    let self = this;
    this.newTxn(READ_WRITE, function(error, txn, stores) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyMarkMessageReadFailed(
          self.translateCrErrorToMessageCallbackError(error));
        return;
      }

      txn.onabort = function(event) {
        if (DEBUG) debug("transaction abort due to " + event.target.error.name);
        let error = (event.target.error.name === 'QuotaExceededError')
                    ? Ci.nsIMobileMessageCallback.STORAGE_FULL_ERROR
                    : Ci.nsIMobileMessageCallback.INTERNAL_ERROR;
        aRequest.notifyMarkMessageReadFailed(error);
      };

      let messageStore = stores[0];
      let threadStore = stores[1];
      messageStore.get(messageId).onsuccess = function(event) {
        let messageRecord = event.target.result;
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + messageId + " not found");
          aRequest.notifyMarkMessageReadFailed(Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR);
          return;
        }

        if (messageRecord.id != messageId) {
          if (DEBUG) {
            debug("Retrieve message ID (" + messageId + ") is " +
                  "different from the one we got");
          }
          aRequest.notifyMarkMessageReadFailed(Ci.nsIMobileMessageCallback.UNKNOWN_ERROR);
          return;
        }

        
        
        if (messageRecord.read == value) {
          if (DEBUG) debug("The value of messageRecord.read is already " + value);
          aRequest.notifyMessageMarkedRead(messageRecord.read);
          return;
        }

        messageRecord.read = value ? FILTER_READ_READ : FILTER_READ_UNREAD;
        messageRecord.readIndex = [messageRecord.read, messageRecord.timestamp];
        let readReportMessageId, readReportTo;
        if (aSendReadReport &&
            messageRecord.type == "mms" &&
            messageRecord.delivery == DELIVERY_RECEIVED &&
            messageRecord.read == FILTER_READ_READ &&
            messageRecord.headers["x-mms-read-report"] &&
            !messageRecord.isReadReportSent) {
          messageRecord.isReadReportSent = true;

          let from = messageRecord.headers["from"];
          readReportTo = from && from.address;
          readReportMessageId = messageRecord.headers["message-id"];
        }

        if (DEBUG) debug("Message.read set to: " + value);
        messageStore.put(messageRecord).onsuccess = function(event) {
          if (DEBUG) {
            debug("Update successfully completed. Message: " +
                  JSON.stringify(event.target.result));
          }

          
          let threadId = messageRecord.threadId;

          threadStore.get(threadId).onsuccess = function(event) {
            let threadRecord = event.target.result;
            threadRecord.unreadCount += value ? -1 : 1;
            if (DEBUG) {
              debug("Updating unreadCount for thread id " + threadId + ": " +
                    (value ?
                     threadRecord.unreadCount + 1 :
                     threadRecord.unreadCount - 1) +
                     " -> " + threadRecord.unreadCount);
            }
            threadStore.put(threadRecord).onsuccess = function(event) {
              if(readReportMessageId && readReportTo) {
                gMMSService.sendReadReport(readReportMessageId,
                                           readReportTo,
                                           messageRecord.iccId);
              }
              aRequest.notifyMessageMarkedRead(messageRecord.read);
            };
          };
        };
      };
    }, [MESSAGE_STORE_NAME, THREAD_STORE_NAME]);
  },

  createThreadCursor: function(callback) {
    if (DEBUG) debug("Getting thread list");

    let cursor = new GetThreadsCursor(this, callback);
    this.newTxn(READ_ONLY, function(error, txn, threadStore) {
      let collector = cursor.collector.idCollector;
      if (error) {
        collector.collect(null, COLLECT_ID_ERROR, COLLECT_TIMESTAMP_UNUSED);
        return;
      }
      txn.onerror = function(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.error.name);
        collector.collect(null, COLLECT_ID_ERROR, COLLECT_TIMESTAMP_UNUSED);
      };
      let request = threadStore.index("lastTimestamp").openKeyCursor(null, PREV);
      request.onsuccess = function(event) {
        let cursor = event.target.result;
        if (cursor) {
          if (collector.collect(txn, cursor.primaryKey, cursor.key)) {
            cursor.continue();
          }
        } else {
          collector.collect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);
        }
      };
    }, [THREAD_STORE_NAME]);

    return cursor;
  }
};

let FilterSearcherHelper = {

  












  filterIndex: function(index, range, direction, txn, collect) {
    let messageStore = txn.objectStore(MESSAGE_STORE_NAME);
    let request = messageStore.index(index).openKeyCursor(range, direction);
    request.onsuccess = function(event) {
      let cursor = event.target.result;
      
      
      if (cursor) {
        let timestamp = Array.isArray(cursor.key) ? cursor.key[1] : cursor.key;
        if (collect(txn, cursor.primaryKey, timestamp)) {
          cursor.continue();
        }
      } else {
        collect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);
      }
    };
    request.onerror = function(event) {
      if (DEBUG && event) debug("IDBRequest error " + event.target.error.name);
      collect(txn, COLLECT_ID_ERROR, COLLECT_TIMESTAMP_UNUSED);
    };
  },

  














  filterTimestamp: function(startDate, endDate, direction, txn, collect) {
    let range = null;
    if (startDate != null && endDate != null) {
      range = IDBKeyRange.bound(startDate, endDate);
    } else if (startDate != null) {
      range = IDBKeyRange.lowerBound(startDate);
    } else if (endDate != null) {
      range = IDBKeyRange.upperBound(endDate);
    }
    this.filterIndex("timestamp", range, direction, txn, collect);
  },

  

















  transact: function(mmdb, txn, error, filter, reverse, collect) {
    if (error) {
      
      if (DEBUG) debug("IDBRequest error " + event.target.error.name);
      collect(txn, COLLECT_ID_ERROR, COLLECT_TIMESTAMP_UNUSED);
      return;
    }

    let direction = reverse ? PREV : NEXT;

    
    
    if (filter.delivery == null &&
        filter.numbers == null &&
        filter.read == null &&
        filter.threadId == null) {
      
      if (DEBUG) {
        debug("filter.timestamp " + filter.startDate + ", " + filter.endDate);
      }

      this.filterTimestamp(filter.startDate, filter.endDate, direction, txn,
                           collect);
      return;
    }

    
    
    let startDate = 0, endDate = "";
    if (filter.startDate != null) {
      startDate = filter.startDate;
    }
    if (filter.endDate != null) {
      endDate = filter.endDate;
    }

    let single, intersectionCollector;
    {
      let num = 0;
      if (filter.delivery) num++;
      if (filter.numbers) num++;
      if (filter.read != undefined) num++;
      if (filter.threadId != undefined) num++;
      single = (num == 1);
    }

    if (!single) {
      intersectionCollector = new IntersectionResultsCollector(collect, reverse);
    }

    
    
    if (filter.delivery) {
      if (DEBUG) debug("filter.delivery " + filter.delivery);
      let delivery = filter.delivery;
      let range = IDBKeyRange.bound([delivery, startDate], [delivery, endDate]);
      this.filterIndex("delivery", range, direction, txn,
                       single ? collect : intersectionCollector.newContext());
    }

    
    
    if (filter.read != undefined) {
      if (DEBUG) debug("filter.read " + filter.read);
      let read = filter.read ? FILTER_READ_READ : FILTER_READ_UNREAD;
      let range = IDBKeyRange.bound([read, startDate], [read, endDate]);
      this.filterIndex("read", range, direction, txn,
                       single ? collect : intersectionCollector.newContext());
    }

    
    
    if (filter.threadId != undefined) {
      if (DEBUG) debug("filter.threadId " + filter.threadId);
      let threadId = filter.threadId;
      let range = IDBKeyRange.bound([threadId, startDate], [threadId, endDate]);
      this.filterIndex("threadId", range, direction, txn,
                       single ? collect : intersectionCollector.newContext());
    }

    
    
    if (filter.numbers) {
      if (DEBUG) debug("filter.numbers " + filter.numbers.join(", "));

      if (!single) {
        collect = intersectionCollector.newContext();
      }

      let participantStore = txn.objectStore(PARTICIPANT_STORE_NAME);
      let typedAddresses = filter.numbers.map(function(number) {
        return {
          address: number,
          type: MMS.Address.resolveType(number)
        };
      });
      mmdb.findParticipantIdsByTypedAddresses(participantStore, typedAddresses,
                                              false, true,
                                              (function(participantIds) {
        if (!participantIds || !participantIds.length) {
          

          collect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);
          return;
        }

        if (participantIds.length == 1) {
          let id = participantIds[0];
          let range = IDBKeyRange.bound([id, startDate], [id, endDate]);
          this.filterIndex("participantIds", range, direction, txn, collect);
          return;
        }

        let unionCollector = new UnionResultsCollector(collect);

        this.filterTimestamp(filter.startDate, filter.endDate, direction, txn,
                             unionCollector.newTimestampContext());

        for (let i = 0; i < participantIds.length; i++) {
          let id = participantIds[i];
          let range = IDBKeyRange.bound([id, startDate], [id, endDate]);
          this.filterIndex("participantIds", range, direction, txn,
                           unionCollector.newContext());
        }
      }).bind(this));
    }
  }
};



















































function ResultsCollector(readAheadFunc) {
  this.idCollector = new IDsCollector();
  this.results = [];
  this.readAhead = readAheadFunc;

  this.maxReadAhead = DEFAULT_READ_AHEAD_ENTRIES;
  try {
    
    
    
    this.maxReadAhead =
      Services.prefs.getIntPref("dom.sms.maxReadAheadEntries");
  } catch (e) {}
}
ResultsCollector.prototype = {
  


  idCollector: null,

  



  results: null,

  






  readAhead: null,

  



  readingAhead: false,

  


  maxReadAhead: 0,

  


  activeTxn: null,

  


  requestWaiting: null,

  



  done: false,

  


  lastId: null,

  









  collect: function(txn, id) {
    if (this.done) {
      
      
      return;
    }

    if (DEBUG) debug("ResultsCollector::collect ID = " + id);

    
    
    txn = txn || this.activeTxn;

    if (id > 0) {
      this.readingAhead = true;
      this.readAhead(txn, id, this);
    } else {
      this.notifyResult(txn, id, null);
    }
  },

  















  notifyResult: function(txn, id, result) {
    if (DEBUG) debug("notifyResult(txn, " + id + ", <result>)");

    this.readingAhead = false;

    if (id > 0) {
      if (result != null) {
        this.results.push(result);
      } else {
        id = COLLECT_ID_ERROR;
      }
    }

    if (id <= 0) {
      this.lastId = id;
      this.done = true;
    }

    if (!this.requestWaiting) {
      if (DEBUG) debug("notifyResult: cursor.continue() not called yet");
    } else {
      let callback = this.requestWaiting;
      this.requestWaiting = null;

      this.drip(callback);
    }

    this.maybeSqueezeIdCollector(txn);
  },

  





  maybeSqueezeIdCollector: function(txn) {
    if (this.done || 
        this.readingAhead || 
        this.idCollector.requestWaiting) { 
      return;
    }

    let max = this.maxReadAhead;
    if (!max && this.requestWaiting) {
      
      max = 1;
    }
    if (max >= 0 && this.results.length >= max) {
      
      if (DEBUG) debug("maybeSqueezeIdCollector: max " + max + " entries read. Stop.");
      return;
    }

    
    
    this.activeTxn = txn;
    this.idCollector.squeeze(this.collect.bind(this));
    this.activeTxn = null;
  },

  





  squeeze: function(callback) {
    if (this.requestWaiting) {
      throw new Error("Already waiting for another request!");
    }

    if (this.results.length || this.done) {
      
      
      
      this.drip(callback);
    } else {
      this.requestWaiting = callback;
    }

    
    
    
    
    
    this.maybeSqueezeIdCollector(null);
  },

  





  drip: function(callback) {
    let results = this.results;
    this.results = [];

    let func = this.notifyCallback.bind(this, callback, results, this.lastId);
    Services.tm.currentThread.dispatch(func, Ci.nsIThread.DISPATCH_NORMAL);
  },

  












  notifyCallback: function(callback, results, lastId) {
    if (DEBUG) {
      debug("notifyCallback(results[" + results.length + "], " + lastId + ")");
    }

    if (results.length) {
      callback.notifyCursorResult(results, results.length);
    } else if (lastId == COLLECT_ID_END) {
      callback.notifyCursorDone();
    } else {
      callback.notifyCursorError(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
    }
  }
};

function IDsCollector() {
  this.results = [];
  this.done = false;
}
IDsCollector.prototype = {
  results: null,
  requestWaiting: null,
  done: null,

  













  collect: function(txn, id, timestamp) {
    if (this.done) {
      return false;
    }

    if (DEBUG) debug("IDsCollector::collect ID = " + id);
    
    this.results.push(id);
    if (id <= 0) {
      
      this.done = true;
    }

    if (!this.requestWaiting) {
      if (DEBUG) debug("IDsCollector::squeeze() not called yet");
      return !this.done;
    }

    
    
    
    
    let callback = this.requestWaiting;
    this.requestWaiting = null;

    this.drip(txn, callback);

    return !this.done;
  },

  






  squeeze: function(callback) {
    if (this.requestWaiting) {
      throw new Error("Already waiting for another request!");
    }

    if (!this.done) {
      
      
      this.requestWaiting = callback;
      return;
    }

    this.drip(null, callback);
  },

  





  drip: function(txn, callback) {
    let firstId = this.results[0];
    if (firstId > 0) {
      this.results.shift();
    }
    callback(txn, firstId);
  }
};

function IntersectionResultsCollector(collect, reverse) {
  this.cascadedCollect = collect;
  this.reverse = reverse;
  this.contexts = [];
}
IntersectionResultsCollector.prototype = {
  cascadedCollect: null,
  reverse: false,
  contexts: null,

  



  collect: function(contextIndex, txn, id, timestamp) {
    if (DEBUG) {
      debug("IntersectionResultsCollector: "
            + contextIndex + ", " + id + ", " + timestamp);
    }

    let contexts = this.contexts;
    let context = contexts[contextIndex];

    if (id < 0) {
      
      id = 0;
    }
    if (!id) {
      context.done = true;

      if (!context.results.length) {
        
        return this.cascadedCollect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);
      }

      for (let i = 0; i < contexts.length; i++) {
        if (!contexts[i].done) {
          
          
          return false;
        }
      }

      
      return this.cascadedCollect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);
    }

    
    
    
    
    
    for (let i = 0; i < contexts.length; i++) {
      if (i == contextIndex) {
        continue;
      }

      let ctx = contexts[i];
      let results = ctx.results;
      let found = false;
      for (let j = 0; j < results.length; j++) {
        let result = results[j];
        if (result.id == id) {
          found = true;
          break;
        }
        if ((!this.reverse && (result.timestamp > timestamp)) ||
            (this.reverse && (result.timestamp < timestamp))) {
          
          return true;
        }
      }

      if (!found) {
        if (ctx.done) {
          
          if (results.length) {
            let lastResult = results[results.length - 1];
            if ((!this.reverse && (lastResult.timestamp >= timestamp)) ||
                (this.reverse && (lastResult.timestamp <= timestamp))) {
              
              return true;
            }
          }

          
          
          context.done = true;
          return this.cascadedCollect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);
        }

        
        context.results.push({
          id: id,
          timestamp: timestamp
        });
        return true;
      }
    }

    
    return this.cascadedCollect(txn, id, timestamp);
  },

  newContext: function() {
    let contextIndex = this.contexts.length;
    this.contexts.push({
      results: [],
      done: false
    });
    return this.collect.bind(this, contextIndex);
  }
};

function UnionResultsCollector(collect) {
  this.cascadedCollect = collect;
  this.contexts = [{
    
    processing: 1,
    results: []
  }, {
    processing: 0,
    results: []
  }];
}
UnionResultsCollector.prototype = {
  cascadedCollect: null,
  contexts: null,

  collect: function(contextIndex, txn, id, timestamp) {
    if (DEBUG) {
      debug("UnionResultsCollector: "
            + contextIndex + ", " + id + ", " + timestamp);
    }

    let contexts = this.contexts;
    let context = contexts[contextIndex];

    if (id < 0) {
      
      id = 0;
    }
    if (id) {
      if (!contextIndex) {
        
        context.results.push({
          id: id,
          timestamp: timestamp
        });
      } else {
        context.results.push(id);
      }
      return true;
    }

    context.processing -= 1;
    if (contexts[0].processing || contexts[1].processing) {
      
      
      
      return false;
    }

    let tres = contexts[0].results;
    let qres = contexts[1].results;
    tres = tres.filter(function(element) {
      return qres.indexOf(element.id) != -1;
    });

    for (let i = 0; i < tres.length; i++) {
      this.cascadedCollect(txn, tres[i].id, tres[i].timestamp);
    }
    this.cascadedCollect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);

    return false;
  },

  newTimestampContext: function() {
    return this.collect.bind(this, 0);
  },

  newContext: function() {
    this.contexts[1].processing++;
    return this.collect.bind(this, 1);
  }
};

function GetMessagesCursor(mmdb, callback) {
  this.mmdb = mmdb;
  this.callback = callback;
  this.collector = new ResultsCollector(this.getMessage.bind(this));

  this.handleContinue(); 
}
GetMessagesCursor.prototype = {
  classID: RIL_GETMESSAGESCURSOR_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICursorContinueCallback]),

  mmdb: null,
  callback: null,
  collector: null,

  getMessageTxn: function(txn, messageStore, messageId, collector) {
    if (DEBUG) debug ("Fetching message " + messageId);

    let getRequest = messageStore.get(messageId);
    let self = this;
    getRequest.onsuccess = function(event) {
      if (DEBUG) {
        debug("notifyNextMessageInListGot - messageId: " + messageId);
      }
      let domMessage =
        self.mmdb.createDomMessageFromRecord(event.target.result);
      collector.notifyResult(txn, messageId, domMessage);
    };
    getRequest.onerror = function(event) {
      
      event.stopPropagation();
      event.preventDefault();

      if (DEBUG) {
        debug("notifyCursorError - messageId: " + messageId);
      }
      collector.notifyResult(txn, messageId, null);
    };
  },

  getMessage: function(txn, messageId, collector) {
    
    
    if (txn) {
      let messageStore = txn.objectStore(MESSAGE_STORE_NAME);
      this.getMessageTxn(txn, messageStore, messageId, collector);
      return;
    }

    
    let self = this;
    this.mmdb.newTxn(READ_ONLY, function(error, txn, messageStore) {
      if (error) {
        debug("getMessage: failed to create new transaction");
        collector.notifyResult(null, messageId, null);
      } else {
        self.getMessageTxn(txn, messageStore, messageId, collector);
      }
    }, [MESSAGE_STORE_NAME]);
  },

  

  handleContinue: function() {
    if (DEBUG) debug("Getting next message in list");
    this.collector.squeeze(this.callback);
  }
};

function GetThreadsCursor(mmdb, callback) {
  this.mmdb = mmdb;
  this.callback = callback;
  this.collector = new ResultsCollector(this.getThread.bind(this));

  this.handleContinue(); 
}
GetThreadsCursor.prototype = {
  classID: RIL_GETTHREADSCURSOR_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICursorContinueCallback]),

  mmdb: null,
  callback: null,
  collector: null,

  getThreadTxn: function(txn, threadStore, threadId, collector) {
    if (DEBUG) debug ("Fetching thread " + threadId);

    let getRequest = threadStore.get(threadId);
    getRequest.onsuccess = function(event) {
      let threadRecord = event.target.result;
      if (DEBUG) {
        debug("notifyCursorResult: " + JSON.stringify(threadRecord));
      }
      let thread =
        gMobileMessageService.createThread(threadRecord.id,
                                           threadRecord.participantAddresses,
                                           threadRecord.lastTimestamp,
                                           threadRecord.lastMessageSubject || "",
                                           threadRecord.body,
                                           threadRecord.unreadCount,
                                           threadRecord.lastMessageType);
      collector.notifyResult(txn, threadId, thread);
    };
    getRequest.onerror = function(event) {
      
      event.stopPropagation();
      event.preventDefault();

      if (DEBUG) {
        debug("notifyCursorError - threadId: " + threadId);
      }
      collector.notifyResult(txn, threadId, null);
    };
  },

  getThread: function(txn, threadId, collector) {
    
    
    if (txn) {
      let threadStore = txn.objectStore(THREAD_STORE_NAME);
      this.getThreadTxn(txn, threadStore, threadId, collector);
      return;
    }

    
    let self = this;
    this.mmdb.newTxn(READ_ONLY, function(error, txn, threadStore) {
      if (error) {
        collector.notifyResult(null, threadId, null);
      } else {
        self.getThreadTxn(txn, threadStore, threadId, collector);
      }
    }, [THREAD_STORE_NAME]);
  },

  

  handleContinue: function() {
    if (DEBUG) debug("Getting next thread in list");
    this.collector.squeeze(this.callback);
  }
}

this.EXPORTED_SYMBOLS = [
  'MobileMessageDB'
];

function debug() {
  dump("MobileMessageDB: " + Array.slice(arguments).join(" ") + "\n");
}
