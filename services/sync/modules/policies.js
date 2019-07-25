





































const EXPORTED_SYMBOLS = ["SyncScheduler"];

const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/status.js");

Cu.import("resource://services-sync/main.js");    

let SyncScheduler = {
  _log: Log4Moz.repository.getLogger("Sync.SyncScheduler"),

  
  get nextSync() Svc.Prefs.get("nextSync", 0) * 1000,
  set nextSync(value) Svc.Prefs.set("nextSync", Math.floor(value / 1000)),

  get syncInterval() {
    
    if (Status.partial && Clients.clientType != "mobile")
      return PARTIAL_DATA_SYNC;
    return Svc.Prefs.get("syncInterval", MULTI_MOBILE_SYNC);
  },
  set syncInterval(value) Svc.Prefs.set("syncInterval", value),

  get syncThreshold() Svc.Prefs.get("syncThreshold", SINGLE_USER_THRESHOLD),
  set syncThreshold(value) Svc.Prefs.set("syncThreshold", value),

  get globalScore() Svc.Prefs.get("globalScore", 0),
  set globalScore(value) Svc.Prefs.set("globalScore", value),

  init: function init() {
    Svc.Obs.add("weave:engine:score:updated", this);
    Svc.Obs.add("network:offline-status-changed", this);
    Svc.Obs.add("weave:service:sync:start", this);
    Svc.Obs.add("weave:service:sync:finish", this);
    Svc.Obs.add("weave:engine:sync:finish", this);
    Svc.Obs.add("weave:service:login:error", this);
    Svc.Obs.add("weave:service:logout:finish", this);
    Svc.Obs.add("weave:service:sync:error", this);
    Svc.Obs.add("weave:service:backoff:interval", this);
  },

  observe: function observe(subject, topic, data) {
    switch(topic) {
      case "weave:engine:score:updated":
        Utils.namedTimer(this.calculateScore, SCORE_UPDATE_DELAY, this,
                         "_scoreTimer");
        break;
      case "network:offline-status-changed":
        
        this._log.trace("Network offline status change: " + data);
        this.checkSyncStatus();
        break;
      case "weave:service:sync:start":
        
        this.clearSyncTriggers();
        this.nextSync = 0;

        
        
        Status.resetBackoff();

        this.globalScore = 0;
        break;
      case "weave:service:sync:finish":
        let sync_interval;
        this._syncErrors = 0;

        if (Status.sync == NO_SYNC_NODE_FOUND) {
          sync_interval = NO_SYNC_NODE_INTERVAL;
        }
        this.scheduleNextSync(sync_interval);
        break;
      case "weave:engine:sync:finish":
        if (subject == "clients") {
          
          this.updateClientMode();
        }
        break;
      case "weave:service:login:error":
        this.clearSyncTriggers();
        
        
        
        if (!this.skipScheduledRetry())
          this.scheduleAtInterval(MASTER_PASSWORD_LOCKED_RETRY_INTERVAL);
        break;
      case "weave:service:logout:finish":
        
        
        this.checkSyncStatus();
        break; 
      case "weave:service:sync:error":
        this.handleSyncError();
        break;
      case "weave:service:backoff:interval":
        let interval = (data + Math.random() * data * 0.25) * 1000; 
        Status.backoffInterval = interval;
        Status.minimumNextSync = Date.now() + data;
        break;
      }
  },

  calculateScore: function calculateScore() {
    var engines = Engines.getEnabled();
    for (let i = 0;i < engines.length;i++) {
      this._log.trace(engines[i].name + ": score: " + engines[i].score);
      this.globalScore += engines[i].score;
      engines[i]._tracker.resetScore();
    }

    this._log.trace("Global score updated: " + this.globalScore);
    this.checkSyncStatus();
  },

  


  updateClientMode: function updateClientMode() {
    
    let {numClients, hasMobile} = Clients.stats;
    if (Weave.Service.numClients == numClients)
      return;

    this._log.debug("Client count: " + Weave.Service.numClients + " -> " + numClients);
    Weave.Service.numClients = numClients;

    if (numClients == 1) {
      this.syncInterval = SINGLE_USER_SYNC;
      this.syncThreshold = SINGLE_USER_THRESHOLD;
    }
    else {
      this.syncInterval = hasMobile ? MULTI_MOBILE_SYNC : MULTI_DESKTOP_SYNC;
      this.syncThreshold = MULTI_DEVICE_THRESHOLD;
    }
  },

  


  checkSyncStatus: function checkSyncStatus() {
    
    
    let ignore = [kSyncBackoffNotMet];

    
    
    if (Utils.mpLocked()) {
      ignore.push(kSyncNotLoggedIn);
      ignore.push(kSyncMasterPasswordLocked);
    }

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

      
      this.scheduleAtInterval(MASTER_PASSWORD_LOCKED_RETRY_INTERVAL);
      return;
    }

    Utils.nextTick(Weave.Service.sync, Weave.Service);
  },

  


  scheduleNextSync: function scheduleNextSync(interval) {
    
    if (interval == null || interval == undefined) {
      
      if (this.nextSync != 0)
        interval = this.nextSync - Date.now();
      
      else
        interval = Math.max(this.syncInterval, Status.backoffInterval);
    }

    
    if (interval <= 0) {
      this.syncIfMPUnlocked();
      return;
    }

    this._log.trace("Next sync in " + Math.ceil(interval / 1000) + " sec.");
    Utils.namedTimer(this._syncIfMPUnlocked, interval, this, "_syncTimer");

    
    this.nextSync = Date.now() + interval;
  },


  



  scheduleAtInterval: function scheduleAtInterval(minimumInterval) {
    let interval = Utils.calculateBackoff(this._syncErrors, MINIMUM_BACKOFF_INTERVAL);
    if (minimumInterval)
      interval = Math.max(minimumInterval, interval);

    let d = new Date(Date.now() + interval);
    this._log.config("Starting backoff, next sync at:" + d.toString());

    this.scheduleNextSync(interval);
  },

  skipScheduledRetry: function skipScheduledRetry() {
    return [LOGIN_FAILED_INVALID_PASSPHRASE,
            LOGIN_FAILED_LOGIN_REJECTED].indexOf(Status.login) == -1;
  },

  _syncErrors: 0,
  


  handleSyncError: function handleSyncError() {
    this._syncErrors++;

    
    
    if (!Status.enforceBackoff) {
      if (this._syncErrors < MAX_ERROR_COUNT_BEFORE_BACKOFF) {
        this.scheduleNextSync();
        return;
      }
      Status.enforceBackoff = true;
    }

    this.scheduleAtInterval();
  },


  


  clearSyncTriggers: function clearSyncTriggers() {
    this._log.debug("Clearing sync triggers.");

    
    if (this._syncTimer)
      this._syncTimer.clear();
  }

};
