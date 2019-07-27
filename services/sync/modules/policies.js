



this.EXPORTED_SYMBOLS = [
  "ErrorHandler",
  "SyncScheduler",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-common/logmanager.js");

XPCOMUtils.defineLazyModuleGetter(this, "Status",
                                  "resource://services-sync/status.js");

this.SyncScheduler = function SyncScheduler(service) {
  this.service = service;
  this.init();
}
SyncScheduler.prototype = {
  _log: Log.repository.getLogger("Sync.SyncScheduler"),

  _fatalLoginStatus: [LOGIN_FAILED_NO_USERNAME,
                      LOGIN_FAILED_NO_PASSWORD,
                      LOGIN_FAILED_NO_PASSPHRASE,
                      LOGIN_FAILED_INVALID_PASSPHRASE,
                      LOGIN_FAILED_LOGIN_REJECTED],

  


  syncTimer: null,

  setDefaults: function setDefaults() {
    this._log.trace("Setting SyncScheduler policy values to defaults.");

    let service = Cc["@mozilla.org/weave/service;1"]
                    .getService(Ci.nsISupports)
                    .wrappedJSObject;

    let part = service.fxAccountsEnabled ? "fxa" : "sync11";
    let prefSDInterval = "scheduler." + part + ".singleDeviceInterval";
    this.singleDeviceInterval = Svc.Prefs.get(prefSDInterval) * 1000;

    this.idleInterval         = Svc.Prefs.get("scheduler.idleInterval")         * 1000;
    this.activeInterval       = Svc.Prefs.get("scheduler.activeInterval")       * 1000;
    this.immediateInterval    = Svc.Prefs.get("scheduler.immediateInterval")    * 1000;
    this.eolInterval          = Svc.Prefs.get("scheduler.eolInterval")          * 1000;

    
    this.idle = false;

    this.hasIncomingItems = false;

    this.clearSyncTriggers();
  },

  
  get nextSync() Svc.Prefs.get("nextSync", 0) * 1000,
  set nextSync(value) Svc.Prefs.set("nextSync", Math.floor(value / 1000)),

  get syncInterval() Svc.Prefs.get("syncInterval", this.singleDeviceInterval),
  set syncInterval(value) Svc.Prefs.set("syncInterval", value),

  get syncThreshold() Svc.Prefs.get("syncThreshold", SINGLE_USER_THRESHOLD),
  set syncThreshold(value) Svc.Prefs.set("syncThreshold", value),

  get globalScore() Svc.Prefs.get("globalScore", 0),
  set globalScore(value) Svc.Prefs.set("globalScore", value),

  get numClients() Svc.Prefs.get("numClients", 0),
  set numClients(value) Svc.Prefs.set("numClients", value),

  init: function init() {
    this._log.level = Log.Level[Svc.Prefs.get("log.logger.service.main")];
    this.setDefaults();
    Svc.Obs.add("weave:engine:score:updated", this);
    Svc.Obs.add("network:offline-status-changed", this);
    Svc.Obs.add("weave:service:sync:start", this);
    Svc.Obs.add("weave:service:sync:finish", this);
    Svc.Obs.add("weave:engine:sync:finish", this);
    Svc.Obs.add("weave:engine:sync:error", this);
    Svc.Obs.add("weave:service:login:error", this);
    Svc.Obs.add("weave:service:logout:finish", this);
    Svc.Obs.add("weave:service:sync:error", this);
    Svc.Obs.add("weave:service:backoff:interval", this);
    Svc.Obs.add("weave:service:ready", this);
    Svc.Obs.add("weave:engine:sync:applied", this);
    Svc.Obs.add("weave:service:setup-complete", this);
    Svc.Obs.add("weave:service:start-over", this);
    Svc.Obs.add("FxA:hawk:backoff:interval", this);

    if (Status.checkSetup() == STATUS_OK) {
      Svc.Obs.add("wake_notification", this);
      Svc.Idle.addIdleObserver(this, Svc.Prefs.get("scheduler.idleTime"));
    }
  },

  observe: function observe(subject, topic, data) {
    this._log.trace("Handling " + topic);
    switch(topic) {
      case "weave:engine:score:updated":
        if (Status.login == LOGIN_SUCCEEDED) {
          Utils.namedTimer(this.calculateScore, SCORE_UPDATE_DELAY, this,
                           "_scoreTimer");
        }
        break;
      case "network:offline-status-changed":
        
        this._log.trace("Network offline status change: " + data);
        this.checkSyncStatus();
        break;
      case "weave:service:sync:start":
        
        this.clearSyncTriggers();

        
        
        Status.resetBackoff();

        this.globalScore = 0;
        break;
      case "weave:service:sync:finish":
        this.nextSync = 0;
        this.adjustSyncInterval();

        if (Status.service == SYNC_FAILED_PARTIAL && this.requiresBackoff) {
          this.requiresBackoff = false;
          this.handleSyncError();
          return;
        }

        let sync_interval;
        this._syncErrors = 0;
        if (Status.sync == NO_SYNC_NODE_FOUND) {
          this._log.trace("Scheduling a sync at interval NO_SYNC_NODE_FOUND.");
          sync_interval = NO_SYNC_NODE_INTERVAL;
        }
        this.scheduleNextSync(sync_interval);
        break;
      case "weave:engine:sync:finish":
        if (data == "clients") {
          
          this.updateClientMode();
        }
        break;
      case "weave:engine:sync:error":
        
        let exception = subject;
        if (exception.status >= 500 && exception.status <= 504) {
          this.requiresBackoff = true;
        }
        break;
      case "weave:service:login:error":
        this.clearSyncTriggers();

        if (Status.login == MASTER_PASSWORD_LOCKED) {
          
          
          this._log.debug("Couldn't log in: master password is locked.");
          this._log.trace("Scheduling a sync at MASTER_PASSWORD_LOCKED_RETRY_INTERVAL");
          this.scheduleAtInterval(MASTER_PASSWORD_LOCKED_RETRY_INTERVAL);
        } else if (this._fatalLoginStatus.indexOf(Status.login) == -1) {
          
          
          this.checkSyncStatus();
        }
        break;
      case "weave:service:logout:finish":
        
        
        this.checkSyncStatus();
        break;
      case "weave:service:sync:error":
        
        
        this.updateClientMode();
        this.adjustSyncInterval();
        this.nextSync = 0;
        this.handleSyncError();
        break;
      case "FxA:hawk:backoff:interval":
      case "weave:service:backoff:interval":
        let requested_interval = subject * 1000;
        this._log.debug("Got backoff notification: " + requested_interval + "ms");
        
        let interval = requested_interval * (1 + Math.random() * 0.25);
        Status.backoffInterval = interval;
        Status.minimumNextSync = Date.now() + requested_interval;
        this._log.debug("Fuzzed minimum next sync: " + Status.minimumNextSync);
        break;
      case "weave:service:ready":
        
        
        let delay = Svc.Prefs.get("autoconnectDelay");
        if (delay) {
          this.delayedAutoConnect(delay);
        }
        break;
      case "weave:engine:sync:applied":
        let numItems = subject.succeeded;
        this._log.trace("Engine " + data + " successfully applied " + numItems +
                        " items.");
        if (numItems) {
          this.hasIncomingItems = true;
        }
        break;
      case "weave:service:setup-complete":
         Services.prefs.savePrefFile(null);
         Svc.Idle.addIdleObserver(this, Svc.Prefs.get("scheduler.idleTime"));
         Svc.Obs.add("wake_notification", this);
         break;
      case "weave:service:start-over":
         this.setDefaults();
         try {
           Svc.Idle.removeIdleObserver(this, Svc.Prefs.get("scheduler.idleTime"));
         } catch (ex if (ex.result == Cr.NS_ERROR_FAILURE)) {
           
           
         }
         break;
      case "idle":
        this._log.trace("We're idle.");
        this.idle = true;
        
        
        
        this.adjustSyncInterval();
        break;
      case "active":
        this._log.trace("Received notification that we're back from idle.");
        this.idle = false;
        Utils.namedTimer(function onBack() {
          if (this.idle) {
            this._log.trace("... and we're idle again. " +
                            "Ignoring spurious back notification.");
            return;
          }

          this._log.trace("Genuine return from idle. Syncing.");
          
          if (this.numClients > 1) {
            this.scheduleNextSync(0);
          }
        }, IDLE_OBSERVER_BACK_DELAY, this, "idleDebouncerTimer");
        break;
      case "wake_notification":
        this._log.debug("Woke from sleep.");
        Utils.nextTick(() => {
          
          if (this.numClients > 1) {
            this._log.debug("More than 1 client. Syncing.");
            this.scheduleNextSync(0);
          }
        });
        break;
    }
  },

  adjustSyncInterval: function adjustSyncInterval() {
    if (Status.eol) {
      this._log.debug("Server status is EOL; using eolInterval.");
      this.syncInterval = this.eolInterval;
      return;
    }

    if (this.numClients <= 1) {
      this._log.trace("Adjusting syncInterval to singleDeviceInterval.");
      this.syncInterval = this.singleDeviceInterval;
      return;
    }

    
    
    if (this.idle) {
      this._log.trace("Adjusting syncInterval to idleInterval.");
      this.syncInterval = this.idleInterval;
      return;
    }

    if (this.hasIncomingItems) {
      this._log.trace("Adjusting syncInterval to immediateInterval.");
      this.hasIncomingItems = false;
      this.syncInterval = this.immediateInterval;
    } else {
      this._log.trace("Adjusting syncInterval to activeInterval.");
      this.syncInterval = this.activeInterval;
    }
  },

  calculateScore: function calculateScore() {
    let engines = [this.service.clientsEngine].concat(this.service.engineManager.getEnabled());
    for (let i = 0;i < engines.length;i++) {
      this._log.trace(engines[i].name + ": score: " + engines[i].score);
      this.globalScore += engines[i].score;
      engines[i]._tracker.resetScore();
    }

    this._log.trace("Global score updated: " + this.globalScore);
    this.checkSyncStatus();
  },

  


  updateClientMode: function updateClientMode() {
    
    let numClients = this.service.clientsEngine.stats.numClients;
    if (this.numClients == numClients)
      return;

    this._log.debug("Client count: " + this.numClients + " -> " + numClients);
    this.numClients = numClients;

    if (numClients <= 1) {
      this._log.trace("Adjusting syncThreshold to SINGLE_USER_THRESHOLD");
      this.syncThreshold = SINGLE_USER_THRESHOLD;
    } else {
      this._log.trace("Adjusting syncThreshold to MULTI_DEVICE_THRESHOLD");
      this.syncThreshold = MULTI_DEVICE_THRESHOLD;
    }
    this.adjustSyncInterval();
  },

  


  checkSyncStatus: function checkSyncStatus() {
    
    
    let ignore = [kSyncBackoffNotMet, kSyncMasterPasswordLocked];
    let skip = this.service._checkSync(ignore);
    this._log.trace("_checkSync returned \"" + skip + "\".");
    if (skip) {
      this.clearSyncTriggers();
      return;
    }

    
    let wait;
    if (this.globalScore > this.syncThreshold) {
      this._log.debug("Global Score threshold hit, triggering sync.");
      wait = 0;
    }
    this.scheduleNextSync(wait);
  },

  




  syncIfMPUnlocked: function syncIfMPUnlocked() {
    
    if (Status.login == MASTER_PASSWORD_LOCKED &&
        Utils.mpLocked()) {
      this._log.debug("Not initiating sync: Login status is " + Status.login);

      
      this._log.trace("Scheduling a sync at MASTER_PASSWORD_LOCKED_RETRY_INTERVAL");
      this.scheduleAtInterval(MASTER_PASSWORD_LOCKED_RETRY_INTERVAL);
      return;
    }

    Utils.nextTick(this.service.sync, this.service);
  },

  


  scheduleNextSync: function scheduleNextSync(interval) {
    
    if (interval == null) {
      interval = this.syncInterval;
    }

    
    if (Status.backoffInterval && interval < Status.backoffInterval) {
      this._log.trace("Requested interval " + interval +
                      " ms is smaller than the backoff interval. " +
                      "Using backoff interval " +
                      Status.backoffInterval + " ms instead.");
      interval = Status.backoffInterval;
    }

    if (this.nextSync != 0) {
      
      
      let currentInterval = this.nextSync - Date.now();
      this._log.trace("There's already a sync scheduled in " +
                      currentInterval + " ms.");
      if (currentInterval < interval && this.syncTimer) {
        this._log.trace("Ignoring scheduling request for next sync in " +
                        interval + " ms.");
        return;
      }
    }

    
    if (interval <= 0) {
      this._log.trace("Requested sync should happen right away.");
      this.syncIfMPUnlocked();
      return;
    }

    this._log.debug("Next sync in " + interval + " ms.");
    Utils.namedTimer(this.syncIfMPUnlocked, interval, this, "syncTimer");

    
    this.nextSync = Date.now() + interval;
  },


  



  scheduleAtInterval: function scheduleAtInterval(minimumInterval) {
    let interval = Utils.calculateBackoff(this._syncErrors,
                                          MINIMUM_BACKOFF_INTERVAL,
                                          Status.backoffInterval);
    if (minimumInterval) {
      interval = Math.max(minimumInterval, interval);
    }

    this._log.debug("Starting client-initiated backoff. Next sync in " +
                    interval + " ms.");
    this.scheduleNextSync(interval);
  },

 







  delayedAutoConnect: function delayedAutoConnect(delay) {
    if (this.service._checkSetup() == STATUS_OK) {
      Utils.namedTimer(this.autoConnect, delay * 1000, this, "_autoTimer");
    }
  },

  autoConnect: function autoConnect() {
    if (this.service._checkSetup() == STATUS_OK && !this.service._checkSync()) {
      
      
      
      this.scheduleNextSync(this.nextSync - Date.now());
    }

    
    if (this._autoTimer) {
      this._autoTimer.clear();
    }
  },

  _syncErrors: 0,
  


  handleSyncError: function handleSyncError() {
    this._log.trace("In handleSyncError. Error count: " + this._syncErrors);
    this._syncErrors++;

    
    
    if (!Status.enforceBackoff) {
      if (this._syncErrors < MAX_ERROR_COUNT_BEFORE_BACKOFF) {
        this.scheduleNextSync();
        return;
      }
      this._log.debug("Sync error count has exceeded " +
                      MAX_ERROR_COUNT_BEFORE_BACKOFF + "; enforcing backoff.");
      Status.enforceBackoff = true;
    }

    this.scheduleAtInterval();
  },


  


  clearSyncTriggers: function clearSyncTriggers() {
    this._log.debug("Clearing sync triggers and the global score.");
    this.globalScore = this.nextSync = 0;

    
    if (this.syncTimer)
      this.syncTimer.clear();
  },

  











  blockSync: function(until = null) {
    if (!until) {
      until = Date.now() + DEFAULT_BLOCK_PERIOD;
    }
    
    Svc.Prefs.set("scheduler.blocked-until", Math.floor(until / 1000));
  },

  unblockSync: function() {
    Svc.Prefs.reset("scheduler.blocked-until");
    
    this.checkSyncStatus();
  },

  get isBlocked() {
    let until = Svc.Prefs.get("scheduler.blocked-until");
    if (until === undefined) {
      return false;
    }
    if (until <= Math.floor(Date.now() / 1000)) {
      
      Svc.Prefs.reset("scheduler.blocked-until");
      return false;
    }
    
    return true;
  },
};

this.ErrorHandler = function ErrorHandler(service) {
  this.service = service;
  this.init();
}
ErrorHandler.prototype = {
  MINIMUM_ALERT_INTERVAL_MSEC: 604800000,   

  


  dontIgnoreErrors: false,

  




  didReportProlongedError: false,

  init: function init() {
    Svc.Obs.add("weave:engine:sync:applied", this);
    Svc.Obs.add("weave:engine:sync:error", this);
    Svc.Obs.add("weave:service:login:error", this);
    Svc.Obs.add("weave:service:sync:error", this);
    Svc.Obs.add("weave:service:sync:finish", this);

    this.initLogs();
  },

  initLogs: function initLogs() {
    this._log = Log.repository.getLogger("Sync.ErrorHandler");
    this._log.level = Log.Level[Svc.Prefs.get("log.logger.service.main")];

    let root = Log.repository.getLogger("Sync");
    root.level = Log.Level[Svc.Prefs.get("log.rootLogger")];

    let logs = ["Sync", "FirefoxAccounts", "Hawk", "Common.TokenServerClient",
                "Sync.SyncMigration"];

    this._logManager = new LogManager(Svc.Prefs, logs, "sync");
  },

  observe: function observe(subject, topic, data) {
    this._log.trace("Handling " + topic);
    switch(topic) {
      case "weave:engine:sync:applied":
        if (subject.newFailed) {
          
          
          
          Status.engines = [data, ENGINE_APPLY_FAIL];
          this._log.debug(data + " failed to apply some records.");
        }
        break;
      case "weave:engine:sync:error":
        let exception = subject;  
        let engine_name = data;   

        this.checkServerError(exception);

        Status.engines = [engine_name, exception.failureCode || ENGINE_UNKNOWN_FAIL];
        this._log.debug(engine_name + " failed: " + Utils.exceptionStr(exception));
        break;
      case "weave:service:login:error":
        this.resetFileLog(this._logManager.REASON_ERROR);

        if (this.shouldReportError()) {
          this.notifyOnNextTick("weave:ui:login:error");
        } else {
          this.notifyOnNextTick("weave:ui:clear-error");
        }

        this.dontIgnoreErrors = false;
        break;
      case "weave:service:sync:error":
        if (Status.sync == CREDENTIALS_CHANGED) {
          this.service.logout();
        }

        this.resetFileLog(this._logManager.REASON_ERROR);

        if (this.shouldReportError()) {
          this.notifyOnNextTick("weave:ui:sync:error");
        } else {
          this.notifyOnNextTick("weave:ui:sync:finish");
        }

        this.dontIgnoreErrors = false;
        break;
      case "weave:service:sync:finish":
        this._log.trace("Status.service is " + Status.service);

        
        
        
        
        if (Status.sync    == SYNC_SUCCEEDED &&
            Status.service == STATUS_OK) {
          
          this._log.trace("Clearing lastSyncReassigned.");
          Svc.Prefs.reset("lastSyncReassigned");
        }

        if (Status.service == SYNC_FAILED_PARTIAL) {
          this._log.debug("Some engines did not sync correctly.");
          this.resetFileLog(this._logManager.REASON_ERROR);

          if (this.shouldReportError()) {
            this.dontIgnoreErrors = false;
            this.notifyOnNextTick("weave:ui:sync:error");
            break;
          }
        } else {
          this.resetFileLog(this._logManager.REASON_SUCCESS);
        }
        this.dontIgnoreErrors = false;
        this.notifyOnNextTick("weave:ui:sync:finish");
        break;
    }
  },

  notifyOnNextTick: function notifyOnNextTick(topic) {
    Utils.nextTick(function() {
      this._log.trace("Notifying " + topic +
                      ". Status.login is " + Status.login +
                      ". Status.sync is " + Status.sync);
      Svc.Obs.notify(topic);
    }, this);
  },

  


  syncAndReportErrors: function syncAndReportErrors() {
    this._log.debug("Beginning user-triggered sync.");

    this.dontIgnoreErrors = true;
    Utils.nextTick(this.service.sync, this.service);
  },

  







  resetFileLog: function resetFileLog(reason) {
    let onComplete = () => {
      Svc.Obs.notify("weave:service:reset-file-log");
      this._log.trace("Notified: " + Date.now());
    };
    
    
    this._logManager.resetFileLog(reason).then(onComplete, onComplete);
  },

  





  errorStr: function errorStr(code) {
    switch (code.toString()) {
    case "1":
      return "illegal-method";
    case "2":
      return "invalid-captcha";
    case "3":
      return "invalid-username";
    case "4":
      return "cannot-overwrite-resource";
    case "5":
      return "userid-mismatch";
    case "6":
      return "json-parse-failure";
    case "7":
      return "invalid-password";
    case "8":
      return "invalid-record";
    case "9":
      return "weak-password";
    default:
      return "generic-server-error";
    }
  },

  shouldReportError: function shouldReportError() {
    if (Status.login == MASTER_PASSWORD_LOCKED) {
      this._log.trace("shouldReportError: false (master password locked).");
      return false;
    }

    if (this.dontIgnoreErrors) {
      return true;
    }

    if (Status.login == LOGIN_FAILED_LOGIN_REJECTED) {
      
      this._log.trace("shouldReportError: true (login was rejected)");
      return true;
    }

    let lastSync = Svc.Prefs.get("lastSync");
    if (lastSync && ((Date.now() - Date.parse(lastSync)) >
        Svc.Prefs.get("errorhandler.networkFailureReportTimeout") * 1000)) {
      Status.sync = PROLONGED_SYNC_FAILURE;
      if (this.didReportProlongedError) {
        this._log.trace("shouldReportError: false (prolonged sync failure, but" +
                        " we've already reported it).");
        return false;
      }
      this._log.trace("shouldReportError: true (first prolonged sync failure).");
      this.didReportProlongedError = true;
      return true;
    }

    
    
    
    if (!this.service.clusterURL) {
      this._log.trace("shouldReportError: false (no cluster URL; " +
                      "possible node reassignment).");
      return false;
    }


    let result = ([Status.login, Status.sync].indexOf(SERVER_MAINTENANCE) == -1 &&
                  [Status.login, Status.sync].indexOf(LOGIN_FAILED_NETWORK_ERROR) == -1);
    this._log.trace("shouldReportError: ${result} due to login=${login}, sync=${sync}",
                    {result, login: Status.login, sync: Status.sync});
    return result;
  },

  get currentAlertMode() {
    return Svc.Prefs.get("errorhandler.alert.mode");
  },

  set currentAlertMode(str) {
    return Svc.Prefs.set("errorhandler.alert.mode", str);
  },

  get earliestNextAlert() {
    return Svc.Prefs.get("errorhandler.alert.earliestNext", 0) * 1000;
  },

  set earliestNextAlert(msec) {
    return Svc.Prefs.set("errorhandler.alert.earliestNext", msec / 1000);
  },

  clearServerAlerts: function () {
    
    Svc.Prefs.resetBranch("errorhandler.alert");
  },

  








  handleServerAlert: function (xwa) {
    if (!xwa.code) {
      this._log.warn("Got structured X-Weave-Alert, but no alert code.");
      return;
    }

    switch (xwa.code) {
      
      
      case "soft-eol":
        

      
      
      case "hard-eol":
        
        
        if ((this.currentAlertMode != xwa.code) ||
            (this.earliestNextAlert < Date.now())) {
          Utils.nextTick(function() {
            Svc.Obs.notify("weave:eol", xwa);
          }, this);
          this._log.error("X-Weave-Alert: " + xwa.code + ": " + xwa.message);
          this.earliestNextAlert = Date.now() + this.MINIMUM_ALERT_INTERVAL_MSEC;
          this.currentAlertMode = xwa.code;
        }
        break;
      default:
        this._log.debug("Got unexpected X-Weave-Alert code: " + xwa.code);
    }
  },

  





  checkServerError: function (resp) {
    switch (resp.status) {
      case 200:
      case 404:
      case 513:
        let xwa = resp.headers['x-weave-alert'];

        
        if (!xwa || !xwa.startsWith("{")) {
          this.clearServerAlerts();
          return;
        }

        try {
          xwa = JSON.parse(xwa);
        } catch (ex) {
          this._log.warn("Malformed X-Weave-Alert from server: " + xwa);
          return;
        }

        this.handleServerAlert(xwa);
        break;

      case 400:
        if (resp == RESPONSE_OVER_QUOTA) {
          Status.sync = OVER_QUOTA;
        }
        break;

      case 401:
        this.service.logout();
        this._log.info("Got 401 response; resetting clusterURL.");
        Svc.Prefs.reset("clusterURL");

        let delay = 0;
        if (Svc.Prefs.get("lastSyncReassigned")) {
          
          
          
          
          
          this._log.warn("Last sync also failed for 401. Delaying next sync.");
          delay = MINIMUM_BACKOFF_INTERVAL;
        } else {
          this._log.debug("New mid-sync 401 failure. Making a note.");
          Svc.Prefs.set("lastSyncReassigned", true);
        }
        this._log.info("Attempting to schedule another sync.");
        this.service.scheduler.scheduleNextSync(delay);
        break;

      case 500:
      case 502:
      case 503:
      case 504:
        Status.enforceBackoff = true;
        if (resp.status == 503 && resp.headers["retry-after"]) {
          let retryAfter = resp.headers["retry-after"];
          this._log.debug("Got Retry-After: " + retryAfter);
          if (this.service.isLoggedIn) {
            Status.sync = SERVER_MAINTENANCE;
          } else {
            Status.login = SERVER_MAINTENANCE;
          }
          Svc.Obs.notify("weave:service:backoff:interval",
                         parseInt(retryAfter, 10));
        }
        break;
    }

    switch (resp.result) {
      case Cr.NS_ERROR_UNKNOWN_HOST:
      case Cr.NS_ERROR_CONNECTION_REFUSED:
      case Cr.NS_ERROR_NET_TIMEOUT:
      case Cr.NS_ERROR_NET_RESET:
      case Cr.NS_ERROR_NET_INTERRUPT:
      case Cr.NS_ERROR_PROXY_CONNECTION_REFUSED:
        
        
        if (this.service.isLoggedIn) {
          Status.sync = LOGIN_FAILED_NETWORK_ERROR;
        } else {
          Status.login = LOGIN_FAILED_NETWORK_ERROR;
        }
        break;
    }
  },
};
