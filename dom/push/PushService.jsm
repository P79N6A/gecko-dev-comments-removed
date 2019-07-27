




"use strict";


let gDebuggingEnabled = true;

function debug(s) {
  if (gDebuggingEnabled)
    dump("-*- PushService.jsm: " + s + "\n");
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const {PushDB} = Cu.import("resource://gre/modules/PushDB.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

const {PushServiceWebSocket} = Cu.import("resource://gre/modules/PushServiceWebSocket.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AlarmService",
                                  "resource://gre/modules/AlarmService.jsm");

this.EXPORTED_SYMBOLS = ["PushService"];

const prefs = new Preferences("dom.push.");

gDebuggingEnabled = prefs.get("debug");

const kPUSHWSDB_DB_NAME = "pushapi";
const kPUSHWSDB_DB_VERSION = 1; 
const kPUSHWSDB_STORE_NAME = "pushapi";

const kCHILD_PROCESS_MESSAGES = ["Push:Register", "Push:Unregister",
                                 "Push:Registration"];

var upgradeSchemaWS = function(aTransaction, aDb, aOldVersion, aNewVersion, aDbInstance) {
  debug("upgradeSchemaWS()");

  let objectStore = aDb.createObjectStore(aDbInstance._dbStoreName,
                                          { keyPath: "channelID" });

  
  objectStore.createIndex("pushEndpoint", "pushEndpoint", { unique: true });

  
  
  objectStore.createIndex("scope", "scope", { unique: true });
};






this.PushService = {
  _service: null,

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      



      case "xpcom-shutdown":
        this.uninit();
        break;
      case "network-active-changed":         
      case "network:offline-status-changed": 
        this._service.observeNetworkChange(aSubject, aTopic, aData);
        break;
      case "nsPref:changed":
        if (aData == "dom.push.serverURL") {
          debug("dom.push.serverURL changed! websocket. new value " +
                prefs.get("serverURL"));
          this._service.shutdownService();
        } else if (aData == "dom.push.connection.enabled") {
          this._service.observePushConnectionPref(prefs.get("connection.enabled"));
        } else if (aData == "dom.push.debug") {
          gDebuggingEnabled = prefs.get("debug");
          this._service.observeDebug(gDebuggingEnabled);
          this._db.observeDebug(gDebuggingEnabled);
        }
        break;
      case "timer-callback":
        this._service.observeTimer(aSubject, aTopic, aData);
        break;
      case "webapps-clear-data":
        debug("webapps-clear-data");

        let data = aSubject.QueryInterface(Ci.mozIApplicationClearPrivateDataParams);
        if (!data) {
          debug("webapps-clear-data: Failed to get information about application");
          return;
        }

        
        
        let appsService = Cc["@mozilla.org/AppsService;1"]
                            .getService(Ci.nsIAppsService);
        let scope = appsService.getScopeByLocalId(data.appId);
        if (!scope) {
          debug("webapps-clear-data: No scope found for " + data.appId);
          return;
        }

        this._db.getByScope(scope)
          .then(record =>
            this._db.delete(records.channelID)
              .then(_ =>
                this._service.unregister(records),
                err => {
                  debug("webapps-clear-data: " + scope +
                        " Could not delete entry " + records.channelID);

                this._service.unregister(records)
                throw "Database error";
              })
          , _ => {
            debug("webapps-clear-data: Error in getByScope(" + scope + ")");
          });

        break;
    }
  },

  
  getNetworkStateChangeEventName: function() {
    try {
      Cc["@mozilla.org/network/manager;1"].getService(Ci.nsINetworkManager);
      return "network-active-changed";
    } catch (e) {
      return "network:offline-status-changed";
    }
  },

  init: function(options = {}) {
    debug("init()");
    if (this._started) {
      return;
    }

    var globalMM = Cc["@mozilla.org/globalmessagemanager;1"]
               .getService(Ci.nsIFrameScriptLoader);

    globalMM.loadFrameScript("chrome://global/content/PushServiceChildPreload.js", true);

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                 .getService(Ci.nsIMessageBroadcaster);

    kCHILD_PROCESS_MESSAGES.forEach(function addMessage(msgName) {
        ppmm.addMessageListener(msgName, this);
    }.bind(this));

    this._alarmID = null;

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, "webapps-clear-data", false);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    this._networkStateChangeEventName = this.getNetworkStateChangeEventName();
    Services.obs.addObserver(this, this._networkStateChangeEventName, false);

    
    
    prefs.observe("serverURL", this);
    
    prefs.observe("connection.enabled", this);
    
    prefs.observe("debug", this);
    this._db = options.db;
    if (!this._db) {
      this._db = new PushDB(kPUSHWSDB_DB_NAME, kPUSHWSDB_DB_VERSION, kPUSHWSDB_STORE_NAME, upgradeSchemaWS);
    }
    this._service = PushServiceWebSocket;
    this._service.init(options, this);
    this._service.startListeningIfChannelsPresent();
    this._started = true;
  },

  uninit: function() {
    if (!this._started)
      return;

    debug("uninit()");

    prefs.ignore("debug", this);
    prefs.ignore("connection.enabled", this);
    prefs.ignore("serverURL", this);
    Services.obs.removeObserver(this, this._networkStateChangeEventName);
    Services.obs.removeObserver(this, "webapps-clear-data", false);
    Services.obs.removeObserver(this, "xpcom-shutdown", false);

    
    
    
    this.stopAlarm();

    if (this._db) {
      this._db.close();
      this._db = null;
    }

    this._service.uninit();

    this._started = false;
    debug("shutdown complete!");
  },

  getServerURI: function() {
    let serverURL = prefs.get("serverURL");
    if (!serverURL) {
      debug("No dom.push.serverURL found!");
      return;
    }

    let uri;
    try {
      uri = Services.io.newURI(serverURL, null, null);
    } catch(e) {
      debug("Error creating valid URI from dom.push.serverURL (" +
            serverURL + ")");
      return;
    }
    return uri;
  },

  
  setAlarm: function(delay) {
    
    
    if (this._settingAlarm) {
        
        
        this._queuedAlarmDelay = delay;
        this._waitingForAlarmSet = true;
        return;
    }

    
    this.stopAlarm();

    this._settingAlarm = true;
    AlarmService.add(
      {
        date: new Date(Date.now() + delay),
        ignoreTimezone: true
      },
      this._service.onAlarmFired.bind(this._service),
      function onSuccess(alarmID) {
        this._alarmID = alarmID;
        debug("Set alarm " + delay + " in the future " + this._alarmID);
        this._settingAlarm = false;

        if (this._waitingForAlarmSet) {
          this._waitingForAlarmSet = false;
          this.setAlarm(this._queuedAlarmDelay);
        }
      }.bind(this)
    )
  },

  stopAlarm: function() {
    if (this._alarmID !== null) {
      debug("Stopped existing alarm " + this._alarmID);
      AlarmService.remove(this._alarmID);
      this._alarmID = null;
    }
  },

  
  
  notifyAllAppsRegister: function() {
    debug("notifyAllAppsRegister()");
    
    return this._db.getAllChannelIDs()
      .then(records => {
        let scopes = new Set();
        for (let record of records) {
          scopes.add(record.scope);
        }
        let globalMM = Cc['@mozilla.org/globalmessagemanager;1'].getService(Ci.nsIMessageListenerManager);
        for (let scope of scopes) {
          
          Services.obs.notifyObservers(
            null,
            "push-subscription-change",
            scope
          );

          let data = {
            originAttributes: {}, 
            scope: scope
          };

          globalMM.broadcastAsyncMessage('pushsubscriptionchange', data);
        }
      });
  },

  notifyApp: function(aPushRecord) {
    if (!aPushRecord || !aPushRecord.scope) {
      debug("notifyApp() something is undefined.  Dropping notification: "
        + JSON.stringify(aPushRecord) );
      return;
    }

    debug("notifyApp() " + aPushRecord.scope);
    let scopeURI = Services.io.newURI(aPushRecord.scope, null, null);
    
    let notification = Cc["@mozilla.org/push/ObserverNotification;1"]
                         .createInstance(Ci.nsIPushObserverNotification);
    notification.pushEndpoint = aPushRecord.pushEndpoint;
    notification.version = aPushRecord.version;
    notification.data = "";
    notification.lastPush = aPushRecord.lastPush;
    notification.pushCount = aPushRecord.pushCount;

    Services.obs.notifyObservers(
      notification,
      "push-notification",
      aPushRecord.scope
    );

    
    if(Services.perms.testExactPermission(scopeURI, "push") != Ci.nsIPermissionManager.ALLOW_ACTION) {
      debug("Does not have permission for push.")
      return;
    }

    
    let data = {
      payload: "Short as life is, we make it still shorter by the careless waste of time.",
      originAttributes: {}, 
      scope: aPushRecord.scope
    };

    let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
                 .getService(Ci.nsIMessageListenerManager);
    globalMM.broadcastAsyncMessage('push', data);
  },

  updatePushRecord: function(aPushRecord) {
    debug("updatePushRecord()");
    return this._db.put(aPushRecord);
  },

  getByChannelID: function(aChannelID) {
    return this._db.getByChannelID(aChannelID)
  },

  getAllChannelIDs: function() {
    return this._db.getAllChannelIDs()
  },

  dropRegistration: function() {
    return this._db.drop();
  },

    



  _registerWithServer: function(channelID, aPageRecord) {
    debug("registerWithServer()");

    return this._service.register(channelID)
      .then(
        this._onRegisterSuccess.bind(this, aPageRecord, channelID),
        this._onRegisterError.bind(this)
      );
  },

  _generateID: function() {
    let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                          .getService(Ci.nsIUUIDGenerator);
    
    return uuidGenerator.generateUUID().toString().slice(1, -1);
  },

  _register: function(aPageRecord) {
    return this._db.getByScope(aPageRecord.scope)
      .then(pushRecord => {
        if (pushRecord == null) {
          let channelID = this._generateID();
          return this._registerWithServer(channelID, aPageRecord);
        }
        return pushRecord;
      },
      error => {
        debug("getByScope failed");
        throw "Database error";
      }
    );
  },

  



  _onRegisterSuccess: function(aPageRecord, generatedChannelID, data) {
    debug("_onRegisterSuccess()");

    if (typeof data.channelID !== "string") {
      debug("Invalid channelID " + data.channelID);
      throw "Invalid channelID received";
    }
    else if (data.channelID != generatedChannelID) {
      debug("Server replied with different channelID " + data.channelID +
            " than what UA generated " + generatedChannelID);
      throw "Server sent 200 status code but different channelID";
    }

    try {
      Services.io.newURI(data.pushEndpoint, null, null);
    }
    catch (e) {
      debug("Invalid pushEndpoint " + data.pushEndpoint);
      throw "Invalid pushEndpoint " + data.pushEndpoint;
    }

    let record = {
      channelID: data.channelID,
      pushEndpoint: data.pushEndpoint,
      pageURL: aPageRecord.pageURL,
      scope: aPageRecord.scope,
      pushCount: 0,
      lastPush: 0,
      version: null
    };

    debug("scope in _onRegisterSuccess: " + aPageRecord.scope)

    return this.updatePushRecord(record)
      .then(
        function() {
          return record;
        },
        function(error) {
          
          this._service.unregister(record);
          throw error;
        }.bind(this)
      );
  },

  



  _onRegisterError: function(reply) {
    debug("_onRegisterError()");
    if (!reply.error) {
      debug("Called without valid error message!");
      throw "Registration error";
    }
    throw reply.error;
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage(): " + aMessage.name);

    if (kCHILD_PROCESS_MESSAGES.indexOf(aMessage.name) == -1) {
      debug("Invalid message from child " + aMessage.name);
      return;
    }

    let mm = aMessage.target.QueryInterface(Ci.nsIMessageSender);
    let json = aMessage.data;
    this[aMessage.name.slice("Push:".length).toLowerCase()](json, mm);
  },

  register: function(aPageRecord, aMessageManager) {
    debug("register(): " + JSON.stringify(aPageRecord));

    this._register(aPageRecord).then(
      function(aPageRecord, aMessageManager, pushRecord) {
        let message = {
          requestID: aPageRecord.requestID,
          pushEndpoint: pushRecord.pushEndpoint
        };
        aMessageManager.sendAsyncMessage("PushService:Register:OK", message);
      }.bind(this, aPageRecord, aMessageManager),
      function(error) {
        let message = {
          requestID: aPageRecord.requestID,
          error
        };
        aMessageManager.sendAsyncMessage("PushService:Register:KO", message);
      }
    );
  },

  























  _unregister: function(aPageRecord) {
    debug("unregisterWithServer()");

    if (!aPageRecord.scope) {
      return Promise.reject("NotFoundError");
    }

    return this._db.getByScope(aPageRecord.scope)
      .then(record => {
        
        if (record === undefined) {
          throw "NotFoundError";
        }

        this._db.delete(record.channelID)
          .then(_ =>
            
            
            this._service.unregister(record)
          );
      });
  },

  unregister: function(aPageRecord, aMessageManager) {
    debug("unregister() " + JSON.stringify(aPageRecord));

    this._unregister(aPageRecord).then(
      () => {
        aMessageManager.sendAsyncMessage("PushService:Unregister:OK", {
          requestID: aPageRecord.requestID,
          pushEndpoint: aPageRecord.pushEndpoint
        });
      },
      error => {
        aMessageManager.sendAsyncMessage("PushService:Unregister:KO", {
          requestID: aPageRecord.requestID,
          error
        });
      }
    );
  },

  _clearAll: function _clearAll() {
    return this._db.clearAll();
  },

  


  _registration: function(aPageRecord) {
    if (!aPageRecord.scope) {
      return Promise.reject("Database error");
    }

    return this._db.getByScope(aPageRecord.scope)
        .then(pushRecord => {
          let registration = null;
          if (pushRecord) {
            registration = {
              pushEndpoint: pushRecord.pushEndpoint,
              version: pushRecord.version,
              lastPush: pushRecord.lastPush,
              pushCount: pushRecord.pushCount
            };
          }
          return registration;
        }, _ => {
          throw "Database error";
        });
  },

  registration: function(aPageRecord, aMessageManager) {
    debug("registration()");

    return this._registration(aPageRecord).then(
      registration => {
        aMessageManager.sendAsyncMessage("PushService:Registration:OK", {
          requestID: aPageRecord.requestID,
          registration
        });
      },
      error => {
        aMessageManager.sendAsyncMessage("PushService:Registration:KO", {
          requestID: aPageRecord.requestID,
          error
        });
      }
    );
  }
};
