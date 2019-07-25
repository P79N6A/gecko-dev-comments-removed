



const EXPORTED_SYMBOLS = ["SyncScheduler",
                          "ErrorHandler",
                          "SendCredentialsController"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/status.js");

Cu.import("resource://services-sync/main.js");    

let SyncScheduler = {
  _log: Log4Moz.repository.getLogger("Sync.SyncScheduler"),

  _fatalLoginStatus: [LOGIN_FAILED_NO_USERNAME,
                      LOGIN_FAILED_NO_PASSWORD,
                      LOGIN_FAILED_NO_PASSPHRASE,
                      LOGIN_FAILED_INVALID_PASSPHRASE,
                      LOGIN_FAILED_LOGIN_REJECTED],

  


  syncTimer: null,

  setDefaults: function setDefaults() {
    this._log.trace("Setting SyncScheduler policy values to defaults.");

    this.singleDeviceInterval = Svc.Prefs.get("scheduler.singleDeviceInterval") * 1000;
    this.idleInterval         = Svc.Prefs.get("scheduler.idleInterval")         * 1000;
    this.activeInterval       = Svc.Prefs.get("scheduler.activeInterval")       * 1000;
    this.immediateInterval    = Svc.Prefs.get("scheduler.immediateInterval")    * 1000;

    
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
    this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];
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

    if (Status.checkSetup() == STATUS_OK) {
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
      case "weave:service:backoff:interval":
        let requested_interval = subject * 1000;
        
        let interval = requested_interval * (1 + Math.random() * 0.25);
        Status.backoffInterval = interval;
        Status.minimumNextSync = Date.now() + requested_interval;
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
         break;
      case "weave:service:start-over":
         SyncScheduler.setDefaults();
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
      case "back":
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
    }
  },

  adjustSyncInterval: function adjustSyncInterval() {
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
    let engines = [Clients].concat(Engines.getEnabled());
    for (let i = 0;i < engines.length;i++) {
      this._log.trace(engines[i].name + ": score: " + engines[i].score);
      this.globalScore += engines[i].score;
      engines[i]._tracker.resetScore();
    }

    this._log.trace("Global score updated: " + this.globalScore);
    this.checkSyncStatus();
  },

  


  updateClientMode: function updateClientMode() {
    
    let numClients = Clients.stats.numClients;
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
    let skip = Weave.Service._checkSync(ignore);
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

    Utils.nextTick(Weave.Service.sync, Weave.Service);
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
    if (Weave.Service._checkSetup() == STATUS_OK) {
      Utils.namedTimer(this.autoConnect, delay * 1000, this, "_autoTimer");
    }
  },

  autoConnect: function autoConnect() {
    if (Weave.Service._checkSetup() == STATUS_OK && !Weave.Service._checkSync()) {
      
      
      
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
  }

};

const LOG_PREFIX_SUCCESS = "success-";
const LOG_PREFIX_ERROR   = "error-";

let ErrorHandler = {

  


  dontIgnoreErrors: false,

  init: function init() {
    Svc.Obs.add("weave:engine:sync:applied", this);
    Svc.Obs.add("weave:engine:sync:error", this);
    Svc.Obs.add("weave:service:login:error", this);
    Svc.Obs.add("weave:service:sync:error", this);
    Svc.Obs.add("weave:service:sync:finish", this);

    this.initLogs();
  },

  initLogs: function initLogs() {
    this._log = Log4Moz.repository.getLogger("Sync.ErrorHandler");
    this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];
    this._cleaningUpFileLogs = false;

    let root = Log4Moz.repository.getLogger("Sync");
    root.level = Log4Moz.Level[Svc.Prefs.get("log.rootLogger")];

    let formatter = new Log4Moz.BasicFormatter();
    let capp = new Log4Moz.ConsoleAppender(formatter);
    capp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.console")];
    root.addAppender(capp);

    let dapp = new Log4Moz.DumpAppender(formatter);
    dapp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.dump")];
    root.addAppender(dapp);

    let fapp = this._logAppender = new Log4Moz.StorageStreamAppender(formatter);
    fapp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.file.level")];
    root.addAppender(fapp);
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
        this.resetFileLog(Svc.Prefs.get("log.appender.file.logOnError"),
                          LOG_PREFIX_ERROR);

        if (this.shouldReportError()) {
          this.notifyOnNextTick("weave:ui:login:error");
        } else {
          this.notifyOnNextTick("weave:ui:clear-error");
        }

        this.dontIgnoreErrors = false;
        break;
      case "weave:service:sync:error":
        if (Status.sync == CREDENTIALS_CHANGED) {
          Weave.Service.logout();
        }

        this.resetFileLog(Svc.Prefs.get("log.appender.file.logOnError"),
                          LOG_PREFIX_ERROR);

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
          this.resetFileLog(Svc.Prefs.get("log.appender.file.logOnError"),
                            LOG_PREFIX_ERROR);

          if (this.shouldReportError()) {
            this.dontIgnoreErrors = false;
            this.notifyOnNextTick("weave:ui:sync:error");
            break;
          }
        } else {
          this.resetFileLog(Svc.Prefs.get("log.appender.file.logOnSuccess"),
                            LOG_PREFIX_SUCCESS);
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
    Utils.nextTick(Weave.Service.sync, Weave.Service);
  },

  


  cleanupLogs: function cleanupLogs() {
    let direntries = FileUtils.getDir("ProfD", ["weave", "logs"]).directoryEntries;
    let oldLogs = [];
    let index = 0;
    let threshold = Date.now() - 1000 * Svc.Prefs.get("log.appender.file.maxErrorAge");

    while (direntries.hasMoreElements()) {
      let logFile = direntries.getNext().QueryInterface(Ci.nsIFile);
      if (logFile.lastModifiedTime < threshold) {
        oldLogs.push(logFile);
      }
    }

    
    function deleteFile() {
      if (index >= oldLogs.length) {
        ErrorHandler._cleaningUpFileLogs = false;
        Svc.Obs.notify("weave:service:cleanup-logs");
        return;
      }
      try {
        oldLogs[index].remove(false);
      } catch (ex) {
        ErrorHandler._log._debug("Encountered error trying to clean up old log file '"
                                 + oldLogs[index].leafName + "':"
                                 + Utils.exceptionStr(ex));
      }
      index++;
      Utils.nextTick(deleteFile);
    }

    if (oldLogs.length > 0) {
      ErrorHandler._cleaningUpFileLogs = true;
      Utils.nextTick(deleteFile);
    }
  },

  










  resetFileLog: function resetFileLog(flushToFile, filenamePrefix) {
    let inStream = this._logAppender.getInputStream();
    this._logAppender.reset();
    if (flushToFile && inStream) {
      try {
        let filename = filenamePrefix + Date.now() + ".txt";
        let file = FileUtils.getFile("ProfD", ["weave", "logs", filename]);
        let outStream = FileUtils.openFileOutputStream(file);
        NetUtil.asyncCopy(inStream, outStream, function () {
          Svc.Obs.notify("weave:service:reset-file-log");
          if (filenamePrefix == LOG_PREFIX_ERROR
              && !ErrorHandler._cleaningUpFileLogs) {
            Utils.nextTick(ErrorHandler.cleanupLogs, ErrorHandler);
          }
        });
      } catch (ex) {
        Svc.Obs.notify("weave:service:reset-file-log");
      }
    } else {
      Svc.Obs.notify("weave:service:reset-file-log");
    }
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

    let lastSync = Svc.Prefs.get("lastSync");
    if (lastSync && ((Date.now() - Date.parse(lastSync)) >
        Svc.Prefs.get("errorhandler.networkFailureReportTimeout") * 1000)) {
      Status.sync = PROLONGED_SYNC_FAILURE;
      this._log.trace("shouldReportError: true (prolonged sync failure).");
      return true;
    }
 
    
    
    
    if (!Weave.Service.clusterURL) {
      this._log.trace("shouldReportError: false (no cluster URL; " +
                      "possible node reassignment).");
      return false;
    }

    return ([Status.login, Status.sync].indexOf(SERVER_MAINTENANCE) == -1 &&
            [Status.login, Status.sync].indexOf(LOGIN_FAILED_NETWORK_ERROR) == -1);
  },

  



  checkServerError: function checkServerError(resp) {
    switch (resp.status) {
      case 400:
        if (resp == RESPONSE_OVER_QUOTA) {
          Status.sync = OVER_QUOTA;
        }
        break;

      case 401:
        Weave.Service.logout();
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
        SyncScheduler.scheduleNextSync(delay);
        break;

      case 500:
      case 502:
      case 503:
      case 504:
        Status.enforceBackoff = true;
        if (resp.status == 503 && resp.headers["retry-after"]) {
          if (Weave.Service.isLoggedIn) {
            Status.sync = SERVER_MAINTENANCE;
          } else {
            Status.login = SERVER_MAINTENANCE;
          }
          Svc.Obs.notify("weave:service:backoff:interval",
                         parseInt(resp.headers["retry-after"], 10));
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
        
        
        if (Weave.Service.isLoggedIn) {
          Status.sync = LOGIN_FAILED_NETWORK_ERROR;
        } else {
          Status.login = LOGIN_FAILED_NETWORK_ERROR;
        }
        break;
    }
  },
};



















function SendCredentialsController(jpakeclient) {
  this._log = Log4Moz.repository.getLogger("Sync.SendCredentialsController");
  this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];

  this._log.trace("Loading.");
  this.jpakeclient = jpakeclient;

  
  
  
  
  Services.obs.addObserver(this, "weave:service:sync:finish", false);
  Services.obs.addObserver(this, "weave:service:sync:error",  false);
  Services.obs.addObserver(this, "weave:service:start-over",  false);
}
SendCredentialsController.prototype = {

  unload: function unload() {
    this._log.trace("Unloading.");
    try {
      Services.obs.removeObserver(this, "weave:service:sync:finish");
      Services.obs.removeObserver(this, "weave:service:sync:error");
      Services.obs.removeObserver(this, "weave:service:start-over");
    } catch (ex) {
      
    }
  },

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "weave:service:sync:finish":
      case "weave:service:sync:error":
        Utils.nextTick(this.sendCredentials, this);
        break;
      case "weave:service:start-over":
        
        this.jpakeclient.abort();
        break;
    }
  },

  sendCredentials: function sendCredentials() {
    this._log.trace("Sending credentials.");
    let credentials = {account:   Weave.Identity.account,
                       password:  Weave.Identity.basicPassword,
                       synckey:   Weave.Identity.syncKey,
                       serverURL: Weave.Service.serverURL};
    this.jpakeclient.sendAndComplete(credentials);
  },

  

  onComplete: function onComplete() {
    this._log.debug("Exchange was completed successfully!");
    this.unload();

    
    
    SyncScheduler.scheduleNextSync(SyncScheduler.activeInterval);
  },

  onAbort: function onAbort(error) {
    
    
    this._log.debug("Exchange was aborted with error: " + error);
    this.unload();
  },

  
  displayPIN: function displayPIN() {},
  onPairingStart: function onPairingStart() {},
  onPaired: function onPaired() {}
};
