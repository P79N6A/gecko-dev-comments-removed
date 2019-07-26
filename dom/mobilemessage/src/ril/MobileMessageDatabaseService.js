



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");

const RIL_MOBILEMESSAGEDATABASESERVICE_CONTRACTID =
  "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1";
const RIL_MOBILEMESSAGEDATABASESERVICE_CID =
  Components.ID("{29785f90-6b5b-11e2-9201-3b280170b2ec}");
const RIL_GETMESSAGESCURSOR_CID =
  Components.ID("{484d1ad8-840e-4782-9dc4-9ebc4d914937}");

const DEBUG = false;
const DB_NAME = "sms";
const DB_VERSION = 8;
const MESSAGE_STORE_NAME = "sms";
const THREAD_STORE_NAME = "thread";
const PARTICIPANT_STORE_NAME = "participant";
const MOST_RECENT_STORE_NAME = "most-recent";

const DELIVERY_SENDING = "sending";
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

const COLLECT_ID_END = 0;
const COLLECT_ID_ERROR = -1;
const COLLECT_TIMESTAMP_UNUSED = 0;

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gIDBManager",
                                   "@mozilla.org/dom/indexeddb/manager;1",
                                   "nsIIndexedDatabaseManager");

const GLOBAL_SCOPE = this;




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
}
MobileMessageDatabaseService.prototype = {

  classID: RIL_MOBILEMESSAGEDATABASESERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRilMobileMessageDatabaseService,
                                         Ci.nsIMobileMessageDatabaseService,
                                         Ci.nsIObserver]),

  


  db: null,

  


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
          case 7:
            if (DEBUG) debug("Upgrade to version 8. Add participant/thread stores.");
            self.upgradeSchema7(db, event.target.transaction);
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

  















  upgradeSchema7: function upgradeSchema7(db, transaction) {
    







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
        return;
      }

      let mostRecentRecord = mostRecentCursor.value;

      
      
      
      let number = mostRecentRecord.senderOrReceiver;
      self.findParticipantRecordByAddress(participantStore, number, true,
                                          function (participantRecord) {
        
        let threadRecord = {
          participantIds: [participantRecord.id],
          participantAddresses: [number],
          lastMessageId: mostRecentRecord.id,
          lastTimestamp: mostRecentRecord.timestamp,
          subject: mostRecentRecord.body,
          unreadCount: mostRecentRecord.unreadCount,
        };
        let addThreadRequest = threadStore.add(threadRecord);
        addThreadRequest.onsuccess = function (event) {
          threadRecord.id = event.target.result;

          let numberRange = IDBKeyRange.bound([number, 0], [number, ""]);
          let messageRequest = messageStore.index("number")
                                           .openCursor(numberRange, NEXT);
          messageRequest.onsuccess = function (event) {
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
          messageRequest.onerror = function () {
            
            mostRecentCursor.continue();
          };
        };
        addThreadRequest.onerror = function () {
          
          mostRecentCursor.continue();
        };
      });
    };
  },

  createDomMessageFromRecord: function createDomMessageFromRecord(aMessageRecord) {
    if (DEBUG) {
      debug("createDomMessageFromRecord: " + JSON.stringify(aMessageRecord));
    }
    if (aMessageRecord.type == "sms") {
      return gMobileMessageService.createSmsMessage(aMessageRecord.id,
                                                    aMessageRecord.threadId,
                                                    aMessageRecord.delivery,
                                                    aMessageRecord.deliveryStatus,
                                                    aMessageRecord.sender,
                                                    aMessageRecord.receiver,
                                                    aMessageRecord.body,
                                                    aMessageRecord.messageClass,
                                                    aMessageRecord.timestamp,
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
      return gMobileMessageService.createMmsMessage(aMessageRecord.id,
                                                    aMessageRecord.threadId,
                                                    aMessageRecord.delivery,
                                                    aMessageRecord.deliveryStatus,
                                                    aMessageRecord.sender,
                                                    aMessageRecord.receivers,
                                                    aMessageRecord.timestamp,
                                                    aMessageRecord.read,
                                                    subject,
                                                    smil,
                                                    attachments);
    }
  },

  findParticipantRecordByAddress: function findParticipantRecordByAddress(
      aParticipantStore, aAddress, aCreate, aCallback) {
    if (DEBUG) {
      debug("findParticipantRecordByAddress("
            + JSON.stringify(aAddress) + ", " + aCreate + ")");
    }

    
    
    

    let request = aParticipantStore.index("addresses").get(aAddress);
    request.onsuccess = (function (event) {
      let participantRecord = event.target.result;
      
      
      if (participantRecord) {
        if (DEBUG) {
          debug("findParticipantRecordByAddress: got "
                + JSON.stringify(participantRecord));
        }
        aCallback(participantRecord);
        return;
      }

      
      let parsedAddress = PhoneNumberUtils.parseWithMCC(aAddress, null);
      
      aParticipantStore.openCursor().onsuccess = (function (event) {
        let cursor = event.target.result;
        if (!cursor) {
          
          if (!aCreate) {
            aCallback(null);
            return;
          }

          let participantRecord = { addresses: [aAddress] };
          let addRequest = aParticipantStore.add(participantRecord);
          addRequest.onsuccess = function (event) {
            participantRecord.id = event.target.result;
            if (DEBUG) {
              debug("findParticipantRecordByAddress: created "
                    + JSON.stringify(participantRecord));
            }
            aCallback(participantRecord);
          };
          return;
        }

        let participantRecord = cursor.value;
        for each (let storedAddress in participantRecord.addresses) {
          let match = false;
          if (parsedAddress) {
            
            
            
            
            if (storedAddress.endsWith(parsedAddress.nationalNumber)) {
              match = true;
            }
          } else {
            
            
            
            let parsedStoredAddress =
              PhoneNumberUtils.parseWithMCC(storedAddress, null);
            if (parsedStoredAddress
                && aAddress.endsWith(parsedStoredAddress.nationalNumber)) {
              match = true;
            }
          }
          if (!match) {
            
            continue;
          }

          
          if (aCreate) {
            
            
            participantRecord.addresses.push(aAddress);
            cursor.update(participantRecord);
          }
          if (DEBUG) {
            debug("findParticipantRecordByAddress: got "
                  + JSON.stringify(cursor.value));
          }
          aCallback(participantRecord);
          return;
        }

        
        cursor.continue();
      }).bind(this);
    }).bind(this);
  },

  findParticipantIdsByAddresses: function findParticipantIdsByAddresses(
      aParticipantStore, aAddresses, aCreate, aSkipNonexistent, aCallback) {
    if (DEBUG) {
      debug("findParticipantIdsByAddresses("
            + JSON.stringify(aAddresses) + ", "
            + aCreate + ", " + aSkipNonexistent + ")");
    }

    if (!aAddresses || !aAddresses.length) {
      if (DEBUG) debug("findParticipantIdsByAddresses: returning null");
      aCallback(null);
      return;
    }

    let self = this;
    (function findParticipantId(index, result) {
      if (index >= aAddresses.length) {
        
        result.sort(function (a, b) {
          return a - b;
        });
        if (DEBUG) debug("findParticipantIdsByAddresses: returning " + result);
        aCallback(result);
        return;
      }

      self.findParticipantRecordByAddress(aParticipantStore,
                                          aAddresses[index++], aCreate,
                                          function (participantRecord) {
        if (!participantRecord) {
          if (!aSkipNonexistent) {
            if (DEBUG) debug("findParticipantIdsByAddresses: returning null");
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

  findThreadRecordByParticipants: function findThreadRecordByParticipants(
      aThreadStore, aParticipantStore, aAddresses,
      aCreateParticipants, aCallback) {
    if (DEBUG) {
      debug("findThreadRecordByParticipants(" + JSON.stringify(aAddresses)
            + ", " + aCreateParticipants + ")");
    }
    this.findParticipantIdsByAddresses(aParticipantStore, aAddresses,
                                       aCreateParticipants, false,
                                       function (participantIds) {
      if (!participantIds) {
        if (DEBUG) debug("findThreadRecordByParticipants: returning null");
        aCallback(null, null);
        return;
      }
      
      let request = aThreadStore.index("participantIds").get(participantIds);
      request.onsuccess = function (event) {
        let threadRecord = event.target.result;
        if (DEBUG) {
          debug("findThreadRecordByParticipants: return "
                + JSON.stringify(threadRecord));
        }
        aCallback(threadRecord, participantIds);
      };
    });
  },

  saveRecord: function saveRecord(aMessageRecord, aAddresses, aCallback) {
    if (aMessageRecord.id === undefined) {
      
      this.lastMessageId += 1;
      aMessageRecord.id = this.lastMessageId;
    }
    if (DEBUG) debug("Going to store " + JSON.stringify(aMessageRecord));

    let self = this;
    function notifyResult(rv) {
      if (!aCallback) {
        return;
      }
      let domMessage = self.createDomMessageFromRecord(aMessageRecord);
      aCallback.notify(rv, domMessage);
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
      let participantStore = stores[1];
      let threadStore = stores[2];

      self.findThreadRecordByParticipants(threadStore, participantStore,
                                          aAddresses, true,
                                          function (threadRecord,
                                                    participantIds) {
        if (!participantIds) {
          notifyResult(Cr.NS_ERROR_FAILURE);
          return;
        }

        let insertMessageRecord = function (threadId) {
          
          aMessageRecord.threadId = threadId;
          aMessageRecord.threadIdIndex = [threadId, timestamp];
          
          aMessageRecord.participantIdsIndex = [];
          for each (let id in participantIds) {
            aMessageRecord.participantIdsIndex.push([id, timestamp]);
          }
          
          messageStore.put(aMessageRecord);
        };

        let timestamp = aMessageRecord.timestamp;
        if (threadRecord) {
          let needsUpdate = false;

          if (threadRecord.lastTimestamp <= timestamp) {
            threadRecord.lastTimestamp = timestamp;
            threadRecord.subject = aMessageRecord.body;
            needsUpdate = true;
          }

          if (!aMessageRecord.read) {
            threadRecord.unreadCount++;
            needsUpdate = true;
          }

          if (needsUpdate) {
            threadStore.put(threadRecord);
          }

          insertMessageRecord(threadRecord.id);
          return;
        }

        threadStore.add({participantIds: participantIds,
                         participantAddresses: aAddresses,
                         lastMessageId: aMessageRecord.id,
                         lastTimestamp: timestamp,
                         subject: aMessageRecord.body,
                         unreadCount: aMessageRecord.read ? 0 : 1})
                   .onsuccess = function (event) {
          let threadId = event.target.result;
          insertMessageRecord(threadId);
        };
      });
    }, [MESSAGE_STORE_NAME, PARTICIPANT_STORE_NAME, THREAD_STORE_NAME]);
    
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

  



  saveReceivedMessage: function saveReceivedMessage(aMessage, aCallback) {
    if ((aMessage.type != "sms" && aMessage.type != "mms") ||
        (aMessage.type == "sms" && aMessage.messageClass == undefined) ||
        (aMessage.type == "mms" && (aMessage.delivery == undefined ||
                                    !Array.isArray(aMessage.deliveryStatus) ||
                                    !Array.isArray(aMessage.receivers))) ||
        aMessage.sender == undefined ||
        aMessage.timestamp == undefined) {
      if (aCallback) {
        aCallback.notify(Cr.NS_ERROR_FAILURE, null);
      }
      return;
    }
    let self = this.getRilIccInfoMsisdn();
    let threadParticipants = [aMessage.sender];
    if (aMessage.type == "sms") {
      
      
      aMessage.receiver = self;
    } else if (aMessage.type == "mms") {
      let receivers = aMessage.receivers;
      if (!receivers.length) {
        
        
        receivers.push(self ? self : "myself");
      } else {
        
        
        let slicedReceivers = receivers.slice();
        if (self) {
          let found = slicedReceivers.indexOf(self);
          if (found !== -1) {
            slicedReceivers.splice(found, 1);
          }
        }
        threadParticipants = threadParticipants.concat(slicedReceivers);
      }
    }

    let timestamp = aMessage.timestamp;

    
    
    aMessage.readIndex = [FILTER_READ_UNREAD, timestamp];
    aMessage.read = FILTER_READ_UNREAD;

    if (aMessage.type == "sms") {
      aMessage.delivery = DELIVERY_RECEIVED;
      aMessage.deliveryStatus = DELIVERY_STATUS_SUCCESS;
    }
    aMessage.deliveryIndex = [aMessage.delivery, timestamp];

    return this.saveRecord(aMessage, threadParticipants, aCallback);
  },

  saveSendingMessage: function saveSendingMessage(aMessage, aCallback) {
    if ((aMessage.type != "sms" && aMessage.type != "mms") ||
        (aMessage.type == "sms" && !aMessage.receiver) ||
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
    } else if (aMessage.type == "mms") {
      let receivers = aMessage.receivers
      if (!Array.isArray(receivers)) {
        if (DEBUG) {
          debug("Need receivers for MMS. Fail to save the sending message.");
        }
        if (aCallback) {
          aCallback.notify(Cr.NS_ERROR_FAILURE, null);
        }
        return;
      }
      aMessage.deliveryStatus = [];
      for (let i = 0; i < receivers.length; i++) {
        aMessage.deliveryStatus.push(deliveryStatus);
      }
    }

    
    
    aMessage.sender = this.getRilIccInfoMsisdn();
    let timestamp = aMessage.timestamp;

    
    
    aMessage.deliveryIndex = [DELIVERY_SENDING, timestamp];
    aMessage.readIndex = [FILTER_READ_READ, timestamp];
    aMessage.delivery = DELIVERY_SENDING;
    aMessage.messageClass = MESSAGE_CLASS_NORMAL;
    aMessage.read = FILTER_READ_READ;

    let addresses;
    if (aMessage.type == "sms") {
      addresses = [aMessage.receiver];
    } else if (aMessage.type == "mms") {
      addresses = aMessage.receivers;
    }
    return this.saveRecord(aMessage, addresses, aCallback);
  },

  setMessageDelivery: function setMessageDelivery(
      messageId, receiver, delivery, deliveryStatus, callback) {
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
      let domMessage = self.createDomMessageFromRecord(messageRecord);
      callback.notify(rv, domMessage);
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

        let isRecordUpdated = false;

        
        if (delivery && messageRecord.delivery != delivery) {
          messageRecord.delivery = delivery;
          messageRecord.deliveryIndex = [delivery, messageRecord.timestamp];
          isRecordUpdated = true;
        }

        
        if (deliveryStatus) {
          if (messageRecord.type == "sms") {
            if (messageRecord.deliveryStatus != deliveryStatus) {
              messageRecord.deliveryStatus = deliveryStatus;
              isRecordUpdated = true;
            }
          } else if (messageRecord.type == "mms") {
            if (!receiver) {
              for (let i = 0; i < messageRecord.deliveryStatus.length; i++) {
                if (messageRecord.deliveryStatus[i] != deliveryStatus) {
                  messageRecord.deliveryStatus[i] = deliveryStatus;
                  isRecordUpdated = true;
                }
              }
            } else {
              let index = messageRecord.receivers.indexOf(receiver);
              if (index == -1) {
                if (DEBUG) {
                  debug("Cannot find the receiver. Fail to set delivery status.");
                }
                return;
              }
              if (messageRecord.deliveryStatus[index] != deliveryStatus) {
                messageRecord.deliveryStatus[index] = deliveryStatus;
                isRecordUpdated = true;
              }
            }
          }
        }

        
        if (!isRecordUpdated) {
          if (DEBUG) {
            debug("The values of attribute delivery and deliveryStatus are the"
                  + " the same with given parameters.");
          }
          return;
        }

        if (DEBUG) {
          debug("The record's delivery and/or deliveryStatus are updated.");
        }
        messageStore.put(messageRecord);
      };
    });
  },

  getMessageRecordById: function getMessageRecordById(aMessageId, aCallback) {
    if (DEBUG) debug("Retrieving message with ID " + aMessageId);
    this.newTxn(READ_ONLY, function (error, txn, messageStore) {
      if (error) {
        if (DEBUG) debug(error);
        aCallback.notify(Ci.nsIMobileMessageCallback.INTERNAL_ERROR, null);
        return;
      }
      let request = messageStore.mozGetAll(aMessageId);

      txn.oncomplete = function oncomplete() {
        if (DEBUG) debug("Transaction " + txn + " completed.");
        if (request.result.length > 1) {
          if (DEBUG) debug("Got too many results for id " + aMessageId);
          aCallback.notify(Ci.nsIMobileMessageCallback.UNKNOWN_ERROR, null);
          return;
        }
        let messageRecord = request.result[0];
        if (!messageRecord) {
          if (DEBUG) debug("Message ID " + aMessageId + " not found");
          aCallback.notify(Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR, null);
          return;
        }
        if (messageRecord.id != aMessageId) {
          if (DEBUG) {
            debug("Requested message ID (" + aMessageId + ") is " +
                  "different from the one we got");
          }
          aCallback.notify(Ci.nsIMobileMessageCallback.UNKNOWN_ERROR, null);
          return;
        }
        aCallback.notify(Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR, messageRecord);
      };

      txn.onerror = function onerror(event) {
        if (DEBUG) {
          if (event.target) {
            debug("Caught error on transaction", event.target.errorCode);
          }
        }
        aCallback.notify(Ci.nsIMobileMessageCallback.INTERNAL_ERROR, null);
      };
    });
  },

  



  getMessage: function getMessage(aMessageId, aRequest) {
    if (DEBUG) debug("Retrieving message with ID " + aMessageId);
    let self = this;
    let notifyCallback = {
      notify: function notify(aRv, aMessageRecord) {
        if (Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR == aRv) {
          let domMessage = self.createDomMessageFromRecord(aMessageRecord);
          aRequest.notifyMessageGot(domMessage);
          return;
        }
        aRequest.notifyGetMessageFailed(aRv, null);
      }
    };
    this.getMessageRecordById(aMessageId, notifyCallback);
  },

  deleteMessage: function deleteMessage(messageId, aRequest) {
    let deleted = false;
    let self = this;
    this.newTxn(READ_WRITE, function (error, txn, stores) {
      if (error) {
        aRequest.notifyDeleteMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction", event.target.errorCode);
        
        aRequest.notifyDeleteMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
      };

      const messageStore = stores[0];
      const threadStore = stores[1];

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

            
            let threadId = messageRecord.threadId;

            threadStore.get(threadId).onsuccess = function(event) {
              
              let threadRecord = event.target.result;

              if (!messageRecord.read) {
                threadRecord.unreadCount--;
              }

              if (threadRecord.lastMessageId == messageId) {
                
                let range = IDBKeyRange.bound([threadId, 0], [threadId, ""]);
                let request = messageStore.index("threadId")
                                          .openCursor(range, PREV);
                request.onsuccess = function(event) {
                  let cursor = event.target.result;
                  if (!cursor) {
                    if (DEBUG) {
                      debug("Deleting mru entry for thread id " + threadId);
                    }
                    threadStore.delete(threadId);
                    return;
                  }

                  let nextMsg = cursor.value;
                  threadRecord.lastMessageId = nextMsg.id;
                  threadRecord.lastTimestamp = nextMsg.timestamp;
                  threadRecord.subject = nextMsg.body;
                  if (DEBUG) {
                    debug("Updating mru entry: " +
                          JSON.stringify(threadRecord));
                  }
                  threadStore.put(threadRecord);
                };
              } else if (!messageRecord.read) {
                
                if (DEBUG) {
                  debug("Updating unread count for thread id " + threadId + ": " +
                        (threadRecord.unreadCount + 1) + " -> " +
                        threadRecord.unreadCount);
                }
                threadStore.put(threadRecord);
              }
            };
          };
        } else if (DEBUG) {
          debug("Message id " + messageId + " does not exist");
        }
      };
    }, [MESSAGE_STORE_NAME, THREAD_STORE_NAME]);
  },

  createMessageCursor: function createMessageCursor(filter, reverse, callback) {
    if (DEBUG) {
      debug("Creating a message cursor. Filters:" +
            " startDate: " + filter.startDate +
            " endDate: " + filter.endDate +
            " delivery: " + filter.delivery +
            " numbers: " + filter.numbers +
            " read: " + filter.read +
            " reverse: " + reverse);
    }

    let cursor = new GetMessagesCursor(this, callback);

    let self = this;
    self.newTxn(READ_ONLY, function (error, txn, stores) {
      let collector = cursor.collector;
      let collect = collector.collect.bind(collector);
      FilterSearcherHelper.transact(self, txn, error, filter, reverse, collect);
    }, [MESSAGE_STORE_NAME, PARTICIPANT_STORE_NAME]);

    return cursor;
  },

  markMessageRead: function markMessageRead(messageId, value, aRequest) {
    if (DEBUG) debug("Setting message " + messageId + " read to " + value);
    this.newTxn(READ_WRITE, function (error, txn, stores) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyMarkMessageReadFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.errorCode);
        aRequest.notifyMarkMessageReadFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
      };
      let messageStore = stores[0];
      let threadStore = stores[1];
      messageStore.get(messageId).onsuccess = function onsuccess(event) {
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
        if (DEBUG) debug("Message.read set to: " + value);
        messageStore.put(messageRecord).onsuccess = function onsuccess(event) {
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
              aRequest.notifyMessageMarkedRead(messageRecord.read);
            };
          };
        };
      };
    }, [MESSAGE_STORE_NAME, THREAD_STORE_NAME]);
  },

  getThreadList: function getThreadList(aRequest) {
    if (DEBUG) debug("Getting thread list");
    this.newTxn(READ_ONLY, function (error, txn, threadStore) {
      if (error) {
        if (DEBUG) debug(error);
        aRequest.notifyThreadListFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      txn.onerror = function onerror(event) {
        if (DEBUG) debug("Caught error on transaction ", event.target.errorCode);
        aRequest.notifyThreadListFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
      };
      let request = threadStore.index("lastTimestamp").mozGetAll();
      request.onsuccess = function(event) {
        
        let results = [];
        for each (let item in event.target.result) {
          results.push({
            id: item.id,
            senderOrReceiver: item.participantAddresses[0],
            timestamp: item.lastTimestamp,
            body: item.subject,
            unreadCount: item.unreadCount
          });
        }
        aRequest.notifyThreadList(results);
      };
    }, [THREAD_STORE_NAME]);
  }
};

