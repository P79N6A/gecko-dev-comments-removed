







this.EXPORTED_SYMBOLS = ["EngineSynchronizer"];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/util.js");






this.EngineSynchronizer = function EngineSynchronizer(service) {
  this._log = Log.repository.getLogger("Sync.Synchronizer");
  this._log.level = Log.Level[Svc.Prefs.get("log.logger.synchronizer")];

  this.service = service;

  this.onComplete = null;
}

EngineSynchronizer.prototype = {
  sync: function sync() {
    if (!this.onComplete) {
      throw new Error("onComplete handler not installed.");
    }

    let startTime = Date.now();

    this.service.status.resetSync();

    
    let reason = this.service._checkSync();
    if (reason) {
      if (reason == kSyncNetworkOffline) {
        this.service.status.sync = LOGIN_FAILED_NETWORK_ERROR;
      }

      
      
      reason = "Can't sync: " + reason;
      this.onComplete(new Error("Can't sync: " + reason));
      return;
    }

    
    if (!this.service.clusterURL && !this.service._clusterManager.setCluster()) {
      this.service.status.sync = NO_SYNC_NODE_FOUND;
      this._log.info("No cluster URL found. Cannot sync.");
      this.onComplete(null);
      return;
    }

    
    let infoURL = this.service.infoURL;
    let now = Math.floor(Date.now() / 1000);
    let lastPing = Svc.Prefs.get("lastPing", 0);
    if (now - lastPing > 86400) { 
      infoURL += "?v=" + WEAVE_VERSION;
      Svc.Prefs.set("lastPing", now);
    }

    let engineManager = this.service.engineManager;

    
    let info = this.service._fetchInfo(infoURL);

    
    for (let engine of [this.service.clientsEngine].concat(engineManager.getAll())) {
      engine.lastModified = info.obj[engine.name] || 0;
    }

    if (!(this.service._remoteSetup(info))) {
      this.onComplete(new Error("Aborting sync, remote setup failed"));
      return;
    }

    
    this._log.debug("Refreshing client list.");
    if (!this._syncEngine(this.service.clientsEngine)) {
      
      
      this._log.warn("Client engine sync failed. Aborting.");
      this.onComplete(null);
      return;
    }

    
    switch (Svc.Prefs.get("firstSync")) {
      case "resetClient":
        this.service.resetClient(engineManager.enabledEngineNames);
        break;
      case "wipeClient":
        this.service.wipeClient(engineManager.enabledEngineNames);
        break;
      case "wipeRemote":
        this.service.wipeRemote(engineManager.enabledEngineNames);
        break;
    }

    if (this.service.clientsEngine.localCommands) {
      try {
        if (!(this.service.clientsEngine.processIncomingCommands())) {
          this.service.status.sync = ABORT_SYNC_COMMAND;
          this.onComplete(new Error("Processed command aborted sync."));
          return;
        }

        
        if (!(this.service._remoteSetup(info))) {
          this.onComplete(new Error("Remote setup failed after processing commands."));
          return;
        }
      }
      finally {
        
        
        
        
        this._syncEngine(this.service.clientsEngine);
      }
    }

    
    try {
      this._updateEnabledEngines();
    } catch (ex) {
      this._log.debug("Updating enabled engines failed: " +
                      Utils.exceptionStr(ex));
      this.service.errorHandler.checkServerError(ex);
      this.onComplete(ex);
      return;
    }

    try {
      for (let engine of engineManager.getEnabled()) {
        
        if (!(this._syncEngine(engine)) || this.service.status.enforceBackoff) {
          this._log.info("Aborting sync for failure in " + engine.name);
          break;
        }
      }

      
      
      
      if (!this.service.clusterURL) {
        this._log.debug("Aborting sync, no cluster URL: " +
                        "not uploading new meta/global.");
        this.onComplete(null);
        return;
      }

      
      let meta = this.service.recordManager.get(this.service.metaURL);
      if (meta.isNew || meta.changed) {
        this._log.info("meta/global changed locally: reuploading.");
        try {
          this.service.uploadMetaGlobal(meta);
          delete meta.isNew;
          delete meta.changed;
        } catch (error) {
          this._log.error("Unable to upload meta/global. Leaving marked as new.");
        }
      }

      
      if (this.service.status.service != SYNC_FAILED_PARTIAL) {
        Svc.Prefs.set("lastSync", new Date().toString());
        this.service.status.sync = SYNC_SUCCEEDED;
      }
    } finally {
      Svc.Prefs.reset("firstSync");

      let syncTime = ((Date.now() - startTime) / 1000).toFixed(2);
      let dateStr = new Date().toLocaleFormat(LOG_DATE_FORMAT);
      this._log.info("Sync completed at " + dateStr
                     + " after " + syncTime + " secs.");
    }

    this.onComplete(null);
  },

  
  
  _syncEngine: function _syncEngine(engine) {
    try {
      engine.sync();
    }
    catch(e) {
      if (e.status == 401) {
        
        
        
        
        
        return false;
      }
    }

    return true;
  },

  _updateEnabledFromMeta: function (meta, numClients, engineManager=this.service.engineManager) {
    this._log.info("Updating enabled engines: " +
                    numClients + " clients.");

    if (meta.isNew || !meta.payload.engines) {
      this._log.debug("meta/global isn't new, or is missing engines. Not updating enabled state.");
      return;
    }

    
    
    
    if ((numClients <= 1) &&
        ([e for (e in meta.payload.engines) if (e != "clients")].length == 0)) {
      this._log.info("One client and no enabled engines: not touching local engine status.");
      return;
    }

    this.service._ignorePrefObserver = true;

    let enabled = engineManager.enabledEngineNames;

    let toDecline = new Set();
    let toUndecline = new Set();

    for (let engineName in meta.payload.engines) {
      if (engineName == "clients") {
        
        continue;
      }
      let index = enabled.indexOf(engineName);
      if (index != -1) {
        
        enabled.splice(index, 1);
        continue;
      }
      let engine = engineManager.get(engineName);
      if (!engine) {
        
        continue;
      }

      if (Svc.Prefs.get("engineStatusChanged." + engine.prefName, false)) {
        
        
        this._log.trace("Wiping data for " + engineName + " engine.");
        engine.wipeServer();
        delete meta.payload.engines[engineName];
        meta.changed = true;             

        
        
        
        this._log.trace("Engine " + engineName + " was disabled locally. Marking as declined.");
        toDecline.add(engineName);
      } else {
        
        this._log.trace("Engine " + engineName + " was enabled. Marking as non-declined.");
        toUndecline.add(engineName);
        this._log.trace(engineName + " engine was enabled remotely.");
        engine.enabled = true;
      }
    }

    
    for each (let engineName in enabled) {
      let engine = engineManager.get(engineName);
      if (Svc.Prefs.get("engineStatusChanged." + engine.prefName, false)) {
        this._log.trace("The " + engineName + " engine was enabled locally.");
        toUndecline.add(engineName);
      } else {
        this._log.trace("The " + engineName + " engine was disabled remotely.");

        
        engine.enabled = false;
      }
    }

    this.service.engineManager.decline(toDecline);
    this.service.engineManager.undecline(toUndecline);

    Svc.Prefs.resetBranch("engineStatusChanged.");
    this.service._ignorePrefObserver = false;
  },

  _updateEnabledEngines: function () {
    let meta = this.service.recordManager.get(this.service.metaURL);
    let numClients = this.service.scheduler.numClients;
    let engineManager = this.service.engineManager;

    this._updateEnabledFromMeta(meta, numClients, engineManager);
  },
};
Object.freeze(EngineSynchronizer.prototype);
