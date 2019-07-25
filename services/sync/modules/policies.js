






































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

  


  syncTimer: null,

  setDefaults: function setDefaults() {
    this._log.trace("Setting SyncScheduler policy values to defaults.");

    this.singleDeviceInterval = Svc.Prefs.get("scheduler.singleDeviceInterval") * 1000;
    this.idleInterval         = Svc.Prefs.get("scheduler.idleInterval")         * 1000;
    this.activeInterval       = Svc.Prefs.get("scheduler.activeInterval")       * 1000;
    this.immediateInterval    = Svc.Prefs.get("scheduler.immediateInterval")    * 1000;

    
    this.idle = false;

    this.hasIncomingItems = false;
    this.numClients = 0;

    this.nextSync = 0,
    this.syncInterval = this.singleDeviceInterval;
    this.syncThreshold = SINGLE_USER_THRESHOLD;
  },

  get globalScore() Svc.Prefs.get("globalScore", 0),
  set globalScore(value) Svc.Prefs.set("globalScore", value),

  init: function init() {
    this._log.level = Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];
    this.setDefaults();
    Svc.Obs.add("weave:engine:score:updated", this);
    Svc.Obs.add("network:offline-status-changed", this);
    Svc.Obs.add("weave:service:sync:start", this);
    Svc.Obs.add("weave:service:sync:finish", this);
    Svc.Obs.add("weave:engine:sync:finish", this);
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
        this.adjustSyncInterval();

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
      case "weave:service:login:error":
        this.clearSyncTriggers();
        
        
        
        if (Status.login == MASTER_PASSWORD_LOCKED) {
          this._log.debug("Couldn't log in: master password is locked.");
          this._log.trace("Scheduling a sync at MASTER_PASSWORD_LOCKED_RETRY_INTERVAL");
          this.scheduleAtInterval(MASTER_PASSWORD_LOCKED_RETRY_INTERVAL);
        }
        break;
      case "weave:service:logout:finish":
        
        
        this.checkSyncStatus();
        break; 
      case "weave:service:sync:error":
        
        
        this.updateClientMode();
        this.adjustSyncInterval();
        this.handleSyncError();
        break;
      case "weave:service:backoff:interval":
        let interval = (data + Math.random() * data * 0.25) * 1000; 
        Status.backoffInterval = interval;
        Status.minimumNextSync = Date.now() + data;
        break;
      case "weave:service:ready":
        
        
        let delay = Svc.Prefs.get("autoconnectDelay");
        if (delay) {
          this.delayedAutoConnect(delay);
        }
        break;
      case "weave:engine:sync:applied":
        let numItems = subject.applied;
        this._log.trace("Engine " + data + " applied " + numItems + " items.");
        if (numItems) 
          this.hasIncomingItems = true;
        break;
      case "weave:service:setup-complete":
         Svc.Idle.addIdleObserver(this, Svc.Prefs.get("scheduler.idleTime"));
         break;
      case "weave:service:start-over":
         Svc.Idle.removeIdleObserver(this, Svc.Prefs.get("scheduler.idleTime"));
         SyncScheduler.setDefaults();
         break;
      case "idle":
        this._log.trace("We're idle.");
        this.idle = true;
        
        
        
        this.adjustSyncInterval();
        break;
      case "back":
        this._log.trace("We're no longer idle.");
        this.idle = false;
        
        if (this.numClients > 1) {
          Utils.nextTick(Weave.Service.sync, Weave.Service);
        }
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
    
    let {numClients} = Clients.stats;	
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
    
    if (interval == null || interval == undefined) {
      
      if (this.nextSync != 0)
        interval = Math.min(this.syncInterval, (this.nextSync - Date.now()));
      
      else
        interval = Math.max(this.syncInterval, Status.backoffInterval);
    }

    
    if (interval <= 0) {
      this.syncIfMPUnlocked();
      return;
    }

    this._log.trace("Next sync in " + Math.ceil(interval / 1000) + " sec.");
    Utils.namedTimer(this.syncIfMPUnlocked, interval, this, "syncTimer");

    
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

 







  delayedAutoConnect: function delayedAutoConnect(delay) {
    if (Weave.Service._checkSetup() == STATUS_OK) {
      Utils.namedTimer(this.autoConnect, delay * 1000, this, "_autoTimer");
    }
  },

  autoConnect: function autoConnect() {
    if (Weave.Service._checkSetup() == STATUS_OK && !Weave.Service._checkSync()) {
      Utils.nextTick(Weave.Service.sync, Weave.Service);
    }

    
    if (this._autoTimer) {
      this._autoTimer.clear();
    }
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

    
    if (this.syncTimer)
      this.syncTimer.clear();
  }

};
