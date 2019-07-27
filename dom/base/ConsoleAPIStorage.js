



"use strict";

let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");



const CALL_DELAY = 15 


const MESSAGES_IN_INTERVAL = 1500

const STORAGE_MAX_EVENTS = 200;

var _consoleStorage = new Map();
var _consolePendingStorage = new Map();
var _timer;

const CONSOLEAPISTORAGE_CID = Components.ID('{96cf7855-dfa9-4c6d-8276-f9705b4890f2}');






















function ConsoleAPIStorageService() {
  this.init();
}

ConsoleAPIStorageService.prototype = {
  classID : CONSOLEAPISTORAGE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIConsoleAPIStorage,
                                         Ci.nsIObserver]),
  classInfo: XPCOMUtils.generateCI({
    classID: CONSOLEAPISTORAGE_CID,
    contractID: '@mozilla.org/consoleAPI-storage;1',
    interfaces: [Ci.nsIConsoleAPIStorage, Ci.nsIObserver],
    flags: Ci.nsIClassInfo.SINGLETON
  }),

  observe: function CS_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "xpcom-shutdown") {
      Services.obs.removeObserver(this, "xpcom-shutdown");
      Services.obs.removeObserver(this, "inner-window-destroyed");
      Services.obs.removeObserver(this, "memory-pressure");
    }
    else if (aTopic == "inner-window-destroyed") {
      let innerWindowID = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      this.clearEvents(innerWindowID + "");
    }
    else if (aTopic == "memory-pressure") {
      this.clearEvents();
    }
  },

  
  init: function CS_init()
  {
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, "inner-window-destroyed", false);
    Services.obs.addObserver(this, "memory-pressure", false);
  },

  










  getEvents: function CS_getEvents(aId)
  {
    if (aId != null) {
      return (_consoleStorage.get(aId) || []).slice(0);
    }

    let result = [];

    for (let [id, events] of _consoleStorage) {
      result.push.apply(result, events);
    }

    return result.sort(function(a, b) {
      return a.timeStamp - b.timeStamp;
    });
  },

  








  recordEvent: function CS_recordEvent(aId, aOuterId, aEvent)
  {
    if (!_consoleStorage.has(aId)) {
      _consoleStorage.set(aId, []);
    }

    let storage = _consoleStorage.get(aId);
    storage.push(aEvent);

    
    if (storage.length > STORAGE_MAX_EVENTS) {
      storage.shift();
    }

    Services.obs.notifyObservers(aEvent, "console-api-log-event", aOuterId);
    Services.obs.notifyObservers(aEvent, "console-storage-cache-event", aId);
  },

  



  recordPendingEvent: function CS_recordPendingEvent(aId, aOuterId, aEvent)
  {
    if (!_consolePendingStorage.has(aId)) {
      _consolePendingStorage.set(aId, []);
    }

    let storage = _consolePendingStorage.get(aId);
    storage.push({ outerId: aOuterId, event: aEvent });

    if (!_timer) {
      _timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    }

    let self = this;
    _timer.initWithCallback(function() { self.flushPendingEvents(); },
                            CALL_DELAY, Ci.nsITimer.TYPE_REPEATING_SLACK);
  },

  


  flushPendingEvents: function CS_flushPendingEvents()
  {
    for (let [id, objs] of _consolePendingStorage) {
      for (let i = 0; i < objs.length && i < MESSAGES_IN_INTERVAL; ++i) {
        this.recordEvent(id, objs[i].outerId, objs[i].event);
      }

      if (objs.length <= MESSAGES_IN_INTERVAL) {
        _consolePendingStorage.delete(id);
      } else {
        _consolePendingStorage.set(id, objs.splice(MESSAGES_IN_INTERVAL));
      }
    }

    if (_timer && _consolePendingStorage.size == 0) {
      _timer.cancel();
      _timer = null;
    }
  },

  







  clearEvents: function CS_clearEvents(aId)
  {
    if (aId != null) {
      _consoleStorage.delete(aId);
    }
    else {
      _consoleStorage.clear();
      Services.obs.notifyObservers(null, "console-storage-reset", null);
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ConsoleAPIStorageService]);
