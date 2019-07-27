



'use strict'

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

function debug(s) {
  
}

const RSYNCDB_VERSION = 1;
const RSYNCDB_NAME = "requestSync";
const RSYNC_MIN_INTERVAL = 100;

const RSYNC_OPERATION_TIMEOUT = 120000 

const RSYNC_STATE_ENABLED = "enabled";
const RSYNC_STATE_DISABLED = "disabled";
const RSYNC_STATE_WIFIONLY = "wifiOnly";

Cu.import('resource://gre/modules/IndexedDBHelper.jsm');
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.importGlobalProperties(["indexedDB"]);


XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "systemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "secMan",
                                   "@mozilla.org/scriptsecuritymanager;1",
                                   "nsIScriptSecurityManager");

this.RequestSyncService = {
  __proto__: IndexedDBHelper.prototype,

  children: [],

  _messages: [ "RequestSync:Register",
               "RequestSync:Unregister",
               "RequestSync:Registrations",
               "RequestSync:Registration",
               "RequestSyncManager:Registrations",
               "RequestSyncManager:SetPolicy",
               "RequestSyncManager:RunTask" ],

  _pendingOperation: false,
  _pendingMessages: [],

  _registrations: {},

  _wifi: false,

  _activeTask: null,
  _queuedTasks: [],

  _timers: {},
  _pendingRequests: {},

  
  init: function() {
    debug("init");

    this._messages.forEach((function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }).bind(this));

    Services.obs.addObserver(this, 'xpcom-shutdown', false);
    Services.obs.addObserver(this, 'webapps-clear-data', false);
    Services.obs.addObserver(this, 'wifi-state-changed', false);

    this.initDBHelper("requestSync", RSYNCDB_VERSION, [RSYNCDB_NAME]);

    
    
    

    let self = this;
    this.dbTxn("readonly", function(aStore) {
      aStore.openCursor().onsuccess = function(event) {
        let cursor = event.target.result;
        if (cursor) {
          self.addRegistration(cursor.value);
          cursor.continue();
        }
      }
    },
    function() {
      debug("initialization done");
    },
    function() {
      dump("ERROR!! RequestSyncService - Failed to retrieve data from the database.\n");
    });
  },

  
  shutdown: function() {
    debug("shutdown");

    this._messages.forEach((function(msgName) {
      ppmm.removeMessageListener(msgName, this);
    }).bind(this));

    Services.obs.removeObserver(this, 'xpcom-shutdown');
    Services.obs.removeObserver(this, 'webapps-clear-data');
    Services.obs.removeObserver(this, 'wifi-state-changed');

    this.close();

    
    let self = this;
    this.forEachRegistration(function(aObj) {
      let key = self.principalToKey(aObj.principal);
      self.removeRegistrationInternal(aObj.data.task, key);
    });
  },

  observe: function(aSubject, aTopic, aData) {
    debug("observe");

    switch (aTopic) {
      case 'xpcom-shutdown':
        this.shutdown();
        break;

      case 'webapps-clear-data':
        this.clearData(aSubject);
        break;

      case 'wifi-state-changed':
        this.wifiStateChanged(aSubject == 'enabled');
        break;

      default:
        debug("Wrong observer topic: " + aTopic);
        break;
    }
  },

  
  clearData: function(aData) {
    debug('clearData');

    if (!aData) {
      return;
    }

    let params =
      aData.QueryInterface(Ci.mozIApplicationClearPrivateDataParams);
    if (!params) {
      return;
    }

    
    
    let partialKey = params.appId + '|' + params.browserOnly + '|';
    let dbKeys = [];

    for (let key  in this._registrations) {
      if (key.indexOf(partialKey) != 0) {
        continue;
      }

      for (let task in this._registrations[key]) {
        dbKeys = this._registrations[key][task].dbKey;
        this.removeRegistrationInternal(task, key);
      }
    }

    if (dbKeys.length == 0) {
      return;
    }

    
    this.dbTxn('readwrite', function(aStore) {
      for (let i = 0; i < dbKeys.length; ++i) {
        aStore.delete(dbKeys[i]);
      }
    },
    function() {
      debug("ClearData completed");
    }, function() {
      debug("ClearData failed");
    });
  },

  
  upgradeSchema: function(aTransaction, aDb, aOldVersion, aNewVersion) {
    debug('updateSchema');
    aDb.createObjectStore(RSYNCDB_NAME, { autoIncrement: true });
  },

  
  principalToKey: function(aPrincipal) {
    return aPrincipal.appId + '|' +
           aPrincipal.isInBrowserElement + '|' +
           aPrincipal.origin;
  },

  
  addRegistration: function(aObj) {
    debug('addRegistration');

    let key = this.principalToKey(aObj.principal);
    if (!(key in this._registrations)) {
      this._registrations[key] = {};
    }

    this.scheduleTimer(aObj);
    this._registrations[key][aObj.data.task] = aObj;
  },

  
  
  
  removeRegistration: function(aTaskName, aKey, aPrincipal) {
    debug('removeRegistration');

    if (!(aKey in this._registrations) ||
        !(aTaskName in this._registrations[aKey])) {
      return false;
    }

    
    if (!aPrincipal.equals(this._registrations[aKey][aTaskName].principal)) {
      return false;
    }

    this.removeRegistrationInternal(aTaskName, aKey);
    return true;
  },

  removeRegistrationInternal: function(aTaskName, aKey) {
    debug('removeRegistrationInternal');

    let obj = this._registrations[aKey][aTaskName];

    this.removeTimer(obj);

    
    this.removeTaskFromQueue(obj);

    
    
    obj.active = false;

    delete this._registrations[aKey][aTaskName];

    
    for (let key in this._registrations[aKey]) {
      return;
    }
    delete this._registrations[aKey];
  },

  removeTaskFromQueue: function(aObj) {
    let pos = this._queuedTasks.indexOf(aObj);
    if (pos != -1) {
      this._queuedTasks.splice(pos, 1);
    }
  },

  
  
  receiveMessage: function(aMessage) {
    debug("receiveMessage");

    
    if (this._pendingOperation) {
      this._pendingMessages.push(aMessage);
      return;
    }

    
    if (!aMessage.principal) {
      return;
    }

    let uri = Services.io.newURI(aMessage.principal.origin, null, null);

    let principal;
    try {
      principal = secMan.getAppCodebasePrincipal(uri,
        aMessage.principal.appId, aMessage.principal.isInBrowserElement);
    } catch(e) {
      return;
    }

    if (!principal) {
      return;
    }

    switch (aMessage.name) {
      case "RequestSync:Register":
        this.register(aMessage.target, aMessage.data, principal);
        break;

      case "RequestSync:Unregister":
        this.unregister(aMessage.target, aMessage.data, principal);
        break;

      case "RequestSync:Registrations":
        this.registrations(aMessage.target, aMessage.data, principal);
        break;

      case "RequestSync:Registration":
        this.registration(aMessage.target, aMessage.data, principal);
        break;

      case "RequestSyncManager:Registrations":
        this.managerRegistrations(aMessage.target, aMessage.data, principal);
        break;

      case "RequestSyncManager:SetPolicy":
        this.managerSetPolicy(aMessage.target, aMessage.data, principal);
        break;

      case "RequestSyncManager:RunTask":
        this.managerRunTask(aMessage.target, aMessage.data, principal);
        break;

      default:
        debug("Wrong message: " + aMessage.name);
        break;
    }
  },

  
  validateRegistrationParams: function(aParams) {
    if (aParams === null) {
      return false;
    }

    
    if (!("wakeUpPage" in aParams) ||
        aParams.wakeUpPage.length == 0) {
      return false;
    }

    let minInterval = RSYNC_MIN_INTERVAL;
    try {
      minInterval = Services.prefs.getIntPref("dom.requestSync.minInterval");
    } catch(e) {}

    if (!("minInterval" in aParams) ||
        aParams.minInterval < minInterval) {
      return false;
    }

    return true;
  },

  
  register: function(aTarget, aData, aPrincipal) {
    debug("register");

    if (!this.validateRegistrationParams(aData.params)) {
      aTarget.sendAsyncMessage("RequestSync:Register:Return",
                               { requestID: aData.requestID,
                                 error: "ParamsError" } );
      return;
    }

    let key = this.principalToKey(aPrincipal);
    if (key in this._registrations &&
        aData.task in this._registrations[key]) {
      
      this.removeRegistrationInternal(aData.task, key);
    }

    
    aData.params.task = aData.task;
    aData.params.lastSync = 0;
    aData.params.principal = aPrincipal;

    aData.params.state = RSYNC_STATE_ENABLED;
    if (aData.params.wifiOnly) {
      aData.params.state = RSYNC_STATE_WIFIONLY;
    }

    aData.params.overwrittenMinInterval = 0;

    let dbKey = aData.task + "|" +
                aPrincipal.appId + '|' +
                aPrincipal.isInBrowserElement + '|' +
                aPrincipal.origin;

    let data = { principal: aPrincipal,
                 dbKey: dbKey,
                 data: aData.params,
                 active: true };

    let self = this;
    this.dbTxn('readwrite', function(aStore) {
      aStore.put(data, data.dbKey);
    },
    function() {
      self.addRegistration(data);
      aTarget.sendAsyncMessage("RequestSync:Register:Return",
                               { requestID: aData.requestID });
    },
    function() {
      aTarget.sendAsyncMessage("RequestSync:Register:Return",
                               { requestID: aData.requestID,
                                 error: "IndexDBError" } );
    });
  },

  
  unregister: function(aTarget, aData, aPrincipal) {
    debug("unregister");

    let key = this.principalToKey(aPrincipal);
    if (!(key in this._registrations) ||
        !(aData.task in this._registrations[key])) {
      aTarget.sendAsyncMessage("RequestSync:Unregister:Return",
                               { requestID: aData.requestID,
                                 error: "UnknownTaskError" });
      return;
    }

    let dbKey = this._registrations[key][aData.task].dbKey;
    this.removeRegistration(aData.task, key, aPrincipal);

    let self = this;
    this.dbTxn('readwrite', function(aStore) {
      aStore.delete(dbKey);
    },
    function() {
      aTarget.sendAsyncMessage("RequestSync:Unregister:Return",
                               { requestID: aData.requestID });
    },
    function() {
      aTarget.sendAsyncMessage("RequestSync:Unregister:Return",
                               { requestID: aData.requestID,
                                 error: "IndexDBError" } );
    });
  },

  
  registrations: function(aTarget, aData, aPrincipal) {
    debug("registrations");

    let results = [];
    let key = this.principalToKey(aPrincipal);
    if (key in this._registrations) {
      for (let i in this._registrations[key]) {
        results.push(this.createPartialTaskObject(
          this._registrations[key][i].data));
      }
    }

    aTarget.sendAsyncMessage("RequestSync:Registrations:Return",
                             { requestID: aData.requestID,
                               results: results });
  },

  
  registration: function(aTarget, aData, aPrincipal) {
    debug("registration");

    let results = null;
    let key = this.principalToKey(aPrincipal);
    if (key in this._registrations &&
        aData.task in this._registrations[key]) {
      results = this.createPartialTaskObject(
        this._registrations[key][aData.task].data);
    }

    aTarget.sendAsyncMessage("RequestSync:Registration:Return",
                             { requestID: aData.requestID,
                               results: results });
  },

  
  managerRegistrations: function(aTarget, aData, aPrincipal) {
    debug("managerRegistrations");

    let results = [];
    let self = this;
    this.forEachRegistration(function(aObj) {
      results.push(self.createFullTaskObject(aObj.data));
    });

    aTarget.sendAsyncMessage("RequestSyncManager:Registrations:Return",
                             { requestID: aData.requestID,
                               results: results });
  },

  
  managerSetPolicy: function(aTarget, aData, aPrincipal) {
    debug("managerSetPolicy");

    let toSave = null;
    let self = this;
    this.forEachRegistration(function(aObj) {
      if (aObj.data.task != aData.task) {
        return;
      }

      if (aObj.principal.isInBrowserElement != aData.isInBrowserElement ||
          aObj.principal.origin != aData.origin) {
        return;
      }

      let app = appsService.getAppByLocalId(aObj.principal.appId);
      if (app && app.manifestURL != aData.manifestURL ||
          (!app && aData.manifestURL != "")) {
        return;
      }

      if ("overwrittenMinInterval" in aData) {
        aObj.data.overwrittenMinInterval = aData.overwrittenMinInterval;
      }

      aObj.data.state = aData.state;

      if (toSave) {
        dump("ERROR!! RequestSyncService - SetPolicy matches more than 1 task.\n");
        return;
      }

      toSave = aObj;
    });

    if (!toSave) {
      aTarget.sendAsyncMessage("RequestSyncManager:SetPolicy:Return",
                               { requestID: aData.requestID, error: "UnknownTaskError" });
      return;
    }

    this.updateObjectInDB(toSave, function() {
      self.scheduleTimer(toSave);
      aTarget.sendAsyncMessage("RequestSyncManager:SetPolicy:Return",
                               { requestID: aData.requestID });
    });
  },

  
  managerRunTask: function(aTarget, aData, aPrincipal) {
    debug("runTask");

    let task = null;
    this.forEachRegistration(function(aObj) {
      if (aObj.data.task != aData.task) {
        return;
      }

      if (aObj.principal.isInBrowserElement != aData.isInBrowserElement ||
          aObj.principal.origin != aData.origin) {
        return;
      }

      let app = appsService.getAppByLocalId(aObj.principal.appId);
      if (app && app.manifestURL != aData.manifestURL ||
          (!app && aData.manifestURL != "")) {
        return;
      }

      if (task) {
        dump("ERROR!! RequestSyncService - RunTask matches more than 1 task.\n");
        return;
      }

      task = aObj;
    });

    if (!task) {
      aTarget.sendAsyncMessage("RequestSyncManager:RunTask:Return",
                               { requestID: aData.requestID, error: "UnknownTaskError" });
      return;
    }

    
    this.storePendingRequest(task, aTarget, aData.requestID);
    this.timeout(task);
  },

  
  
  createPartialTaskObject: function(aObj) {
    return { task: aObj.task,
             lastSync: aObj.lastSync,
             oneShot: aObj.oneShot,
             minInterval: aObj.minInterval,
             wakeUpPage: aObj.wakeUpPage,
             wifiOnly: aObj.wifiOnly,
             data: aObj.data };
  },

  createFullTaskObject: function(aObj) {
    let obj = this.createPartialTaskObject(aObj);

    obj.app = { manifestURL: '',
                origin: aObj.principal.origin,
                isInBrowserElement: aObj.principal.isInBrowserElement };

    let app = appsService.getAppByLocalId(aObj.principal.appId);
    if (app) {
      obj.app.manifestURL = app.manifestURL;
    }

    obj.state = aObj.state;
    obj.overwrittenMinInterval = aObj.overwrittenMinInterval;
    return obj;
  },

  
  scheduleTimer: function(aObj) {
    debug("scheduleTimer");

    this.removeTimer(aObj);

    
    if (!aObj.active) {
      return;
    }

    if (aObj.data.state == RSYNC_STATE_DISABLED) {
      return;
    }

    
    if (aObj.data.state == RSYNC_STATE_WIFIONLY && !this._wifi) {
      return;
    }

    this.createTimer(aObj);
  },

  timeout: function(aObj) {
    debug("timeout");

    if (this._activeTask) {
      debug("queueing tasks");
      
      if (this._queuedTasks.indexOf(aObj) == -1) {
        this._queuedTasks.push(aObj);
      }
      return;
    }

    let app = appsService.getAppByLocalId(aObj.principal.appId);
    if (!app) {
      dump("ERROR!! RequestSyncService - Failed to retrieve app data from a principal.\n");
      aObj.active = false;
      this.updateObjectInDB(aObj);
      return;
    }

    let manifestURL = Services.io.newURI(app.manifestURL, null, null);
    let pageURL = Services.io.newURI(aObj.data.wakeUpPage, null, aObj.principal.URI);

    
    if (this.hasPendingMessages('request-sync', manifestURL, pageURL)) {
      this.scheduleTimer(aObj);
      return;
    }

    this.removeTimer(aObj);
    this._activeTask = aObj;

    if (!manifestURL || !pageURL) {
      dump("ERROR!! RequestSyncService - Failed to create URI for the page or the manifest\n");
      aObj.active = false;
      this.updateObjectInDB(aObj);
      return;
    }

    
    
    
    

    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    let done = false;
    let self = this;
    function taskCompleted() {
      debug("promise or timeout for task calls taskCompleted");

      if (!done) {
        done = true;
        self.operationCompleted();
      }

      timer.cancel();
      timer = null;
    }

    let timeout = RSYNC_OPERATION_TIMEOUT;
    try {
      let tmp = Services.prefs.getIntPref("dom.requestSync.maxTaskTimeout");
      timeout = tmp;
    } catch(e) {}

    timer.initWithCallback(function() {
      debug("Task is taking too much, let's ignore the promise.");
      taskCompleted();
    }, timeout, Ci.nsITimer.TYPE_ONE_SHOT);

    
    debug("Sending message.");
    let promise =
      systemMessenger.sendMessage('request-sync',
                                  this.createPartialTaskObject(aObj.data),
                                  pageURL, manifestURL);

    promise.then(function() {
      debug("promise resolved");
      taskCompleted();
    }, function() {
      debug("promise rejected");
      taskCompleted();
    });
  },

  operationCompleted: function() {
    debug("operationCompleted");

    if (!this._activeTask) {
      dump("ERROR!! RequestSyncService - OperationCompleted called without an active task\n");
      return;
    }

    
    this._activeTask.active = !this._activeTask.data.oneShot;
    this._activeTask.data.lastSync = new Date();

    let pendingRequests = this.stealPendingRequests(this._activeTask);
    for (let i = 0; i < pendingRequests.length; ++i) {
      pendingRequests[i]
          .target.sendAsyncMessage("RequestSyncManager:RunTask:Return",
                                   { requestID: pendingRequests[i].requestID });
    }

    let self = this;
    this.updateObjectInDB(this._activeTask, function() {
      
      
      
      if (!self._activeTask.data.oneShot) {
        self.scheduleTimer(self._activeTask);
      }

      self.processNextTask();
    });
  },

  processNextTask: function() {
    debug("processNextTask");

    this._activeTask = null;

    if (this._queuedTasks.length == 0) {
      return;
    }

    let task = this._queuedTasks.shift();
    this.timeout(task);
  },

  hasPendingMessages: function(aMessageName, aManifestURL, aPageURL) {
    let hasPendingMessages =
      cpmm.sendSyncMessage("SystemMessageManager:HasPendingMessages",
                           { type: aMessageName,
                             pageURL: aPageURL.spec,
                             manifestURL: aManifestURL.spec })[0];

    debug("Pending messages: " + hasPendingMessages);
    return hasPendingMessages;
  },

  
  updateObjectInDB: function(aObj, aCb) {
    debug("updateObjectInDB");

    this.dbTxn('readwrite', function(aStore) {
      aStore.put(aObj, aObj.dbKey);
    },
    function() {
      if (aCb) {
        aCb();
      }
      debug("UpdateObjectInDB completed");
    }, function() {
      debug("UpdateObjectInDB failed");
    });
  },

  pendingOperationStarted: function() {
    debug('pendingOperationStarted');
    this._pendingOperation = true;
  },

  pendingOperationDone: function() {
    debug('pendingOperationDone');

    this._pendingOperation = false;

    
    while (this._pendingMessages.length && !this._pendingOperation) {
      this.receiveMessage(this._pendingMessages.shift());
    }
  },

  
  
  dbTxn: function(aType, aCb, aSuccessCb, aErrorCb) {
    debug('dbTxn');

    this.pendingOperationStarted();

    let self = this;
    this.newTxn(aType, RSYNCDB_NAME, function(aTxn, aStore) {
      aCb(aStore);
    },
    function() {
      self.pendingOperationDone();
      aSuccessCb();
    },
    function() {
      self.pendingOperationDone();
      aErrorCb();
    });
  },

  forEachRegistration: function(aCb) {
    
    
    let list = [];
    for (let key in this._registrations) {
      for (let task in this._registrations[key]) {
        list.push(this._registrations[key][task]);
      }
    }

    for (let i = 0; i < list.length; ++i) {
      aCb(list[i]);
    }
  },

  wifiStateChanged: function(aEnabled) {
    debug("onWifiStateChanged");
    this._wifi = aEnabled;

    if (!this._wifi) {
      
      let self = this;
      this.forEachRegistration(function(aObj) {
        if (aObj.data.state == RSYNC_STATE_WIFIONLY && self.hasTimer(aObj)) {
          self.removeTimer(aObj);

          
          self.removeTaskFromQueue(aObj);
        }
      });
      return;
    }

    
    let self = this;
    this.forEachRegistration(function(aObj) {
      if (aObj.active && !self.hasTimer(aObj)) {
        if (!aObj.data.wifiOnly) {
          dump("ERROR - Found a disabled task that is not wifiOnly.");
        }

        self.scheduleTimer(aObj);
      }
    });
  },

  createTimer: function(aObj) {
    this._timers[aObj.dbKey] = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    let interval = aObj.data.minInterval;
    if (aObj.data.overwrittenMinInterval > 0) {
      interval = aObj.data.overwrittenMinInterval;
    }

    let self = this;
    this._timers[aObj.dbKey].initWithCallback(function() { self.timeout(aObj); },
                                              interval * 1000,
                                              Ci.nsITimer.TYPE_ONE_SHOT);
  },

  hasTimer: function(aObj) {
    return (aObj.dbKey in this._timers);
  },

  removeTimer: function(aObj) {
    if (aObj.dbKey in this._timers) {
      this._timers[aObj.dbKey].cancel();
      delete this._timers[aObj.dbKey];
    }
  },

  storePendingRequest: function(aObj, aTarget, aRequestID) {
    if (!(aObj.dbKey in this._pendingRequests)) {
      this._pendingRequests[aObj.dbKey] = [];
    }

    this._pendingRequests[aObj.dbKey].push({ target: aTarget,
                                             requestID: aRequestID });
  },

  stealPendingRequests: function(aObj) {
    if (!(aObj.dbKey in this._pendingRequests)) {
      return [];
    }

    let requests = this._pendingRequests[aObj.dbKey];
    delete this._pendingRequests[aObj.dbKey];
    return requests;
  }
}

RequestSyncService.init();

this.EXPORTED_SYMBOLS = [""];
