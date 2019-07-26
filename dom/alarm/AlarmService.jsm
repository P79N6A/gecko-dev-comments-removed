



"use strict";


const DEBUG = false;

function debug(aStr) {
  if (DEBUG)
    dump("AlarmService: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AlarmDB.jsm");

this.EXPORTED_SYMBOLS = ["AlarmService"];

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyGetter(this, "messenger", function() {
  return Cc["@mozilla.org/system-message-internal;1"].getService(Ci.nsISystemMessagesInternal);
});

XPCOMUtils.defineLazyGetter(this, "powerManagerService", function() {
  return Cc["@mozilla.org/power/powermanagerservice;1"].getService(Ci.nsIPowerManagerService);
});

let myGlobal = this;












this.AlarmService = {
  init: function init() {
    debug("init()");

    this._currentTimezoneOffset = (new Date()).getTimezoneOffset();

    let alarmHalService =
      this._alarmHalService = Cc["@mozilla.org/alarmHalService;1"]
                              .getService(Ci.nsIAlarmHalService);

    alarmHalService.setAlarmFiredCb(this._onAlarmFired.bind(this));
    alarmHalService.setTimezoneChangedCb(this._onTimezoneChanged.bind(this));

    
    const messages = ["AlarmsManager:GetAll",
                      "AlarmsManager:Add",
                      "AlarmsManager:Remove"];
    messages.forEach(function addMessage(msgName) {
        ppmm.addMessageListener(msgName, this);
    }.bind(this));

    
    let idbManager = Cc["@mozilla.org/dom/indexeddb/manager;1"]
                     .getService(Ci.nsIIndexedDatabaseManager);
    idbManager.initWindowless(myGlobal);
    this._db = new AlarmDB(myGlobal);
    this._db.init(myGlobal);

    
    this._alarmQueue = [];

    this._restoreAlarmsFromDb();
  },

  
  _alarm: null,
  get _currentAlarm() {
    return this._alarm;
  },
  set _currentAlarm(aAlarm) {
    this._alarm = aAlarm;
    if (!aAlarm)
      return;

    if (!this._alarmHalService.setAlarm(this._getAlarmTime(aAlarm) / 1000, 0))
      throw Components.results.NS_ERROR_FAILURE;
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("receiveMessage(): " + aMessage.name);
    let json = aMessage.json;

    
    
    if (["AlarmsManager:GetAll", "AlarmsManager:Add", "AlarmsManager:Remove"]
          .indexOf(aMessage.name) != -1) {
      if (!aMessage.target.assertPermission("alarms")) {
        debug("Got message from a child process with no 'alarms' permission.");
        return null;
      }
      if (!aMessage.target.assertContainApp(json.manifestURL)) {
        debug("Got message from a child process containing illegal manifest URL.");
        return null;
      }
    }

    let mm = aMessage.target.QueryInterface(Ci.nsIMessageSender);
    switch (aMessage.name) {
      case "AlarmsManager:GetAll":
        this._db.getAll(
          json.manifestURL,
          function getAllSuccessCb(aAlarms) {
            debug("Callback after getting alarms from database: " +
                  JSON.stringify(aAlarms));
            this._sendAsyncMessage(mm, "GetAll", true, json.requestId, aAlarms);
          }.bind(this),
          function getAllErrorCb(aErrorMsg) {
            this._sendAsyncMessage(mm, "GetAll", false, json.requestId, aErrorMsg);
          }.bind(this)
        );
        break;

      case "AlarmsManager:Add":
        
        let newAlarm = {
          date: json.date,
          ignoreTimezone: json.ignoreTimezone,
          data: json.data,
          pageURL: json.pageURL,
          manifestURL: json.manifestURL
        };

        this.add(newAlarm, null,
          
          this._sendAsyncMessage.bind(this, mm, "Add", true, json.requestId),
          
          this._sendAsyncMessage.bind(this, mm, "Add", false, json.requestId)
        );
        break;

      case "AlarmsManager:Remove":
        this.remove(json.id, json.manifestURL);
        break;

      default:
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
        break;
    }
  },

  _sendAsyncMessage: function _sendAsyncMessage(aMessageManager, aMessageName,
                                                aSuccess, aRequestId, aData) {
    debug("_sendAsyncMessage()");

    if (!aMessageManager) {
      debug("Invalid message manager: null");
      throw Components.results.NS_ERROR_FAILURE;
    }

    let json = null;
    switch (aMessageName)
    {
      case "Add":
        json = aSuccess ?
          { requestId: aRequestId, id: aData } :
          { requestId: aRequestId, errorMsg: aData };
        break;

      case "GetAll":
        json = aSuccess ?
          { requestId: aRequestId, alarms: aData } :
          { requestId: aRequestId, errorMsg: aData };
        break;

      default:
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
        break;
    }

    aMessageManager.sendAsyncMessage("AlarmsManager:" + aMessageName +
                                     ":Return:" + (aSuccess ? "OK" : "KO"), json);
  },

  _removeAlarmFromDb: function _removeAlarmFromDb(aId, aManifestURL,
                                                  aRemoveSuccessCb) {
    debug("_removeAlarmFromDb()");

    
    
    if (!aRemoveSuccessCb) {
      aRemoveSuccessCb = function removeSuccessCb() {
        debug("Remove alarm from DB successfully.");
      };
    }

    this._db.remove(
      aId,
      aManifestURL,
      aRemoveSuccessCb,
      function removeErrorCb(aErrorMsg) {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
      }
    );
  },

  




  _publicAlarm: function _publicAlarm(aAlarm) {
    let alarm = {
      "id":              aAlarm.id,
      "date":            aAlarm.date,
      "respectTimezone": aAlarm.ignoreTimezone ?
                           "ignoreTimezone" : "honorTimezone",
      "data":            aAlarm.data
    };

    return alarm;
  },

  _fireSystemMessage: function _fireSystemMessage(aAlarm) {
    debug("Fire system message: " + JSON.stringify(aAlarm));

    let manifestURI = Services.io.newURI(aAlarm.manifestURL, null, null);
    let pageURI = Services.io.newURI(aAlarm.pageURL, null, null);

    messenger.sendMessage("alarm", this._publicAlarm(aAlarm),
                          pageURI, manifestURI);
  },

  _notifyAlarmObserver: function _notifyAlarmObserver(aAlarm) {
    debug("_notifyAlarmObserver()");

    if (aAlarm.manifestURL) {
      this._fireSystemMessage(aAlarm);
    } else if (typeof aAlarm.alarmFiredCb === "function") {
      aAlarm.alarmFiredCb(this._publicAlarm(aAlarm));
    }
  },

  _onAlarmFired: function _onAlarmFired() {
    debug("_onAlarmFired()");

    if (this._currentAlarm) {
      this._removeAlarmFromDb(this._currentAlarm.id, null);
      this._notifyAlarmObserver(this._currentAlarm);
      this._currentAlarm = null;
    }

    
    let alarmQueue = this._alarmQueue;
    while (alarmQueue.length > 0) {
      let nextAlarm = alarmQueue.shift();
      let nextAlarmTime = this._getAlarmTime(nextAlarm);

      
      
      if (nextAlarmTime <= Date.now()) {
        this._removeAlarmFromDb(nextAlarm.id, null);
        this._notifyAlarmObserver(nextAlarm);
      } else {
        this._currentAlarm = nextAlarm;
        break;
      }
    }
    this._debugCurrentAlarm();
  },

  _onTimezoneChanged: function _onTimezoneChanged(aTimezoneOffset) {
    debug("_onTimezoneChanged()");

    this._currentTimezoneOffset = aTimezoneOffset;
    this._restoreAlarmsFromDb();
  },

  _restoreAlarmsFromDb: function _restoreAlarmsFromDb() {
    debug("_restoreAlarmsFromDb()");

    this._db.getAll(
      null,
      function getAllSuccessCb(aAlarms) {
        debug("Callback after getting alarms from database: " +
              JSON.stringify(aAlarms));

        
        let alarmQueue = this._alarmQueue;
        alarmQueue.length = 0;
        this._currentAlarm = null;

        
        
        aAlarms.forEach(function addAlarm(aAlarm) {
          if (this._getAlarmTime(aAlarm) > Date.now()) {
            alarmQueue.push(aAlarm);
          } else {
            this._removeAlarmFromDb(aAlarm.id, null);
            this._notifyAlarmObserver(aAlarm);
          }
        }.bind(this));

        
        if (alarmQueue.length) {
          alarmQueue.sort(this._sortAlarmByTimeStamps.bind(this));
          this._currentAlarm = alarmQueue.shift();
        }

        this._debugCurrentAlarm();
      }.bind(this),
      function getAllErrorCb(aErrorMsg) {
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
      }
    );
  },

  _getAlarmTime: function _getAlarmTime(aAlarm) {
    let alarmTime = (new Date(aAlarm.date)).getTime();

    
    
    
    
    
    if (aAlarm.ignoreTimezone)
       alarmTime += (this._currentTimezoneOffset - aAlarm.timezoneOffset) * 60000;

    return alarmTime;
  },

  _sortAlarmByTimeStamps: function _sortAlarmByTimeStamps(aAlarm1, aAlarm2) {
    return this._getAlarmTime(aAlarm1) - this._getAlarmTime(aAlarm2);
  },

  _debugCurrentAlarm: function _debugCurrentAlarm() {
    debug("Current alarm: " + JSON.stringify(this._currentAlarm));
    debug("Alarm queue: " + JSON.stringify(this._alarmQueue));
  },

  




























  add: function(aNewAlarm, aAlarmFiredCb, aSuccessCb, aErrorCb) {
    debug("add(" + aNewAlarm.date + ")");

    aSuccessCb = aSuccessCb || function() {};
    aErrorCb = aErrorCb || function() {};

    if (!aNewAlarm) {
      aErrorCb("alarm is null");
      return;
    }

    aNewAlarm['timezoneOffset'] = this._currentTimezoneOffset;
    let aNewAlarmTime = this._getAlarmTime(aNewAlarm);
    if (aNewAlarmTime <= Date.now()) {
      debug("Adding a alarm that has past time.");
      this._debugCurrentAlarm();
      aErrorCb("InvalidStateError");
      return;
    }

    this._db.add(
      aNewAlarm,
      function addSuccessCb(aNewId) {
        debug("Callback after adding alarm in database.");

        aNewAlarm['id'] = aNewId;

        
        
        aNewAlarm['alarmFiredCb'] = aAlarmFiredCb;

        
        if (this._currentAlarm == null) {
          this._currentAlarm = aNewAlarm;
          this._debugCurrentAlarm();
          aSuccessCb(aNewId);
          return;
        }

        
        
        let alarmQueue = this._alarmQueue;
        let currentAlarmTime = this._getAlarmTime(this._currentAlarm);
        if (aNewAlarmTime < currentAlarmTime) {
          alarmQueue.unshift(this._currentAlarm);
          this._currentAlarm = aNewAlarm;
          this._debugCurrentAlarm();
          aSuccessCb(aNewId);
          return;
        }

        
        alarmQueue.push(aNewAlarm);
        alarmQueue.sort(this._sortAlarmByTimeStamps.bind(this));
        this._debugCurrentAlarm();
        aSuccessCb(aNewId);
      }.bind(this),
      function addErrorCb(aErrorMsg) {
        aErrorCb(aErrorMsg);
      }.bind(this)
    );
  },

  








  remove: function(aAlarmId, aManifestURL) {
    debug("remove(" + aAlarmId + ", " + aManifestURL + ")");
    this._removeAlarmFromDb(
      aAlarmId,
      aManifestURL,
      function removeSuccessCb() {
        debug("Callback after removing alarm from database.");

        
        if (!this._currentAlarm) {
          debug("No alarms set.");
          return;
        }

        
        
        let alarmQueue = this._alarmQueue;
        if (this._currentAlarm.id != aAlarmId ||
            this._currentAlarm.manifestURL != aManifestURL) {

          for (let i = 0; i < alarmQueue.length; i++) {
            if (alarmQueue[i].id == aAlarmId &&
                alarmQueue[i].manifestURL == aManifestURL) {

              alarmQueue.splice(i, 1);
              break;
            }
          }
          this._debugCurrentAlarm();
          return;
        }

        
        
        if (alarmQueue.length) {
          this._currentAlarm = alarmQueue.shift();
          this._debugCurrentAlarm();
          return;
        }

        
        this._currentAlarm = null;
        this._debugCurrentAlarm();
      }.bind(this)
    );
  }
}

AlarmService.init();