let FilterSearcherHelper = {

  












  filterIndex: function filterIndex(index, range, direction, txn, collect) {
    let messageStore = txn.objectStore(MESSAGE_STORE_NAME);
    let request = messageStore.index(index).openKeyCursor(range, direction);
    request.onsuccess = function onsuccess(event) {
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
    request.onerror = function onerror(event) {
      if (DEBUG && event) debug("IDBRequest error " + event.target.errorCode);
      collect(txn, COLLECT_ID_ERROR, COLLECT_TIMESTAMP_UNUSED);
    };
  },

  














  filterTimestamp: function filterTimestamp(startDate, endDate, direction, txn,
                                            collect) {
    let range = null;
    if (startDate != null && endDate != null) {
      range = IDBKeyRange.bound(startDate.getTime(), endDate.getTime());
    } else if (startDate != null) {
      range = IDBKeyRange.lowerBound(startDate.getTime());
    } else if (endDate != null) {
      range = IDBKeyRange.upperBound(endDate.getTime());
    }
    this.filterIndex("timestamp", range, direction, txn, collect);
  },

  

















  transact: function transact(service, txn, error, filter, reverse, collect) {
    if (error) {
      
      if (DEBUG) debug("IDBRequest error " + error.target.errorCode);
      collect(txn, COLLECT_ID_ERROR, COLLECT_TIMESTAMP_UNUSED);
      return;
    }

    let direction = reverse ? PREV : NEXT;

    
    
    if (filter.delivery == null &&
        filter.numbers == null &&
        filter.read == null) {
      
      if (DEBUG) {
        debug("filter.timestamp " + filter.startDate + ", " + filter.endDate);
      }

      this.filterTimestamp(filter.startDate, filter.endDate, direction, txn,
                           collect);
      return;
    }

    
    
    let startDate = 0, endDate = "";
    if (filter.startDate != null) {
      startDate = filter.startDate.getTime();
    }
    if (filter.endDate != null) {
      endDate = filter.endDate.getTime();
    }

    let single, intersectionCollector;
    {
      let num = 0;
      if (filter.delivery) num++;
      if (filter.numbers) num++;
      if (filter.read != undefined) num++;
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

    
    
    if (filter.numbers) {
      if (DEBUG) debug("filter.numbers " + filter.numbers.join(", "));

      if (!single) {
        collect = intersectionCollector.newContext();
      }

      let participantStore = txn.objectStore(PARTICIPANT_STORE_NAME);
      service.findParticipantIdsByAddresses(participantStore, filter.numbers,
                                            false, true,
                                            (function (participantIds) {
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

function ResultsCollector() {
  this.results = [];
  this.done = false;
}
ResultsCollector.prototype = {
  results: null,
  requestWaiting: null,
  done: null,

  













  collect: function collect(txn, id, timestamp) {
    if (this.done) {
      return false;
    }

    if (DEBUG) {
      debug("collect: message ID = " + id);
    }
    if (id) {
      
      this.results.push(id);
    }
    if (id <= 0) {
      
      this.done = true;
    }

    if (!this.requestWaiting) {
      if (DEBUG) debug("Cursor.continue() not called yet");
      return !this.done;
    }

    
    
    
    
    let callback = this.requestWaiting;
    this.requestWaiting = null;

    this.drip(txn, callback);

    return !this.done;
  },

  






  squeeze: function squeeze(callback) {
    if (this.requestWaiting) {
      throw new Error("Already waiting for another request!");
    }

    if (!this.done) {
      
      
      this.requestWaiting = callback;
      return;
    }

    this.drip(null, callback);
  },

  





  drip: function drip(txn, callback) {
    if (!this.results.length) {
      if (DEBUG) debug("No messages matching the filter criteria");
      callback(txn, COLLECT_ID_END);
      return;
    }

    if (this.results[0] < 0) {
      
      
      if (DEBUG) debug("An previous error found");
      callback(txn, COLLECT_ID_ERROR);
      return;
    }

    let firstMessageId = this.results.shift();
    callback(txn, firstMessageId);
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

  



  collect: function collect(contextIndex, txn, id, timestamp) {
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

  newContext: function newContext() {
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

  collect: function collect(contextIndex, txn, id, timestamp) {
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
    tres = tres.filter(function (element) {
      return qres.indexOf(element.id) != -1;
    });

    for (let i = 0; i < tres.length; i++) {
      this.cascadedCollect(txn, tres[i].id, tres[i.timestamp]);
    }
    this.cascadedCollect(txn, COLLECT_ID_END, COLLECT_TIMESTAMP_UNUSED);

    return false;
  },

  newTimestampContext: function newTimestampContext() {
    return this.collect.bind(this, 0);
  },

  newContext: function newContext() {
    this.contexts[1].processing++;
    return this.collect.bind(this, 1);
  }
};

function GetMessagesCursor(service, callback) {
  this.service = service;
  this.callback = callback;
  this.collector = new ResultsCollector();

  this.handleContinue(); 
}
GetMessagesCursor.prototype = {
  classID: RIL_GETMESSAGESCURSOR_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICursorContinueCallback]),

  service: null,
  callback: null,
  collector: null,

  getMessageTxn: function getMessageTxn(messageStore, messageId) {
    if (DEBUG) debug ("Fetching message " + messageId);

    let getRequest = messageStore.get(messageId);
    let self = this;
    getRequest.onsuccess = function onsuccess(event) {
      if (DEBUG) {
        debug("notifyNextMessageInListGot - messageId: " + messageId);
      }
      let domMessage =
        self.service.createDomMessageFromRecord(event.target.result);
      self.callback.notifyCursorResult(domMessage);
    };
    getRequest.onerror = function onerror(event) {
      if (DEBUG) {
        debug("notifyCursorError - messageId: " + messageId);
      }
      self.callback.notifyCursorError(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
    };
  },

  notify: function notify(txn, messageId) {
    if (!messageId) {
      this.callback.notifyCursorDone();
      return;
    }

    if (messageId < 0) {
      this.callback.notifyCursorError(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
      return;
    }

    
    
    if (txn) {
      let messageStore = txn.objectStore(MESSAGE_STORE_NAME);
      this.getMessageTxn(messageStore, messageId);
      return;
    }

    
    let self = this;
    this.service.newTxn(READ_ONLY, function (error, txn, messageStore) {
      if (error) {
        self.callback.notifyCursorError(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      self.getMessageTxn(messageStore, messageId);
    }, [MESSAGE_STORE_NAME]);
  },

  

  handleContinue: function handleContinue() {
    if (DEBUG) debug("Getting next message in list");
    this.collector.squeeze(this.notify.bind(this));
  }
};

XPCOMUtils.defineLazyServiceGetter(MobileMessageDatabaseService.prototype, "mRIL",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MobileMessageDatabaseService]);

function debug() {
  dump("MobileMessageDatabaseService: " + Array.slice(arguments).join(" ") + "\n");
}
