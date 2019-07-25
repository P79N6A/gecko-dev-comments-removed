



































let Cu = Components.utils;
let Ci = Components.interfaces;
let Cc = Components.classes;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const STORAGE_MAX_EVENTS = 200;

XPCOMUtils.defineLazyGetter(this, "gPrivBrowsing", function () {
  
  try {
    return Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);
  }
  catch (ex) {
    return null;
  }
});

var EXPORTED_SYMBOLS = ["ConsoleAPIStorage"];

var _consoleStorage = {};






















var ConsoleAPIStorage = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  
  observe: function CS_observe(aSubject, aTopic, aData)
  {
    if (aTopic == "xpcom-shutdown") {
      Services.obs.removeObserver(this, "xpcom-shutdown");
      Services.obs.removeObserver(this, "inner-window-destroyed");
      Services.obs.removeObserver(this, "memory-pressure");
      delete _consoleStorage;
    }
    else if (aTopic == "inner-window-destroyed") {
      let innerWindowID = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      this.clearEvents(innerWindowID);
    }
    else if (aTopic == "memory-pressure") {
      if (aData == "low-memory") {
        this.clearEvents();
      }
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
    return (_consoleStorage[aId] || []).slice(0);
  },

  







  recordEvent: function CS_recordEvent(aWindowID, aEvent)
  {
    let ID = parseInt(aWindowID);
    if (isNaN(ID)) {
      throw new Error("Invalid window ID argument");
    }

    if (gPrivBrowsing && gPrivBrowsing.privateBrowsingEnabled) {
      return;
    }

    if (!_consoleStorage[ID]) {
      _consoleStorage[ID] = [];
    }
    let storage = _consoleStorage[ID];
    storage.push(aEvent);

    
    if (storage.length > STORAGE_MAX_EVENTS) {
      storage.shift();
    }

    Services.obs.notifyObservers(aEvent, "console-storage-cache-event", ID);
  },

  







  clearEvents: function CS_clearEvents(aId)
  {
    if (aId != null) {
      delete _consoleStorage[aId];
    }
    else {
      _consoleStorage = {};
      Services.obs.notifyObservers(null, "console-storage-reset", null);
    }
  },
};

ConsoleAPIStorage.init();
