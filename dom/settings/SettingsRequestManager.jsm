



"use strict";

const DEBUG = false;
function debug(s) { dump("-*- SettingsRequestManager: " + s + "\n"); }
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["SettingsRequestManager"];

Cu.import("resource://gre/modules/SettingsDB.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");

const kXpcomShutdownObserverTopic      = "xpcom-shutdown";
const kMozSettingsChangedObserverTopic = "mozsettings-changed";
const kSettingsReadSuffix              = "-read";
const kSettingsWriteSuffix             = "-write";
const kSettingsClearPermission         = "settings-clear";
const kAllSettingsReadPermission       = "settings" + kSettingsReadSuffix;
const kAllSettingsWritePermission      = "settings" + kSettingsWriteSuffix;





const kSomeSettingsReadPermission      = "settings-api" + kSettingsReadSuffix;
const kSomeSettingsWritePermission     = "settings-api" + kSettingsWriteSuffix;

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");
XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

let SettingsPermissions = {
  checkPermission: function(aPrincipal, aPerm) {
    if (aPrincipal.origin == "[System Principal]" ||
        Services.perms.testExactPermissionFromPrincipal(aPrincipal, aPerm) == Ci.nsIPermissionManager.ALLOW_ACTION) {
      return true;
    }
    return false;
  },
  hasAllReadPermission: function(aPrincipal) {
    return this.checkPermission(aPrincipal, kAllSettingsReadPermission);
  },
  hasAllWritePermission: function(aPrincipal) {
    return this.checkPermission(aPrincipal, kAllSettingsWritePermission);
  },
  hasSomeReadPermission: function(aPrincipal) {
    return this.checkPermission(aPrincipal, kSomeSettingsReadPermission);
  },
  hasSomeWritePermission: function(aPrincipal) {
    return this.checkPermission(aPrincipal, kSomeSettingsWritePermission);
  },
  hasClearPermission: function(aPrincipal) {
    return this.checkPermission(aPrincipal, kSettingsClearPermission);
  },
  hasReadPermission: function(aPrincipal, aSettingsName) {
    return this.hasAllReadPermission(aPrincipal) || this.checkPermission(aPrincipal, "settings:" + aSettingsName + kSettingsReadSuffix);
  },
  hasWritePermission: function(aPrincipal, aSettingsName) {
    return this.hasAllWritePermission(aPrincipal) || this.checkPermission(aPrincipal, "settings:" + aSettingsName + kSettingsWriteSuffix);
  }
};


function SettingsLockInfo(aDB, aMsgMgr, aLockID, aIsServiceLock) {
  return {
    
    lockID: aLockID,
    
    isServiceLock: aIsServiceLock,
    
    tasks: [],
    
    
    consumable: false,
    
    
    queuedSets: {},
    
    _transaction: undefined,
    
    _mm: aMsgMgr,
    
    _failed: false,
    
    
    finalizing: false,
    
    canClear: true,
    
    hasCleared: false,
    getObjectStore: function(aPrincipal) {
      if (DEBUG) debug("Getting transaction for " + this.lockID);
      let store;
      
      
      
      if (this._transaction) {
        try {
          store = this._transaction.objectStore(SETTINGSSTORE_NAME);
        } catch (e) {
          if (e.name == "InvalidStateError") {
            if (DEBUG) debug("Current transaction for " + this.lockID + " closed, trying to create new one.");
          } else {
            throw e;
          }
        }
      }
      
      
      
      
      if (!SettingsPermissions.hasSomeWritePermission(aPrincipal)) {
        if (DEBUG) debug("Making READONLY transaction for " + this.lockID);
        this._transaction = aDB._db.transaction(SETTINGSSTORE_NAME, "readonly");
      } else {
        if (DEBUG) debug("Making READWRITE transaction for " + this.lockID);
        this._transaction = aDB._db.transaction(SETTINGSSTORE_NAME, "readwrite");
      }
      this._transaction.oncomplete = function() {
        if (DEBUG) debug("Transaction for lock " + this.lockID + " closed");
      }.bind(this);
      this._transaction.onabort = function () {
        if (DEBUG) debug("Transaction for lock " + this.lockID + " aborted");
        this._failed = true;
      }.bind(this);
      try {
        store = this._transaction.objectStore(SETTINGSSTORE_NAME);
      } catch (e) {
          if (e.name == "InvalidStateError") {
            if (DEBUG) debug("Cannot create objectstore on transaction for " + this.lockID);
            return null;
          } else {
            throw e;
          }
      }
      return store;
    }
  };
}

