



"use strict;"

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/Task.jsm');


XPCOMUtils.defineLazyModuleGetter(this, 'LogManager',
  'resource://services-common/logmanager.js');

XPCOMUtils.defineLazyModuleGetter(this, 'Log',
  'resource://gre/modules/Log.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'Preferences',
  'resource://gre/modules/Preferences.jsm');

XPCOMUtils.defineLazyModuleGetter(this, 'setTimeout',
  'resource://gre/modules/Timer.jsm');
XPCOMUtils.defineLazyModuleGetter(this, 'clearTimeout',
  'resource://gre/modules/Timer.jsm');


XPCOMUtils.defineLazyModuleGetter(this, 'ReadingList',
  'resource:///modules/readinglist/ReadingList.jsm');


XPCOMUtils.defineLazyModuleGetter(this, 'Sync',
  'resource:///modules/readinglist/Sync.jsm');


XPCOMUtils.defineLazyGetter(this, "fxAccountsCommon", function() {
  let namespace = {};
  Cu.import("resource://gre/modules/FxAccountsCommon.js", namespace);
  return namespace;
});

this.EXPORTED_SYMBOLS = ["ReadingListScheduler"];



const OBSERVERS = [
  
  "network:offline-status-changed",
  
  "fxaccounts:onverified",
  
  "readinglist:backoff-requested",
  
  "readinglist:user-sync",

];

let prefs = new Preferences("readinglist.scheduler.");


let intervals = {
  
  _fixupIntervalPref(prefName, def) {
    
    return prefs.get(prefName, def) * 1000;
  },

  
  get initial() this._fixupIntervalPref("initial", 10), 
  
  get schedule() this._fixupIntervalPref("schedule", 2 * 60 * 60), 
  
  get retry() this._fixupIntervalPref("retry", 2 * 60), 
};


function InternalScheduler(readingList = null) {
  
  let logs = [
    "browserwindow.syncui",
    "FirefoxAccounts",
    "readinglist.api",
    "readinglist.scheduler",
    "readinglist.serverclient",
    "readinglist.sync",
  ];

  this._logManager = new LogManager("readinglist.", logs, "readinglist");
  this.log = Log.repository.getLogger("readinglist.scheduler");
  this.log.info("readinglist scheduler created.")
  this.state = this.STATE_OK;
  this.readingList = readingList || ReadingList; 

  
  
}

