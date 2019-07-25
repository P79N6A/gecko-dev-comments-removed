



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

let EXPORTED_SYMBOLS = ["AlarmService"];

XPCOMUtils.defineLazyGetter(this, "ppmm", function() {
  return Cc["@mozilla.org/parentprocessmessagemanager;1"].getService(Ci.nsIFrameMessageManager);
});

XPCOMUtils.defineLazyGetter(this, "messenger", function() {
  return Cc["@mozilla.org/system-message-internal;1"].getService(Ci.nsISystemMessagesInternal);
});

let myGlobal = this;

let AlarmService = {
  init: function init() {
    debug("init()");

    this._currentTimezoneOffset = (new Date()).getTimezoneOffset();

    let alarmHalService = this._alarmHalService = Cc["@mozilla.org/alarmHalService;1"].getService(Ci.nsIAlarmHalService);
    alarmHalService.setAlarmFiredCb(this._onAlarmFired.bind(this));
    alarmHalService.setTimezoneChangedCb(this._onTimezoneChanged.bind(this));

    
    const messages = ["AlarmsManager:GetAll", "AlarmsManager:Add", "AlarmsManager:Remove"];
    messages.forEach(function addMessage(msgName) {
        ppmm.addMessageListener(msgName, this);
    }.bind(this));

    
    let idbManager = Components.classes["@mozilla.org/dom/indexeddb/manager;1"].getService(Ci.nsIIndexedDatabaseManager);
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
    switch (aMessage.name) {
      case "AlarmsManager:GetAll":
        this._db.getAll(
          function getAllSuccessCb(aAlarms) {
            debug("Callback after getting alarms from database: " + JSON.stringify(aAlarms));
            ppmm.sendAsyncMessage(
              "AlarmsManager:GetAll:Return:OK", 
              { requestID: json.requestID, alarms: aAlarms }
            ); 
          }.bind(this),
          function getAllErrorCb(aErrorMsg) {
            ppmm.sendAsyncMessage(
              "AlarmsManager:GetAll:Return:KO", 
              { requestID: json.requestID, errorMsg: aErrorMsg }
            );
          }.bind(this)
        );
        break;

      case "AlarmsManager:Add":
        
        let newAlarm = {
          date: json.date, 
          ignoreTimezone: json.ignoreTimezone, 
          timezoneOffset: this._currentTimezoneOffset, 
          data: json.data,
          manifestURL: json.manifestURL
        };

        this._db.add(
          newAlarm,
          function addSuccessCb(aNewId) {
            debug("Callback after adding alarm in database.");

            newAlarm['id'] = aNewId;
            let newAlarmTime = this._getAlarmTime(newAlarm);

            if (newAlarmTime <= Date.now()) {
              debug("Adding a alarm that has past time. Don't set it in system.");
              this._debugCurrentAlarm();
              return;
            }

            
            if (this._currentAlarm == null) {
              this._currentAlarm = newAlarm;
              this._debugCurrentAlarm();
              return;
            }

            
            
            let alarmQueue = this._alarmQueue;
            let currentAlarmTime = this._getAlarmTime(this._currentAlarm);
            if (newAlarmTime < currentAlarmTime) {
              alarmQueue.unshift(this._currentAlarm);
              this._currentAlarm = newAlarm;
              this._debugCurrentAlarm();
              return;
            }

            
            alarmQueue.push(newAlarm);
            alarmQueue.sort(this._sortAlarmByTimeStamps.bind(this));
            this._debugCurrentAlarm();

            ppmm.sendAsyncMessage(
              "AlarmsManager:Add:Return:OK", 
              { requestID: json.requestID, id: aNewId }
            );
          }.bind(this),
          function addErrorCb(aErrorMsg) {
            ppmm.sendAsyncMessage(
              "AlarmsManager:Add:Return:KO", 
              { requestID: json.requestID, errorMsg: aErrorMsg }
            );
          }.bind(this)
        );
        break;

      case "AlarmsManager:Remove":
        this._db.remove(
          json.id,
          function removeSuccessCb() {
            debug("Callback after removing alarm from database.");

            
            if (!this._currentAlarm) {
              this._debugCurrentAlarm();
              return;
            }

            
            let alarmQueue = this._alarmQueue;
            if (this._currentAlarm.id != json.id) {
              for (let i = 0; i < alarmQueue.length; i++) {
                if (alarmQueue[i].id == json.id) {
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
          }.bind(this),
          function removeErrorCb(aErrorMsg) {
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
          }
        );
        break;

      default:
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
        break;
    }
  },

  _onAlarmFired: function _onAlarmFired() {
    debug("_onAlarmFired()");

    if (this._currentAlarm) {
      debug("Fire system intent: " + JSON.stringify(this._currentAlarm));
      if (this._currentAlarm.manifestURL)
        messenger.sendMessage("alarm", this._currentAlarm, this._currentAlarm.manifestURL);
      this._currentAlarm = null;
    }

    
    let nowTime = Date.now();
    let alarmQueue = this._alarmQueue;
    while (alarmQueue.length > 0) {
      let nextAlarm = alarmQueue.shift();
      let nextAlarmTime = this._getAlarmTime(nextAlarm);

      
      
      if (nextAlarmTime <= nowTime) {
        debug("Fire system intent: " + JSON.stringify(nextAlarm));
        if (nextAlarm.manifestURL)
          messenger.sendMessage("alarm", nextAlarm, nextAlarm.manifestURL);
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
      function getAllSuccessCb(aAlarms) {
        debug("Callback after getting alarms from database: " + JSON.stringify(aAlarms));

        
        let alarmQueue = this._alarmQueue;
        alarmQueue.length = 0;
        this._currentAlarm = null;
        
        
        let nowTime = Date.now();
        aAlarms.forEach(function addAlarm(aAlarm) {
          if (this._getAlarmTime(aAlarm) > nowTime)
            alarmQueue.push(aAlarm);
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
}

AlarmService.init();
