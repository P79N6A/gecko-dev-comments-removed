




"use strict";


let gDebuggingEnabled = true;

function debug(s) {
  if (gDebuggingEnabled) {
    dump("-*- PushService.jsm: " + s + "\n");
  }
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
const {PushServiceHttp2} = Cu.import("resource://gre/modules/PushServiceHttp2.jsm");


const CONNECTION_PROTOCOLS = [PushServiceWebSocket, PushServiceHttp2];

XPCOMUtils.defineLazyModuleGetter(this, "AlarmService",
                                  "resource://gre/modules/AlarmService.jsm");

this.EXPORTED_SYMBOLS = ["PushService"];

const prefs = new Preferences("dom.push.");

gDebuggingEnabled = prefs.get("debug");

const kCHILD_PROCESS_MESSAGES = ["Push:Register", "Push:Unregister",
                                 "Push:Registration"];

const PUSH_SERVICE_UNINIT = 0;
const PUSH_SERVICE_INIT = 1; 
const PUSH_SERVICE_ACTIVATING = 2;
const PUSH_SERVICE_CONNECTION_DISABLE = 3;
const PUSH_SERVICE_ACTIVE_OFFLINE = 4;
const PUSH_SERVICE_RUNNING = 5;


















const STARTING_SERVICE_EVENT = 0;
const CHANGING_SERVICE_EVENT = 1;
const STOPPING_SERVICE_EVENT = 2;
const UNINIT_EVENT = 3;






this.PushService = {
  _service: null,
  _state: PUSH_SERVICE_UNINIT,
  _db: null,
  _options: null,

  
  
  _serverURIProcessQueue: null,
  _serverURIProcessEnqueue: function(op) {
    if (!this._serverURIProcessQueue) {
      this._serverURIProcessQueue = Promise.resolve();
    }

    this._serverURIProcessQueue = this._serverURIProcessQueue
                                    .then(op)
                                    .catch(_ => {});
  },

  
  
  
  _pendingRegisterRequest: {},
  _notifyActivated: null,
  _activated: null,
  _checkActivated: function() {
    if (this._state < PUSH_SERVICE_ACTIVATING) {
      return Promise.reject({state: 0, error: "Service not active"});
    } else if (this._state > PUSH_SERVICE_ACTIVATING) {
      return Promise.resolve();
    } else {
      return (this._activated) ? this._activated :
                                 this._activated = new Promise((res, rej) =>
                                   this._notifyActivated = {resolve: res,
                                                            reject: rej});
    }
  },

  _setState: function(aNewState) {
    debug("new state: " + aNewState + " old state: " + this._state);

    if (this._state == aNewState) {
      return;
    }

    if (this._state == PUSH_SERVICE_ACTIVATING) {
      
      
      this._state = aNewState;
      if (this._notifyActivated) {
        if (aNewState < PUSH_SERVICE_ACTIVATING) {
          this._notifyActivated.reject({state: 0, error: "Service not active"});
        } else {
          this._notifyActivated.resolve();
        }
      }
      this._notifyActivated = null;
      this._activated = null;
    }
    this._state = aNewState;
  },

  _changeStateOfflineEvent: function(offline, calledFromConnEnabledEvent) {
    debug("changeStateOfflineEvent: " + offline);

    if (this._state < PUSH_SERVICE_ACTIVE_OFFLINE &&
        this._state != PUSH_SERVICE_ACTIVATING &&
        !calledFromConnEnabledEvent) {
      return;
    }

    if (offline) {
      if (this._state == PUSH_SERVICE_RUNNING) {
        this._service.disconnect();
      }
      this._setState(PUSH_SERVICE_ACTIVE_OFFLINE);
    } else {
      if (this._state == PUSH_SERVICE_RUNNING) {
        
        
        
        this._service.disconnect();
      }
      this._db.getAllKeyIDs()
        .then(keyIDs => {
          if (keyIDs.length > 0) {
            
            this._service.connect(keyIDs);
          }
        });
      this._setState(PUSH_SERVICE_RUNNING);
    }
  },

  _changeStateConnectionEnabledEvent: function(enabled) {
    debug("changeStateConnectionEnabledEvent: " + enabled);

    if (this._state < PUSH_SERVICE_CONNECTION_DISABLE &&
        this._state != PUSH_SERVICE_ACTIVATING) {
      return;
    }

    if (enabled) {
      this._changeStateOfflineEvent(Services.io.offline, true);
    } else {
      if (this._state == PUSH_SERVICE_RUNNING) {
        this._service.disconnect();
      }
      this._setState(PUSH_SERVICE_CONNECTION_DISABLE);
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      




      case "xpcom-shutdown":
        this.uninit();
        break;
      case "network-active-changed":         
      case "network:offline-status-changed": 
        this._changeStateOfflineEvent(aData === "offline", false);
        break;

      case "nsPref:changed":
        if (aData == "dom.push.serverURL") {
          debug("dom.push.serverURL changed! websocket. new value " +
                prefs.get("serverURL"));
          this._serverURIProcessEnqueue(_ =>
            this._changeServerURL(prefs.get("serverURL"),
                                  CHANGING_SERVICE_EVENT)
          );

        } else if (aData == "dom.push.connection.enabled") {
          this._changeStateConnectionEnabledEvent(prefs.get("connection.enabled"));

        } else if (aData == "dom.push.debug") {
          gDebuggingEnabled = prefs.get("debug");
        }
        break;

      case "webapps-clear-data":
        debug("webapps-clear-data");

        let data = aSubject
                     .QueryInterface(Ci.mozIApplicationClearPrivateDataParams);
        if (!data) {
          debug("webapps-clear-data: Failed to get information about " +
                "application");
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
            Promise.all([
              this._db.delete(this._service.getKeyFromRecord(record)),
              this._sendRequest("unregister", record)
            ])
          ).catch(_ => {
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

  _findService: function(serverURI) {
    var uri;
    var service;
    if (serverURI) {
      for (let connProtocol of CONNECTION_PROTOCOLS) {
        uri = connProtocol.checkServerURI(serverURI);
        if (uri) {
          service = connProtocol;
          break;
        }
      }
    }
    return [service, uri];
  },

  _changeServerURL: function(serverURI, event) {
    debug("changeServerURL");

    switch(event) {
      case UNINIT_EVENT:
        return this._stopService(event);

      case STARTING_SERVICE_EVENT:
      {
        let [service, uri] = this._findService(serverURI);
        if (!service) {
          this._setState(PUSH_SERVICE_INIT);
          return Promise.resolve();
        }
        return this._startService(service, uri, event)
          .then(_ =>
            this._changeStateConnectionEnabledEvent(prefs.get("connection.enabled"))
          );
      }
      case CHANGING_SERVICE_EVENT:
        let [service, uri] = this._findService(serverURI);
        if (service) {
          if (this._state == PUSH_SERVICE_INIT) {
            this._setState(PUSH_SERVICE_ACTIVATING);
            
            return this._startService(service, uri, STARTING_SERVICE_EVENT)
              .then(_ =>
                this._changeStateConnectionEnabledEvent(prefs.get("connection.enabled"))
              );

          } else {
            this._setState(PUSH_SERVICE_ACTIVATING);
            
            
            
            return this._stopService(CHANGING_SERVICE_EVENT)
              .then(_ =>
                 this._startService(service, uri, CHANGING_SERVICE_EVENT)
              )
              .then(_ =>
                this._changeStateConnectionEnabledEvent(prefs.get("connection.enabled"))
              );

          }
        } else {
          if (this._state == PUSH_SERVICE_INIT) {
            return Promise.resolve();

          } else {
            
            this._setState(PUSH_SERVICE_INIT);
            return this._stopService(STOPPING_SERVICE_EVENT);
          }
        }
    }
  },

  














  init: function(options = {}) {
    debug("init()");

    if (this._state > PUSH_SERVICE_UNINIT) {
      return;
    }

    this._setState(PUSH_SERVICE_ACTIVATING);

    var globalMM = Cc["@mozilla.org/globalmessagemanager;1"]
                     .getService(Ci.nsIFrameScriptLoader);

    globalMM.loadFrameScript("chrome://global/content/PushServiceChildPreload.js",
                             true);

    
    prefs.observe("debug", this);

    Services.obs.addObserver(this, "xpcom-shutdown", false);

    if (options.serverURI) {
      

      var uri;
      var service;
      if (!options.service) {
        for (let connProtocol of CONNECTION_PROTOCOLS) {
          uri = connProtocol.checkServerURI(options.serverURI);
          if (uri) {
            service = connProtocol;
            break;
          }
        }
      } else {
        try {
          uri  = Services.io.newURI(options.serverURI, null, null);
          service = options.service;
        } catch(e) {}
      }
      if (!service) {
        this._setState(PUSH_SERVICE_INIT);
        return;
      }

      
      this._startService(service, uri, false, options);
      
      
      this._changeStateConnectionEnabledEvent(prefs.get("connection.enabled"));

    } else {
      
      
      prefs.observe("serverURL", this);

      this._serverURIProcessEnqueue(_ =>
        this._changeServerURL(prefs.get("serverURL"), STARTING_SERVICE_EVENT));
    }
  },

  _startObservers: function() {
    debug("startObservers");

    if (this._state != PUSH_SERVICE_ACTIVATING) {
      return;
    }

    this._alarmID = null;

    Services.obs.addObserver(this, "webapps-clear-data", false);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    this._networkStateChangeEventName = this.getNetworkStateChangeEventName();
    Services.obs.addObserver(this, this._networkStateChangeEventName, false);

    
    prefs.observe("connection.enabled", this);
  },

  _startService: function(service, serverURI, event, options = {}) {
    debug("startService");

    if (this._state != PUSH_SERVICE_ACTIVATING) {
      return;
    }

    if (event != CHANGING_SERVICE_EVENT) {
      
      
      let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                   .getService(Ci.nsIMessageBroadcaster);

      kCHILD_PROCESS_MESSAGES.forEach(msgName =>
        ppmm.addMessageListener(msgName, this)
      );
    }

    this._service = service;

    this._db = options.db;
    if (!this._db) {
      this._db = this._service.newPushDB();
    }

    this._service.init(options, this, serverURI);
    this._startObservers();
    return Promise.resolve();
  },

  








  _stopService: function(event) {
    debug("stopService");

    if (this._state < PUSH_SERVICE_ACTIVATING) {
      return;
    }

    this.stopAlarm();
    this._stopObservers();

    if (event != CHANGING_SERVICE_EVENT) {
      let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                   .getService(Ci.nsIMessageBroadcaster);

      kCHILD_PROCESS_MESSAGES.forEach(
        msgName => ppmm.removeMessageListener(msgName, this)
      );
    }

    this._service.disconnect();
    this._service.uninit();
    this._service = null;
    this.stopAlarm();

    if (!this._db) {
      return Promise.resolve();
    }
    if (event == UNINIT_EVENT) {
      
      this._db.close();
      this._db = null;
      return Promise.resolve();
    }

    return this.dropRegistrations()
       .then(_ => {
         this._db.close();
         this._db = null;
       }, err => {
         this._db.close();
         this._db = null;
       });
  },

  _stopObservers: function() {
    debug("stopObservers()");

    if (this._state < PUSH_SERVICE_ACTIVATING) {
      return;
    }

    prefs.ignore("debug", this);
    prefs.ignore("connection.enabled", this);

    Services.obs.removeObserver(this, this._networkStateChangeEventName);
    Services.obs.removeObserver(this, "webapps-clear-data", false);
  },

  uninit: function() {
    debug("uninit()");

    if (this._state == PUSH_SERVICE_UNINIT) {
      return;
    }

    this._setState(PUSH_SERVICE_UNINIT);

    prefs.ignore("serverURL", this);
    Services.obs.removeObserver(this, "xpcom-shutdown", false);

    this._serverURIProcessEnqueue(_ =>
            this._changeServerURL("", UNINIT_EVENT));
    debug("shutdown complete!");
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
      () => {
        if (this._state > PUSH_SERVICE_ACTIVATING) {
          this._service.onAlarmFired();
        }
      }, (alarmID) => {
        this._alarmID = alarmID;
        debug("Set alarm " + delay + " in the future " + this._alarmID);
        this._settingAlarm = false;

        if (this._waitingForAlarmSet) {
          this._waitingForAlarmSet = false;
          this.setAlarm(this._queuedAlarmDelay);
        }
      }
    );
  },

  stopAlarm: function() {
    if (this._alarmID !== null) {
      debug("Stopped existing alarm " + this._alarmID);
      AlarmService.remove(this._alarmID);
      this._alarmID = null;
    }
  },

  dropRegistrations: function() {
    return this._notifyAllAppsRegister()
      .then(_ => this._db.drop());
  },

  
  
  _notifyAllAppsRegister: function() {
    debug("notifyAllAppsRegister()");
    
    return this._db.getAllKeyIDs()
      .then(records => {
        let scopes = new Set();
        for (let record of records) {
          scopes.add(record.scope);
        }
        let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
                         .getService(Ci.nsIMessageListenerManager);
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

  dropRegistrationAndNotifyApp: function(aKeyId) {
    return this._db.getByKeyID(aKeyId)
      .then(record => {
        let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
                         .getService(Ci.nsIMessageListenerManager);
        Services.obs.notifyObservers(
          null,
          "push-subscription-change",
          record.scope
        );

        let data = {
          originAttributes: {}, 
          scope: record.scope
        };

        globalMM.broadcastAsyncMessage('pushsubscriptionchange', data);
      })
      .then(_ => this._db.delete(aKeyId));
  },

  updateRegistrationAndNotifyApp: function(aOldKey, aRecord) {
    return this._db.delete(aOldKey)
      .then(_ => this._db.put(aRecord)
        .then(record => {
          let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
                           .getService(Ci.nsIMessageListenerManager);
          Services.obs.notifyObservers(
            null,
            "push-subscription-change",
            record.scope
          );

          let data = {
            originAttributes: {}, 
            scope: record.scope
          };

          globalMM.broadcastAsyncMessage('pushsubscriptionchange', data);
        }));
  },

  receivedPushMessage: function(aPushRecord, message) {
    this._db.put(aPushRecord)
      .then(_ => this._notifyApp(aPushRecord, message));
  },

  _notifyApp: function(aPushRecord, message) {
    if (!aPushRecord || !aPushRecord.scope) {
      debug("notifyApp() something is undefined.  Dropping notification: " +
        JSON.stringify(aPushRecord) );
      return;
    }

    debug("notifyApp() " + aPushRecord.scope);
    let scopeURI = Services.io.newURI(aPushRecord.scope, null, null);
    
    let notification = Cc["@mozilla.org/push/ObserverNotification;1"]
                         .createInstance(Ci.nsIPushObserverNotification);
    notification.pushEndpoint = aPushRecord.pushEndpoint;
    notification.version = aPushRecord.version;
    notification.data = message;
    notification.lastPush = aPushRecord.lastPush;
    notification.pushCount = aPushRecord.pushCount;

    Services.obs.notifyObservers(
      notification,
      "push-notification",
      aPushRecord.scope
    );

    
    if (Services.perms.testExactPermission(scopeURI, "push") !=
        Ci.nsIPermissionManager.ALLOW_ACTION) {
      debug("Does not have permission for push.");
      return;
    }

    
    let data = {
      payload: message,
      originAttributes: {}, 
      scope: aPushRecord.scope
    };

    let globalMM = Cc['@mozilla.org/globalmessagemanager;1']
                 .getService(Ci.nsIMessageListenerManager);
    globalMM.broadcastAsyncMessage('push', data);
  },

  getByKeyID: function(aKeyID) {
    return this._db.getByKeyID(aKeyID);
  },

  getAllKeyIDs: function() {
    return this._db.getAllKeyIDs();
  },

  _sendRequest(action, aRecord) {
    if (this._state == PUSH_SERVICE_CONNECTION_DISABLE) {
      return Promise.reject({state: 0, error: "Service not active"});
    } else if (this._state == PUSH_SERVICE_ACTIVE_OFFLINE) {
      return Promise.reject({state: 0, error: "NetworkError"});
    }
    return this._service.request(action, aRecord);
  },

  



  _registerWithServer: function(aPageRecord) {
    debug("registerWithServer()");

    return this._sendRequest("register", aPageRecord)
      .then(pushRecord => this._onRegisterSuccess(pushRecord),
            err => this._onRegisterError(err))
      .then(pushRecord => {
        if (this._pendingRegisterRequest[aPageRecord.scope]) {
          delete this._pendingRegisterRequest[aPageRecord.scope];
        }
        return pushRecord;
      }, err => {
        if (this._pendingRegisterRequest[aPageRecord.scope]) {
          delete this._pendingRegisterRequest[aPageRecord.scope];
        }
        throw err;
     });
  },

  _register: function(aPageRecord) {
    return this._checkActivated()
      .then(_ => this._db.getByScope(aPageRecord.scope))
      .then(pushRecord => {
        if (pushRecord === undefined) {
          if (this._pendingRegisterRequest[aPageRecord.scope]) {
            return this._pendingRegisterRequest[aPageRecord.scope];
          }
          return this._pendingRegisterRequest[aPageRecord.scope] = this._registerWithServer(aPageRecord);
        }
        return pushRecord;
      }, error => {
        debug("getByScope failed");
        throw error;
      });
  },

  



  _onRegisterSuccess: function(aRecord) {
    debug("_onRegisterSuccess()");

    return this._db.put(aRecord)
      .then(_ => aRecord, error => {
        
        this._sendRequest("unregister", aRecord).catch(err => {
          debug("_onRegisterSuccess: Error unregistering stale subscription" +
            err);
        });
        throw error;
      });
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

    this._register(aPageRecord)
      .then(pushRecord => {
        let message = this._service.prepareRegister(pushRecord);
        message.requestID = aPageRecord.requestID;
        aMessageManager.sendAsyncMessage("PushService:Register:OK", message);
      }, error => {
        let message = {
          requestID: aPageRecord.requestID,
          error
        };
        aMessageManager.sendAsyncMessage("PushService:Register:KO", message);
      });
  },

  
























  _unregister: function(aPageRecord) {
    debug("_unregister()");

    if (!aPageRecord.scope) {
      return Promise.reject({state: 0, error: "NotFoundError"});
    }

    return this._checkActivated()
      .then(_ => this._db.getByScope(aPageRecord.scope))
      .then(record => {
        
        if (record === undefined) {
          throw "NotFoundError";
        }

        return Promise.all([
          this._sendRequest("unregister", record),
          this._db.delete(this._service.getKeyFromRecord(record))
        ]);
      });
  },

  unregister: function(aPageRecord, aMessageManager) {
    debug("unregister() " + JSON.stringify(aPageRecord));

    this._unregister(aPageRecord)
      .then(_ =>
        aMessageManager.sendAsyncMessage("PushService:Unregister:OK", {
          requestID: aPageRecord.requestID,
          pushEndpoint: aPageRecord.pushEndpoint
        }), error =>
        aMessageManager.sendAsyncMessage("PushService:Unregister:KO", {
          requestID: aPageRecord.requestID,
          error
        })
      );
  },

  _clearAll: function _clearAll() {
    return this._checkActivated()
      .then(_ => this._db.clearAll())
      .catch(_ => {
        return Promise.resolve();
      });
  },

  


  _registration: function(aPageRecord) {
    debug("_registration()");
    if (!aPageRecord.scope) {
      return Promise.reject({state: 0, error: "Database error"});
    }

    return this._checkActivated()
      .then(_ => this._db.getByScope(aPageRecord.scope))
      .then(pushRecord => {
        if (!pushRecord) {
          return null;
        }
        return this._service.prepareRegistration(pushRecord);
      });
  },

  registration: function(aPageRecord, aMessageManager) {
    debug("registration()");

    return this._registration(aPageRecord)
      .then(registration =>
        aMessageManager.sendAsyncMessage("PushService:Registration:OK", {
          requestID: aPageRecord.requestID,
          registration
        }), error =>
        aMessageManager.sendAsyncMessage("PushService:Registration:KO", {
          requestID: aPageRecord.requestID,
          error
        })
      );
  }
};