InternalScheduler.prototype = {
  
  
  
  _nextScheduledSync: null,
  
  
  _backoffUntil: 0,
  
  _timer: null,
  
  
  _timerRunning: false,
  
  _engine: Sync,
  
  
  _currentErrorBackoff: 0,

  
  state: null,
  STATE_OK: "ok",
  STATE_ERROR_AUTHENTICATION: "authentication error",
  STATE_ERROR_OTHER: "other error",

  init() {
    this.log.info("scheduler initialzing");
    this._setupRLListener();
    this._observe = this.observe.bind(this);
    for (let notification of OBSERVERS) {
      Services.obs.addObserver(this._observe, notification, false);
    }
    this._nextScheduledSync = Date.now() + intervals.initial;
    this._setupTimer();
  },

  _setupRLListener() {
    let maybeSync = () => {
      if (this._timerRunning) {
        
        
        
        this._maybeReschedule(1);
      } else {
        
        this._syncNow();
      }
    };
    let listener = {
      onItemAdded: maybeSync,
      onItemUpdated: maybeSync,
      onItemDeleted: maybeSync,
    }
    this.readingList.addListener(listener);
  },

  
  finalize() {
    this.log.info("scheduler finalizing");
    this._clearTimer();
    for (let notification of OBSERVERS) {
      Services.obs.removeObserver(this._observe, notification);
    }
    this._observe = null;
  },

  observe(subject, topic, data) {
    this.log.debug("observed ${}", topic);
    switch (topic) {
      case "readinglist:backoff-requested": {
        
        let interval = parseInt(data, 10);
        if (isNaN(interval)) {
          this.log.warn("Backoff request had non-numeric value", data);
          return;
        }
        this.log.info("Received a request to backoff for ${} seconds", interval);
        this._backoffUntil = Date.now() + interval * 1000;
        this._maybeReschedule(0);
        break;
      }
      case "readinglist:user-sync":
        this._syncNow();
        break;
      case "fxaccounts:onverified":
        
        if (this.state == this.STATE_ERROR_AUTHENTICATION) {
          this.state = this.STATE_OK;
        }
        
        this._syncNow();
        break;

      
      
      default:
        break;
    }
    
    
    this._setupTimer(true);
  },

  
  _isBlockedOnError() {
    
    return this.state == this.STATE_ERROR_AUTHENTICATION;
  },

  
  _canSync(ignoreBlockingErrors = false) {
    if (!prefs.get("enabled")) {
      this.log.info("canSync=false - syncing is disabled");
      return false;
    }
    if (Services.io.offline) {
      this.log.info("canSync=false - we are offline");
      return false;
    }
    if (!ignoreBlockingErrors && this._isBlockedOnError()) {
      this.log.info("canSync=false - we are in a blocked error state", this.state);
      return false;
    }
    this.log.info("canSync=true");
    return true;
  },

  
  
  _setupTimer(ignoreBlockingErrors = false) {
    if (!this._canSync(ignoreBlockingErrors)) {
      this._clearTimer();
      return;
    }
    if (this._timer) {
      let when = new Date(this._nextScheduledSync);
      let delay = this._nextScheduledSync - Date.now();
      this.log.info("checkStatus - already have a timer - will fire in ${delay}ms at ${when}",
                    {delay, when});
      return;
    }
    if (this._timerRunning) {
      this.log.info("checkStatus - currently syncing");
      return;
    }
    
    let now = Date.now();
    let delay = Math.max(0, this._nextScheduledSync - now);
    let when = new Date(now + delay);
    this.log.info("next scheduled sync is in ${delay}ms (at ${when})", {delay, when})
    this._timer = this._setTimeout(delay);
  },

  
  
  
  
  
  _maybeReschedule(delay) {
    
    
    
    
    if (!delay && !this._nextScheduledSync) {
      this.log.debug("_maybeReschedule ignoring a backoff request while running");
      return;
    }
    let now = Date.now();
    if (!this._nextScheduledSync) {
      this._nextScheduledSync = now + delay;
    }
    
    
    
    
    this._nextScheduledSync = Math.min(this._nextScheduledSync, now + delay);
    
    this._nextScheduledSync = Math.max(this._nextScheduledSync, this._backoffUntil);
    
    this._clearTimer();
  },

  
  _doSync() {
    this.log.debug("starting sync");
    this._timer = null;
    this._timerRunning = true;
    
    
    this._nextScheduledSync = 0;
    Services.obs.notifyObservers(null, "readinglist:sync:start", null);
    this._engine.start().then(() => {
      this.log.info("Sync completed successfully");
      
      
      prefs.set("lastSync", new Date().toString());
      this.state = this.STATE_OK;
      this._logManager.resetFileLog();
      Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
      this._currentErrorBackoff = 0; 
      return intervals.schedule;
    }).catch(err => {
      
      
      
      if (err.message == fxAccountsCommon.ERROR_NO_ACCOUNT ||
          err.message == fxAccountsCommon.ERROR_UNVERIFIED_ACCOUNT) {
        
        this._currentErrorBackoff = 0; 
        this.log.info("Can't sync due to FxA account state " + err.message);
        this.state = this.STATE_OK;
        this._logManager.resetFileLog();
        Services.obs.notifyObservers(null, "readinglist:sync:finish", null);
        
        
        return intervals.schedule;
      }
      this.state = err.message == fxAccountsCommon.ERROR_AUTH_ERROR ?
                   this.STATE_ERROR_AUTHENTICATION : this.STATE_ERROR_OTHER;
      this.log.error("Sync failed, now in state '${state}': ${err}",
                     {state: this.state, err});
      this._logManager.resetFileLog();
      Services.obs.notifyObservers(null, "readinglist:sync:error", null);
      
      this._currentErrorBackoff = this._currentErrorBackoff == 0 ? intervals.retry :
                                  Math.min(intervals.schedule, this._currentErrorBackoff * 2);
      return this._currentErrorBackoff;
    }).then(nextDelay => {
      this._timerRunning = false;
      
      this._maybeReschedule(nextDelay);
      this._setupTimer();
      this._onAutoReschedule(); 
    }).catch(err => {
      
      this.log.error("Failed to reschedule after sync completed", err);
    });
  },

  _clearTimer() {
    if (this._timer) {
      clearTimeout(this._timer);
      this._timer = null;
    }
  },

  
  
  
  _syncNow() {
    if (!prefs.get("enabled")) {
      this.log.info("syncNow() but syncing is disabled - ignoring");
      return;
    }

    if (this._timerRunning) {
      this.log.info("syncNow() but a sync is already in progress - ignoring");
      return;
    }
    this._clearTimer();
    this._doSync();
  },

  
  
  
  _setTimeout(delay) {
    return setTimeout(() => this._doSync(), delay);
  },
  
  
  _onAutoReschedule() {},
};

let internalScheduler = new InternalScheduler();
internalScheduler.init();



let ReadingListScheduler = {
  get STATE_OK() internalScheduler.STATE_OK,
  get STATE_ERROR_AUTHENTICATION() internalScheduler.STATE_ERROR_AUTHENTICATION,
  get STATE_ERROR_OTHER() internalScheduler.STATE_ERROR_OTHER,

  get state() internalScheduler.state,
};



function createTestableScheduler(readingList) {
  
  if (internalScheduler) {
    internalScheduler.finalize();
    internalScheduler = null;
  }
  
  return new InternalScheduler(readingList);
}


function getInternalScheduler() {
  return internalScheduler;
}