let SettingsRequestManager = {
  
  settingsDB: new SettingsDB(),
  
  messages: ["child-process-shutdown", "Settings:Get", "Settings:Set",
             "Settings:Clear", "Settings:Run", "Settings:Finalize",
             "Settings:CreateLock", "Settings:RegisterForMessages"],
  
  lockInfo: {},
  
  
  
  settingsLockQueue: [],
  children: [],
  mmPrincipals: {},
  init: function() {
    if (DEBUG) debug("init");
    this.settingsDB.init();
    this.messages.forEach((function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }).bind(this));
    Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
  },

  _serializePreservingBinaries: function _serializePreservingBinaries(aObject) {
    function needsUUID(aValue) {
      if (!aValue || !aValue.constructor) {
        return false;
      }
      return (aValue.constructor.name == "Date") || (aValue instanceof Ci.nsIDOMFile) ||
             (aValue instanceof Ci.nsIDOMBlob);
    }
    
    
    
    
    
    let binaries = Object.create(null);
    let stringified = JSON.stringify(aObject, function(key, value) {
      value = this.settingsDB.prepareValue(value);
      if (needsUUID(value)) {
        let uuid = uuidgen.generateUUID().toString();
        binaries[uuid] = value;
        return uuid;
      }
      return value;
    }.bind(this));
    return JSON.parse(stringified, function(key, value) {
      if (value in binaries) {
        return binaries[value];
      }
      return value;
    });
  },

  queueTask: function(aOperation, aData, aPrincipal) {
    if (DEBUG) debug("Queueing task: " + aOperation);

    let defer = {};

    let lock = this.lockInfo[aData.lockID];

    if (!lock) {
      return Promise.reject({error: "Lock already dead, cannot queue task"});
    }

    if (aOperation == "set") {
      aData.settings = this._serializePreservingBinaries(aData.settings);
    }

    this.lockInfo[aData.lockID].tasks.push({
      operation: aOperation,
      data: aData,
      principal: aPrincipal,
      defer: defer
    });

    let promise = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });

    return promise;
  },

  
  
  
  
  
  
  
  queueTaskReturn: function(aTask, aReturnValue) {
    if (DEBUG) debug("Making task queuing transaction request.");
    let data = aTask.data;
    let lock = this.lockInfo[data.lockID];
    let store = lock.getObjectStore(aTask.principal);
    if (!store) {
      if (DEBUG) debug("Rejecting task queue on lock " + aTask.data.lockID);
      return Promise.reject({task: aTask, error: "Cannot get object store"});
    }
    
    
    
    
    
    
    let getReq = store.get(0);

    let defer = {};
    let promiseWrapper = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });

    getReq.onsuccess = function(event) {
      return defer.resolve(aReturnValue);
    };
    getReq.onerror = function() {
      return defer.reject({task: aTask, error: getReq.error.name});
    };
    return promiseWrapper;
  },
  
  taskGet: function(aTask) {
    if (DEBUG) debug("Running Get task on lock " + aTask.data.lockID);

    
    let data = aTask.data;
    let lock = this.lockInfo[data.lockID];

    if (!lock) {
      return Promise.reject({task: aTask, error: "Lock died, can't finalize"});
    }
    if (lock._failed) {
      if (DEBUG) debug("Lock failed. All subsequent requests will fail.");
      return Promise.reject({task: aTask, error: "Lock failed, all requests now failing."});
    }

    if (lock.hasCleared) {
      if (DEBUG) debug("Lock was used for a clear command. All subsequent requests will fail.");
      return Promise.reject({task: aTask, error: "Lock was used for a clear command. All subsequent requests will fail."});
    }

    lock.canClear = false;
    
    if (!SettingsPermissions.hasReadPermission(aTask.principal, data.name)) {
      if (DEBUG) debug("get not allowed for " + data.name);
      lock._failed = true;
      return Promise.reject({task: aTask, error: "No permission to get " + data.name});
    }

    
    if (data.name in lock.queuedSets) {
      if (DEBUG) debug("Returning cached set value " + lock.queuedSets[data.name] + " for " + data.name);
      let local_results = {};
      local_results[data.name] = lock.queuedSets[data.name];
      return this.queueTaskReturn(aTask, {task: aTask, results: local_results});
    }

    
    if (DEBUG) debug("Making get transaction request for " + data.name);
    let store = lock.getObjectStore(aTask.principal);
    if (!store) {
      if (DEBUG) debug("Rejecting Get task on lock " + aTask.data.lockID);
      return Promise.reject({task: aTask, error: "Cannot get object store"});
    }

    if (DEBUG) debug("Making get request for " + data.name);
    let getReq = (data.name === "*") ? store.mozGetAll() : store.mozGetAll(data.name);

    let defer = {};
    let promiseWrapper = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });

    getReq.onsuccess = function(event) {
      if (DEBUG) debug("Request for '" + data.name + "' successful. " +
            "Record count: " + event.target.result.length);

      if (event.target.result.length == 0) {
        if (DEBUG) debug("MOZSETTINGS-GET-WARNING: " + data.name + " is not in the database.\n");
      }

      let results = {};

      for (let i in event.target.result) {
        let result = event.target.result[i];
        let name = result.settingName;
        if (DEBUG) debug(name + ": " + result.userValue +", " + result.defaultValue);
        let value = result.userValue !== undefined ? result.userValue : result.defaultValue;
        results[name] = value;
      }
      return defer.resolve({task: aTask, results: results});
    };
    getReq.onerror = function() {
      return defer.reject({task: aTask, error: getReq.error.name});
    };
    return promiseWrapper;
  },

  taskSet: function(aTask) {
    let data = aTask.data;
    let lock = this.lockInfo[data.lockID];
    let keys = Object.getOwnPropertyNames(data.settings);

    if (!lock) {
      return Promise.reject({task: aTask, error: "Lock died, can't finalize"});
    }
    if (lock._failed) {
      if (DEBUG) debug("Lock failed. All subsequent requests will fail.");
      return Promise.reject({task: aTask, error: "Lock failed a permissions check, all requests now failing."});
    }

    if (lock.hasCleared) {
      if (DEBUG) debug("Lock was used for a clear command. All subsequent requests will fail.");
      return Promise.reject({task: aTask, error: "Lock was used for a clear command. All other requests will fail."});
    }

    lock.canClear = false;

    
    if (keys.length === 0) {
      if (DEBUG) debug("No keys to change entered!");
      return Promise.resolve({task: aTask});
    }

    for (let i = 0; i < keys.length; i++) {
      if (!SettingsPermissions.hasWritePermission(aTask.principal, keys[i])) {
        if (DEBUG) debug("set not allowed on " + keys[i]);
        lock._failed = true;
        return Promise.reject({task: aTask, error: "No permission to set " + keys[i]});
      }
    }

    for (let i = 0; i < keys.length; i++) {
      let key = keys[i];
      if (DEBUG) debug("key: " + key + ", val: " + JSON.stringify(data.settings[key]) + ", type: " + typeof(data.settings[key]));
      lock.queuedSets[key] = data.settings[key];
    }

    return this.queueTaskReturn(aTask, {task: aTask});
  },

  queueConsume: function() {
    if (this.settingsLockQueue.length > 0 && this.lockInfo[this.settingsLockQueue[0]].consumable) {
      Services.tm.currentThread.dispatch(SettingsRequestManager.consumeTasks.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
    }
  },

  finalizeSets: function(aTask) {
    let data = aTask.data;
    if (DEBUG) debug("Finalizing tasks for lock " + data.lockID);
    let lock = this.lockInfo[data.lockID];

    if (!lock) {
      return Promise.reject({task: aTask, error: "Lock died, can't finalize"});
    }
    lock.finalizing = true;
    if (lock._failed) {
      this.removeLock(data.lockID);
      return Promise.reject({task: aTask, error: "Lock failed a permissions check, all requests now failing."});
    }
    
    
    if (lock.hasCleared) {
      if (DEBUG) debug("Clear was called on lock, skipping finalize");
      this.removeLock(data.lockID);
      return Promise.resolve({task: aTask});
    }
    let keys = Object.getOwnPropertyNames(lock.queuedSets);
    if (keys.length === 0) {
      if (DEBUG) debug("Nothing to finalize. Exiting.");
      this.removeLock(data.lockID);
      return Promise.resolve({task: aTask});
    }

    let store = lock.getObjectStore(aTask.principal);
    if (!store) {
      if (DEBUG) debug("Rejecting Set task on lock " + aTask.data.lockID);
      return Promise.reject({task: aTask, error: "Cannot get object store"});
    }

    
    
    
    let checkPromises = [];
    let finalValues = {};
    for (let i = 0; i < keys.length; i++) {
      let key = keys[i];
      if (DEBUG) debug("key: " + key + ", val: " + lock.queuedSets[key] + ", type: " + typeof(lock.queuedSets[key]));
      let checkDefer = {};
      let checkPromise = new Promise(function(resolve, reject) {
        checkDefer.resolve = resolve;
        checkDefer.reject = reject;
      });

      
      
      
      let checkKeyRequest = store.get(key);
      checkKeyRequest.onsuccess = function (event) {
        let userValue = lock.queuedSets[key];
        let defaultValue;
        if (!event.target.result) {
          defaultValue = null;
          if (DEBUG) debug("MOZSETTINGS-GET-WARNING: " + key + " is not in the database.\n");
        } else {
          defaultValue = event.target.result.defaultValue;
        }
        let obj = {settingName: key, defaultValue: defaultValue, userValue: userValue};
        finalValues[key] = {defaultValue: defaultValue, userValue: userValue};
        let setReq = store.put(obj);
        setReq.onsuccess = function() {
          if (DEBUG) debug("Set successful!");
          if (DEBUG) debug("key: " + key + ", val: " + finalValues[key] + ", type: " + typeof(finalValues[key]));
          return checkDefer.resolve({task: aTask});
        };
        setReq.onerror = function() {
          return checkDefer.reject({task: aTask, error: setReq.error.name});
        };
      }.bind(this);
      checkKeyRequest.onerror = function(event) {
        return checkDefer.reject({task: aTask, error: checkKeyRequest.error.name});
      };
      checkPromises.push(checkPromise);
    }

    let defer = {};
    let promiseWrapper = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });

    
    
    Promise.all(checkPromises).then(function() {
      
      for (let i = 0; i < keys.length; i++) {
        this.sendSettingsChange(keys[i], finalValues[keys[i]].userValue, lock.isServiceLock);
      }
      this.removeLock(data.lockID);
      defer.resolve({task: aTask});
    }.bind(this), function(ret) {
      this.removeLock(data.lockID);
      defer.reject({task: aTask, error: "Set transaction failure"});
    }.bind(this));
    return promiseWrapper;
  },

  
  
  
  
  
  
  taskClear: function(aTask) {
    if (DEBUG) debug("Clearing");
    let data = aTask.data;
    let lock = this.lockInfo[data.lockID];

    if (lock._failed) {
      if (DEBUG) debug("Lock failed, all requests now failing.");
      return Promise.reject({task: aTask, error: "Lock failed, all requests now failing."});
    }

    if (!lock.canClear) {
      if (DEBUG) debug("Lock tried to clear after queuing other tasks. Failing.");
      lock._failed = true;
      return Promise.reject({task: aTask, error: "Cannot call clear after queuing other tasks, all requests now failing."});
    }

    if (!SettingsPermissions.hasClearPermission(aTask.principal)) {
      if (DEBUG) debug("clear not allowed");
      lock._failed = true;
      return Promise.reject({task: aTask, error: "No permission to clear DB"});
    }

    lock.hasCleared = true;

    let store = lock.getObjectStore(aTask.principal);
    if (!store) {
      if (DEBUG) debug("Rejecting Clear task on lock " + aTask.data.lockID);
      return Promise.reject({task: aTask, error: "Cannot get object store"});
    }
    let defer = {};
    let promiseWrapper = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });

    let clearReq = store.clear();
    clearReq.onsuccess = function() {
      return defer.resolve({task: aTask});
    };
    clearReq.onerror = function() {
      return defer.reject({task: aTask});
    };
    return promiseWrapper;
  },

  ensureConnection : function() {
    if (DEBUG) debug("Ensuring Connection");
    let defer = {};
    let promiseWrapper = new Promise(function(resolve, reject) {
      defer.resolve = resolve;
      defer.reject = reject;
    });
    this.settingsDB.ensureDB(
      function() { defer.resolve(); },
      function(error) {
        if (DEBUG) debug("Cannot open Settings DB. Trying to open an old version?\n");
        defer.reject(error);
      }
    );
    return promiseWrapper;
  },

  runTasks: function(aLockID) {
    if (DEBUG) debug("Running tasks for " + aLockID);
    let lock = this.lockInfo[aLockID];
    if (!lock) {
      if (DEBUG) debug("Lock no longer alive, cannot run tasks");
      return;
    }
    if (lock.finalizing) {
      debug("TASK TRYING TO QUEUE AFTER FINALIZE CALLED. THIS IS BAD. Lock: " + aLockID);
      return;
    }
    let currentTask = lock.tasks.shift();
    let promises = [];
    while (currentTask) {
      if (DEBUG) debug("Running Operation " + currentTask.operation);
      let p;
      switch (currentTask.operation) {
        case "get":
          p = this.taskGet(currentTask);
          break;
        case "set":
          p = this.taskSet(currentTask);
          break;
        case "clear":
          p = this.taskClear(currentTask);
          break;
        case "finalize":
          p = this.finalizeSets(currentTask);
          break;
        default:
          if (DEBUG) debug("Invalid operation: " + currentTask.operation);
          p.reject("Invalid operation: " + currentTask.operation);
      }
      p.then(function(ret) {
        ret.task.defer.resolve("results" in ret ? ret.results : null);
      }.bind(currentTask), function(ret) {
        ret.task.defer.reject(ret.error);
      });
      promises.push(p);
      currentTask = lock.tasks.shift();
    }
  },

  consumeTasks: function() {
    if (this.settingsLockQueue.length == 0) {
      if (DEBUG) debug("Nothing to run!");
      return;
    }

    let lockID = this.settingsLockQueue[0];
    if (DEBUG) debug("Consuming tasks for " + lockID);
    let lock = this.lockInfo[lockID];

    
    
    
    if (!lock) {
      if (DEBUG) debug("Lock no longer alive, cannot consume tasks");
      this.queueConsume();
      return;
    }

    if (!lock.consumable || lock.tasks.length === 0) {
      if (DEBUG) debug("No more tasks to run or not yet consuamble.");
      return;
    }

    lock.consumable = false;
    this.ensureConnection().then(
      function(task) {
        this.runTasks(lockID);
      }.bind(this), function(ret) {
        dump("-*- SettingsRequestManager: SETTINGS DATABASE ERROR: Cannot make DB connection!\n");
    });
  },

  observe: function(aSubject, aTopic, aData) {
    if (DEBUG) debug("observe");
    switch (aTopic) {
      case kXpcomShutdownObserverTopic:
        this.messages.forEach((function(msgName) {
          ppmm.removeMessageListener(msgName, this);
        }).bind(this));
        Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
        ppmm = null;
        break;
      default:
        if (DEBUG) debug("Wrong observer topic: " + aTopic);
        break;
    }
  },

  sendSettingsChange: function(aKey, aValue, aIsServiceLock) {
    this.broadcastMessage("Settings:Change:Return:OK",
      { key: aKey, value: aValue });
    Services.obs.notifyObservers(this, kMozSettingsChangedObserverTopic,
      JSON.stringify({
        key: aKey,
        value: aValue,
        isInternalChange: aIsServiceLock
      }));
  },

  broadcastMessage: function broadcastMessage(aMsgName, aContent) {
    if (DEBUG) debug("Broadcast");
    this.children.forEach(function(msgMgr) {
      let principal = this.mmPrincipals[msgMgr];
      if (!principal) {
        if (DEBUG) debug("Cannot find principal for message manager to check permissions");
      }
      else if (SettingsPermissions.hasReadPermission(principal, aContent.key)) {
        msgMgr.sendAsyncMessage(aMsgName, aContent);
      }
    }.bind(this));
    if (DEBUG) debug("Finished Broadcasting");
  },

  addObserver: function(aMsgMgr, aPrincipal) {
    if (DEBUG) debug("Add observer for" + aMsgMgr);
    if (this.children.indexOf(aMsgMgr) == -1) {
      this.children.push(aMsgMgr);
      this.mmPrincipals[aMsgMgr] = aPrincipal;
    }
  },

  removeObserver: function(aMsgMgr) {
    if (DEBUG) debug("Remove observer for" + aMsgMgr);
    let index = this.children.indexOf(aMsgMgr);
    if (index != -1) {
      this.children.splice(index, 1);
      delete this.mmPrincipals[aMsgMgr];
    }
  },

  removeLock: function(aLockID) {
    if (DEBUG) debug("Removing lock " + aLockID);
    let transaction = this.lockInfo[aLockID]._transaction;
    if (transaction) {
      try {
        transaction.abort();
      } catch (e) {
        if (e.name == "InvalidStateError") {
          if (DEBUG) debug("Transaction for " + aLockID + " closed already");
        } else {
          throw e;
        }
      }
    }
    delete this.lockInfo[aLockID];
    let index = this.settingsLockQueue.indexOf(aLockID);
    if (index > -1) {
      this.settingsLockQueue.splice(index, 1);
    }
    
    
    
    if (index == 0) {
      this.queueConsume();
    }
  },

  removeMessageManager: function(aMsgMgr){
    if (DEBUG) debug("Removing message manager " + aMsgMgr);
    this.removeObserver(aMsgMgr);
    let closedLockIDs = [];
    let lockIDs = Object.keys(this.lockInfo);
    for (let i in lockIDs) {
      if (this.lockInfo[lockIDs[i]]._mm == aMsgMgr) {
      	if (DEBUG) debug("Removing lock " + lockIDs[i] + " due to process close/crash");
        closedLockIDs.push(lockIDs[i]);
      }
    }
    for (let i in closedLockIDs) {
      this.removeLock(closedLockIDs[i]);
    }
  },

  receiveMessage: function(aMessage) {
    if (DEBUG) debug("receiveMessage " + aMessage.name);

    let msg = aMessage.data;
    let mm = aMessage.target;

    function returnMessage(name, data) {
      try {
        mm.sendAsyncMessage(name, data);
      } catch (e) {
        if (DEBUG) debug("Return message failed, " + name);
      }
    }

    
    
    
    
    
    switch (aMessage.name) {
      case "Settings:Get":
      case "Settings:Set":
      case "Settings:Clear":
      case "Settings:Run":
      case "Settings:Finalize":
        let kill_process = false;
        if (!msg.lockID) {
          Cu.reportError("Process sending request for lock that does not exist. Killing.");
          kill_process = true;
        }
        else if (!this.lockInfo[msg.lockID]) {
          if (DEBUG) debug("Cannot find lock ID " + msg.lockID);
          
          
          
          return;
        }
        else if (mm != this.lockInfo[msg.lockID]._mm) {
          Cu.reportError("Process trying to access settings lock from another process. Killing.");
          kill_process = true;
        }
        if (kill_process) {
          
          aMessage.target.assertPermission("message-manager-mismatch-kill");
          return;
        }
      default:
      break;
    }

    switch (aMessage.name) {
      case "child-process-shutdown":
        if (DEBUG) debug("Child process shutdown received.");
        this.removeMessageManager(mm);
        break;
      case "Settings:RegisterForMessages":
        if (!SettingsPermissions.hasSomeReadPermission(aMessage.principal)) {
          Cu.reportError("Settings message " + aMessage.name +
                         " from a content process with no 'settings-api-read' privileges.");
          aMessage.target.assertPermission("message-manager-no-read-kill");
          return;
        }
        this.addObserver(mm, aMessage.principal);
        break;
      case "Settings:UnregisterForMessages":
        this.removeObserver(mm);
        break;
      case "Settings:CreateLock":
        if (DEBUG) debug("Received CreateLock for " + msg.lockID + " from " + aMessage.principal.origin);
        
        
        
        if (msg.lockID in this.settingsLockQueue) {
          Cu.reportError("Trying to queue a lock with the same ID as an already queued lock. Killing app.");
          aMessage.target.assertPermission("lock-id-duplicate-kill");
          return;
        }
        this.settingsLockQueue.push(msg.lockID);
        this.lockInfo[msg.lockID] = SettingsLockInfo(this.settingsDB, mm, msg.lockID, msg.isServiceLock);
        break;
      case "Settings:Get":
        if (DEBUG) debug("Received getRequest from " + msg.lockID);
        this.queueTask("get", msg, aMessage.principal).then(function(settings) {
            returnMessage("Settings:Get:OK", {
              lockID: msg.lockID,
              requestID: msg.requestID,
              settings: settings
            });
          }, function(error) {
            if (DEBUG) debug("getRequest FAILED " + msg.name);
            returnMessage("Settings:Get:KO", {
              lockID: msg.lockID,
              requestID: msg.requestID,
              errorMsg: error
            });
        });
        break;
      case "Settings:Set":
        if (DEBUG) debug("Received Set Request from " + msg.lockID);
        this.queueTask("set", msg, aMessage.principal).then(function(settings) {
          returnMessage("Settings:Set:OK", {
            lockID: msg.lockID,
            requestID: msg.requestID
          });
        }, function(error) {
          returnMessage("Settings:Set:KO", {
            lockID: msg.lockID,
            requestID: msg.requestID,
            errorMsg: error
          });
        });
        break;
      case "Settings:Clear":
        if (DEBUG) debug("Received Clear Request from " + msg.lockID);
        this.queueTask("clear", msg, aMessage.principal).then(function() {
          returnMessage("Settings:Clear:OK", {
            lockID: msg.lockID,
            requestID: msg.requestID
          });
        }, function(error) {
          returnMessage("Settings:Clear:KO", {
            lockID: msg.lockID,
            requestID: msg.requestID,
            errorMsg: error
          });
        });
        break;
      case "Settings:Finalize":
        if (DEBUG) debug("Received Finalize");
        this.queueTask("finalize", msg, aMessage.principal).then(function() {
          returnMessage("Settings:Finalize:OK", {
            lockID: msg.lockID
          });
        }, function(error) {
          returnMessage("Settings:Finalize:KO", {
            lockID: msg.lockID,
            errorMsg: error
          });
        });
      
      
      case "Settings:Run":
        if (DEBUG) debug("Received Run");
        this.lockInfo[msg.lockID].consumable = true;
        if (msg.lockID == this.settingsLockQueue[0] || this.settingsLockQueue.length == 0) {
          
          
          if (DEBUG) debug("Running tasks for " + msg.lockID);
          this.queueConsume();
        } else {
          
          
          
          if (DEBUG) debug("Queuing tasks for " + msg.lockID + " while waiting for " + this.settingsLockQueue[0]);
        }
        break;
      default:
        if (DEBUG) debug("Wrong message: " + aMessage.name);
    }
  }
};

SettingsRequestManager.init();
