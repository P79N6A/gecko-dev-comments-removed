



#ifndef MERGED_COMPARTMENT

"use strict";

this.EXPORTED_SYMBOLS = ["HealthReporter"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

Cu.import("resource://gre/modules/Metrics.jsm");
Cu.import("resource://services-common/async.js");

Cu.import("resource://services-common/bagheeraclient.js");
#endif

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TelemetryController",
                                  "resource://gre/modules/TelemetryController.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");



const OLDEST_ALLOWED_YEAR = 2012;

const DAYS_IN_PAYLOAD = 180;

const DEFAULT_DATABASE_NAME = "healthreport.sqlite";

const TELEMETRY_INIT = "HEALTHREPORT_INIT_MS";
const TELEMETRY_INIT_FIRSTRUN = "HEALTHREPORT_INIT_FIRSTRUN_MS";
const TELEMETRY_DB_OPEN = "HEALTHREPORT_DB_OPEN_MS";
const TELEMETRY_DB_OPEN_FIRSTRUN = "HEALTHREPORT_DB_OPEN_FIRSTRUN_MS";
const TELEMETRY_GENERATE_PAYLOAD = "HEALTHREPORT_GENERATE_JSON_PAYLOAD_MS";
const TELEMETRY_JSON_PAYLOAD_SERIALIZE = "HEALTHREPORT_JSON_PAYLOAD_SERIALIZE_MS";
const TELEMETRY_PAYLOAD_SIZE_UNCOMPRESSED = "HEALTHREPORT_PAYLOAD_UNCOMPRESSED_BYTES";
const TELEMETRY_PAYLOAD_SIZE_COMPRESSED = "HEALTHREPORT_PAYLOAD_COMPRESSED_BYTES";
const TELEMETRY_UPLOAD = "HEALTHREPORT_UPLOAD_MS";
const TELEMETRY_COLLECT_CONSTANT = "HEALTHREPORT_COLLECT_CONSTANT_DATA_MS";
const TELEMETRY_COLLECT_DAILY = "HEALTHREPORT_COLLECT_DAILY_MS";
const TELEMETRY_SHUTDOWN = "HEALTHREPORT_SHUTDOWN_MS";
const TELEMETRY_COLLECT_CHECKPOINT = "HEALTHREPORT_POST_COLLECT_CHECKPOINT_MS";





















function HealthReporterState(reporter) {
  this._reporter = reporter;

  let profD = OS.Constants.Path.profileDir;

  if (!profD || !profD.length) {
    throw new Error("Could not obtain profile directory. OS.File not " +
                    "initialized properly?");
  }

  this._log = reporter._log;

  this._stateDir = OS.Path.join(profD, "healthreport");

  
  let leaf = reporter._stateLeaf || "state.json";

  this._filename = OS.Path.join(this._stateDir, leaf);
  this._log.debug("Storing state in " + this._filename);
  this._s = null;
}

HealthReporterState.prototype = Object.freeze({
  


  get clientID() {
    return this._s.clientID;
  },

  


  get clientIDVersion() {
    return this._s.clientIDVersion;
  },

  get lastPingDate() {
    return new Date(this._s.lastPingTime);
  },

  get lastSubmitID() {
    return this._s.remoteIDs[0];
  },

  get remoteIDs() {
    return this._s.remoteIDs;
  },

  get _lastPayloadPath() {
    return OS.Path.join(this._stateDir, "lastpayload.json");
  },

  init: function () {
    return Task.spawn(function* init() {
      yield OS.File.makeDir(this._stateDir);

      let drs = Cc["@mozilla.org/datareporting/service;1"]
                  .getService(Ci.nsISupports)
                  .wrappedJSObject;
      let drsClientID = yield drs.getClientID();

      let resetObjectState = function () {
        this._s = {
          
          
          v: 1,
          
          clientID: drsClientID,
          
          
          clientIDVersion: 1,
          
          remoteIDs: [],
          
          lastPingTime: 0,
          
          removedOutdatedLastpayload: false,
        };
      }.bind(this);

      try {
        this._s = yield CommonUtils.readJSON(this._filename);
      } catch (ex if ex instanceof OS.File.Error &&
               ex.becauseNoSuchFile) {
        this._log.warn("Saved state file does not exist.");
        resetObjectState();
      } catch (ex) {
        this._log.error("Exception when reading state from disk: " +
                        CommonUtils.exceptionStr(ex));
        resetObjectState();

        
      }

      if (typeof(this._s) != "object") {
        this._log.warn("Read state is not an object. Resetting state.");
        resetObjectState();
        yield this.save();
      }

      if (this._s.v != 1) {
        this._log.warn("Unknown version in state file: " + this._s.v);
        resetObjectState();
        
        
      }

      this._s.clientID = drsClientID;

      
      
      for (let promise of this._migratePrefs()) {
        yield promise;
      }
    }.bind(this));
  },

  save: function () {
    this._log.info("Writing state file: " + this._filename);
    return CommonUtils.writeJSON(this._s, this._filename);
  },

  addRemoteID: function (id) {
    this._log.warn("Recording new remote ID: " + id);
    this._s.remoteIDs.push(id);
    return this.save();
  },

  removeRemoteID: function (id) {
    return this.removeRemoteIDs(id ? [id] : []);
  },

  removeRemoteIDs: function (ids) {
    if (!ids || !ids.length) {
      this._log.warn("No IDs passed for removal.");
      return Promise.resolve();
    }

    this._log.warn("Removing documents from remote ID list: " + ids);
    let filtered = this._s.remoteIDs.filter((x) => ids.indexOf(x) === -1);

    if (filtered.length == this._s.remoteIDs.length) {
      return Promise.resolve();
    }

    this._s.remoteIDs = filtered;
    return this.save();
  },

  setLastPingDate: function (date) {
    this._s.lastPingTime = date.getTime();

    return this.save();
  },

  updateLastPingAndRemoveRemoteID: function (date, id) {
    return this.updateLastPingAndRemoveRemoteIDs(date, id ? [id] : []);
  },

  updateLastPingAndRemoveRemoteIDs: function (date, ids) {
    if (!ids) {
      return this.setLastPingDate(date);
    }

    this._log.info("Recording last ping time and deleted remote document.");
    this._s.lastPingTime = date.getTime();
    return this.removeRemoteIDs(ids);
  },

  



  resetClientID: Task.async(function* () {
    let drs = Cc["@mozilla.org/datareporting/service;1"]
                .getService(Ci.nsISupports)
                .wrappedJSObject;
    yield drs.resetClientID();
    this._s.clientID = yield drs.getClientID();
    this._log.info("Reset client id to " + this._s.clientID + ".");

    yield this.save();
  }),

  _migratePrefs: function () {
    let prefs = this._reporter._prefs;

    let lastID = prefs.get("lastSubmitID", null);
    let lastPingDate = CommonUtils.getDatePref(prefs, "lastPingTime",
                                               0, this._log, OLDEST_ALLOWED_YEAR);

    
    
    if (lastID || (lastPingDate && lastPingDate.getTime() > 0)) {
      this._log.warn("Migrating saved state from preferences.");

      if (lastID) {
        this._log.info("Migrating last saved ID: " + lastID);
        this._s.remoteIDs.push(lastID);
      }

      let ourLast = this.lastPingDate;

      if (lastPingDate && lastPingDate.getTime() > ourLast.getTime()) {
        this._log.info("Migrating last ping time: " + lastPingDate);
        this._s.lastPingTime = lastPingDate.getTime();
      }

      yield this.save();
      prefs.reset(["lastSubmitID", "lastPingTime"]);
    } else {
      this._log.debug("No prefs data found.");
    }
  },
});






function AbstractHealthReporter(branch, policy, sessionRecorder) {
  if (!branch.endsWith(".")) {
    throw new Error("Branch must end with a period (.): " + branch);
  }

  if (!policy) {
    throw new Error("Must provide policy to HealthReporter constructor.");
  }

  this._log = Log.repository.getLogger("Services.HealthReport.HealthReporter");
  this._log.info("Initializing health reporter instance against " + branch);

  this._branch = branch;
  this._prefs = new Preferences(branch);

  this._policy = policy;
  this.sessionRecorder = sessionRecorder;

  this._dbName = this._prefs.get("dbName") || DEFAULT_DATABASE_NAME;

  this._storage = null;
  this._storageInProgress = false;
  this._providerManager = null;
  this._providerManagerInProgress = false;
  this._initializeStarted = false;
  this._initialized = false;
  this._initializeHadError = false;
  this._initializedDeferred = Promise.defer();
  this._shutdownRequested = false;
  this._shutdownInitiated = false;
  this._shutdownComplete = false;
  this._deferredShutdown = Promise.defer();
  this._promiseShutdown = this._deferredShutdown.promise;

  this._errors = [];

  this._lastDailyDate = null;

  
  let hasFirstRun = this._prefs.get("service.firstRun", false);
  this._initHistogram = hasFirstRun ? TELEMETRY_INIT : TELEMETRY_INIT_FIRSTRUN;
  this._dbOpenHistogram = hasFirstRun ? TELEMETRY_DB_OPEN : TELEMETRY_DB_OPEN_FIRSTRUN;

  
  
  
  this._currentProviderInShutdown = null;
  this._currentProviderInInit = null;
  this._currentProviderInCollect = null;
}

AbstractHealthReporter.prototype = Object.freeze({
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  




  get initialized() {
    return this._initialized;
  },

  





  init: function () {
    if (this._initializeStarted) {
      throw new Error("We have already started initialization.");
    }

    this._initializeStarted = true;

    return Task.spawn(function*() {
      TelemetryStopwatch.start(this._initHistogram, this);

      try {
        yield this._state.init();

        if (!this._state._s.removedOutdatedLastpayload) {
          yield this._deleteOldLastPayload();
          this._state._s.removedOutdatedLastpayload = true;
          
          
          
        }
      } catch (ex) {
        this._log.error("Error deleting last payload: " +
                        CommonUtils.exceptionStr(ex));
      }

      
      
      Services.obs.addObserver(this, "quit-application", false);

      
      
      Metrics.Storage.shutdown.addBlocker("FHR: Flushing storage shutdown",
        () => {
          
          
          
          
          
          this._initiateShutdown();
          return this._promiseShutdown;
        },
        () => ({
            shutdownInitiated: this._shutdownInitiated,
            initialized: this._initialized,
            shutdownRequested: this._shutdownRequested,
            initializeHadError: this._initializeHadError,
            providerManagerInProgress: this._providerManagerInProgress,
            storageInProgress: this._storageInProgress,
            hasProviderManager: !!this._providerManager,
            hasStorage: !!this._storage,
            shutdownComplete: this._shutdownComplete,
            currentProviderInShutdown: this._currentProviderInShutdown,
            currentProviderInInit: this._currentProviderInInit,
            currentProviderInCollect: this._currentProviderInCollect,
          }));

      try {
        this._storageInProgress = true;
        TelemetryStopwatch.start(this._dbOpenHistogram, this);
        let storage = yield Metrics.Storage(this._dbName);
        TelemetryStopwatch.finish(this._dbOpenHistogram, this);
        yield this._onStorageCreated();

        delete this._dbOpenHistogram;
        this._log.info("Storage initialized.");
        this._storage = storage;
        this._storageInProgress = false;

        if (this._shutdownRequested) {
          this._initiateShutdown();
          return null;
        }

        yield this._initializeProviderManager();
        yield this._onProviderManagerInitialized();
        this._initializedDeferred.resolve();
        return this.onInit();
      } catch (ex) {
        yield this._onInitError(ex);
        this._initializedDeferred.reject(ex);
      }
    }.bind(this));
  },

  
  
  
  
  

  _onInitError: function (error) {
    TelemetryStopwatch.cancel(this._initHistogram, this);
    TelemetryStopwatch.cancel(this._dbOpenHistogram, this);
    delete this._initHistogram;
    delete this._dbOpenHistogram;

    this._recordError("Error during initialization", error);
    this._initializeHadError = true;
    this._initiateShutdown();
    return Promise.reject(error);

    
    
  },


  




  _deleteOldLastPayload: function () {
    let paths = [this._state._lastPayloadPath, this._state._lastPayloadPath + ".tmp"];
    return Task.spawn(function removeAllFiles () {
      for (let path of paths) {
        try {
          OS.File.remove(path);
        } catch (ex) {
          if (!ex.becauseNoSuchFile) {
            this._log.error("Exception when removing outdated payload files: " +
                            CommonUtils.exceptionStr(ex));
          }
        }
      }
    }.bind(this));
  },

  _initializeProviderManager: Task.async(function* _initializeProviderManager() {
    if (this._collector) {
      throw new Error("Provider manager has already been initialized.");
    }

    this._log.info("Initializing provider manager.");
    this._providerManager = new Metrics.ProviderManager(this._storage);
    this._providerManager.onProviderError = this._recordError.bind(this);
    this._providerManager.onProviderInit = this._initProvider.bind(this);
    this._providerManagerInProgress = true;

    let catString = this._prefs.get("service.providerCategories") || "";
    if (catString.length) {
      for (let category of catString.split(",")) {
        yield this._providerManager.registerProvidersFromCategoryManager(category,
                     providerName => this._currentProviderInInit = providerName);
      }
      this._currentProviderInInit = null;
    }
  }),

  _onProviderManagerInitialized: function () {
    TelemetryStopwatch.finish(this._initHistogram, this);
    delete this._initHistogram;
    this._log.debug("Provider manager initialized.");
    this._providerManagerInProgress = false;

    if (this._shutdownRequested) {
      this._initiateShutdown();
      return;
    }

    this._log.info("HealthReporter started.");
    this._initialized = true;
    Services.obs.addObserver(this, "idle-daily", false);

    
    
    
    
    
    
    
    
    
    
    
    
    if (!this._policy.healthReportUploadEnabled) {
      this._log.info("Upload not enabled. Scheduling daily collection.");
      
      
      
      try {
        let timerName = this._branch.replace(/\./g, "-") + "lastDailyCollection";
        let tm = Cc["@mozilla.org/updates/timer-manager;1"]
                   .getService(Ci.nsIUpdateTimerManager);
        tm.registerTimer(timerName, this.collectMeasurements.bind(this),
                         24 * 60 * 60);
      } catch (ex) {
        this._log.error("Error registering collection timer: " +
                        CommonUtils.exceptionStr(ex));
      }
    }

    
    this._storage.compact();
  },

  
  observe: function (subject, topic, data) {
    switch (topic) {
      case "quit-application":
        Services.obs.removeObserver(this, "quit-application");
        this._initiateShutdown();
        break;

      case "idle-daily":
        this._performDailyMaintenance();
        break;
    }
  },

  _initiateShutdown: function () {
    
    if (this._shutdownInitiated) {
      this._log.warn("Shutdown has already been initiated. No-op.");
      return;
    }

    this._log.info("Request to shut down.");

    this._initialized = false;
    this._shutdownRequested = true;

    if (this._initializeHadError) {
      this._log.warn("Initialization had error. Shutting down immediately.");
    } else {
      if (this._providerManagerInProgress) {
        this._log.warn("Provider manager is in progress of initializing. " +
                       "Waiting to finish.");
        return;
      }

      
      
      
      if (this._storageInProgress) {
        this._log.warn("Storage is in progress of initializing. Waiting to finish.");
        return;
      }
    }

    this._log.warn("Initiating main shutdown procedure.");

    
    

    TelemetryStopwatch.start(TELEMETRY_SHUTDOWN, this);
    this._shutdownInitiated = true;

    
    
    try {
      Services.obs.removeObserver(this, "idle-daily");
    } catch (ex) { }

    Task.spawn(function*() {
      try {
        if (this._providerManager) {
          this._log.info("Shutting down provider manager.");
          for (let provider of this._providerManager.providers) {
            try {
              this._log.info("Shutting down provider: " + provider.name);
              this._currentProviderInShutdown = provider.name;
              yield provider.shutdown();
            } catch (ex) {
              this._log.warn("Error when shutting down provider: " +
                             CommonUtils.exceptionStr(ex));
            }
          }
          this._log.info("Provider manager shut down.");
          this._providerManager = null;
          this._currentProviderInShutdown = null;
          this._onProviderManagerShutdown();
        }
        if (this._storage) {
          this._log.info("Shutting down storage.");
          try {
            yield this._storage.close();
            yield this._onStorageClose();
          } catch (error) {
            this._log.warn("Error when closing storage: " +
                           CommonUtils.exceptionStr(error));
          }
          this._storage = null;
        }

        this._log.warn("Shutdown complete.");
        this._shutdownComplete = true;
      } finally {
        this._deferredShutdown.resolve();
        TelemetryStopwatch.finish(TELEMETRY_SHUTDOWN, this);
      }
    }.bind(this));
  },

  onInit: function() {
    return this._initializedDeferred.promise;
  },

  _onStorageCreated: function() {
    
    
  },

  _onStorageClose: function() {
    
    
  },

  _onProviderManagerShutdown: function() {
    
    
  },

  




  _shutdown: function () {
    this._initiateShutdown();
    return this._promiseShutdown;
  },

  _performDailyMaintenance: function () {
    this._log.info("Request to perform daily maintenance.");

    if (!this._initialized) {
      return;
    }

    let now = new Date();
    let cutoff = new Date(now.getTime() - MILLISECONDS_PER_DAY * (DAYS_IN_PAYLOAD - 1));

    
    this._storage.pruneDataBefore(cutoff);
  },

  
  
  

  






  getProvider: function (name) {
    if (!this._providerManager) {
      return null;
    }

    return this._providerManager.getProvider(name);
  },

  _initProvider: function (provider) {
    provider.healthReporter = this;
  },

  















  _recordError: function (message, ex) {
    let recordMessage = message;
    let logMessage = message;

    if (ex) {
      recordMessage += ": " + CommonUtils.exceptionStr(ex);
      logMessage += ": " + CommonUtils.exceptionStr(ex);
    }

    
    
    let appData = Services.dirsvc.get("UAppData", Ci.nsIFile);
    let profile = Services.dirsvc.get("ProfD", Ci.nsIFile);

    let appDataURI = Services.io.newFileURI(appData);
    let profileURI = Services.io.newFileURI(profile);

    
    
    
    

    
    function regexify(s) {
      return new RegExp(s.replace(/[-\\^$*+?.()|[\]{}]/g, "\\$&"), "g");
    }

    function replace(uri, path, thing) {
      
      try {
        recordMessage = recordMessage.replace(regexify(uri.spec), "<" + thing + "URI>");
      } catch (ex) { }

      recordMessage = recordMessage.replace(regexify(path), "<" + thing + "Path>");
    }

    if (appData.path.contains(profile.path)) {
      replace(appDataURI, appData.path, 'AppData');
      replace(profileURI, profile.path, 'Profile');
    } else {
      replace(profileURI, profile.path, 'Profile');
      replace(appDataURI, appData.path, 'AppData');
    }

    this._log.warn(logMessage);
    this._errors.push(recordMessage);
  },

  


  collectMeasurements: function () {
    if (!this._initialized) {
      return Promise.reject(new Error("Not initialized."));
    }

    return Task.spawn(function doCollection() {
      yield this._providerManager.ensurePullOnlyProvidersRegistered();

      try {
        TelemetryStopwatch.start(TELEMETRY_COLLECT_CONSTANT, this);
        yield this._providerManager.collectConstantData(name => this._currentProviderInCollect = name);
        this._currentProviderInCollect = null;
        TelemetryStopwatch.finish(TELEMETRY_COLLECT_CONSTANT, this);
      } catch (ex) {
        TelemetryStopwatch.cancel(TELEMETRY_COLLECT_CONSTANT, this);
        this._log.warn("Error collecting constant data: " +
                       CommonUtils.exceptionStr(ex));
      }

      
      
      
      
      
      
      if (!this._lastDailyDate ||
          Date.now() - this._lastDailyDate > MILLISECONDS_PER_DAY) {

        try {
          TelemetryStopwatch.start(TELEMETRY_COLLECT_DAILY, this);
          this._lastDailyDate = new Date();
          yield this._providerManager.collectDailyData(name => this._currentProviderInCollect = name);
          this._currentProviderInCollect = null;
          TelemetryStopwatch.finish(TELEMETRY_COLLECT_DAILY, this);
        } catch (ex) {
          TelemetryStopwatch.cancel(TELEMETRY_COLLECT_DAILY, this);
          this._log.warn("Error collecting daily data from providers: " +
                         CommonUtils.exceptionStr(ex));
        }
      }

      yield this._providerManager.ensurePullOnlyProvidersUnregistered();

      
      
      
      try {
        TelemetryStopwatch.start(TELEMETRY_COLLECT_CHECKPOINT, this);
        yield this._storage.checkpoint();
        TelemetryStopwatch.finish(TELEMETRY_COLLECT_CHECKPOINT, this);
      } catch (ex) {
        TelemetryStopwatch.cancel(TELEMETRY_COLLECT_CHECKPOINT, this);
        throw ex;
      }

      throw new Task.Result();
    }.bind(this));
  },

  











  collectAndObtainJSONPayload: function (asObject=false) {
    if (!this._initialized) {
      return Promise.reject(new Error("Not initialized."));
    }

    return Task.spawn(function collectAndObtain() {
      yield this._storage.setAutoCheckpoint(0);
      yield this._providerManager.ensurePullOnlyProvidersRegistered();

      let payload;
      let error;

      try {
        yield this.collectMeasurements();
        payload = yield this.getJSONPayload(asObject);
      } catch (ex) {
        error = ex;
        this._collectException("Error collecting and/or retrieving JSON payload",
                               ex);
      } finally {
        yield this._providerManager.ensurePullOnlyProvidersUnregistered();
        yield this._storage.setAutoCheckpoint(1);

        if (error) {
          throw error;
        }
      }

      
      
      throw new Task.Result(payload);
    }.bind(this));
  },


  













  getJSONPayload: function (asObject=false) {
    TelemetryStopwatch.start(TELEMETRY_GENERATE_PAYLOAD, this);
    let deferred = Promise.defer();

    Task.spawn(this._getJSONPayload.bind(this, this._now(), asObject)).then(
      function onResult(result) {
        TelemetryStopwatch.finish(TELEMETRY_GENERATE_PAYLOAD, this);
        deferred.resolve(result);
      }.bind(this),
      function onError(error) {
        TelemetryStopwatch.cancel(TELEMETRY_GENERATE_PAYLOAD, this);
        deferred.reject(error);
      }.bind(this)
    );

    return deferred.promise;
  },

  _getJSONPayload: function (now, asObject=false) {
    let pingDateString = this._formatDate(now);
    this._log.info("Producing JSON payload for " + pingDateString);

    
    if (this._providerManager) {
      yield this._providerManager.ensurePullOnlyProvidersRegistered();
    }

    let o = {
      version: 2,
      clientID: this._state.clientID,
      clientIDVersion: this._state.clientIDVersion,
      thisPingDate: pingDateString,
      geckoAppInfo: this.obtainAppInfo(this._log),
      data: {last: {}, days: {}},
    };

    let outputDataDays = o.data.days;

    
    let lastPingDate = this.lastPingDate;
    if (lastPingDate && lastPingDate.getTime() > 0) {
      o.lastPingDate = this._formatDate(lastPingDate);
    }

    
    
    if (this._initialized) {
      for (let provider of this._providerManager.providers) {
        let providerName = provider.name;

        let providerEntry = {
          measurements: {},
        };

        
        let lastVersions = {};
        
        let dayVersions = {};

        for (let [measurementKey, measurement] of provider.measurements) {
          let name = providerName + "." + measurement.name;
          let version = measurement.version;

          let serializer;
          try {
            
            
            serializer = measurement.serializer(measurement.SERIALIZE_JSON);
          } catch (ex) {
            this._recordError("Error obtaining serializer for measurement: " +
                              name, ex);
            continue;
          }

          let data;
          try {
            data = yield measurement.getValues();
          } catch (ex) {
            this._recordError("Error obtaining data for measurement: " + name,
                              ex);
            continue;
          }

          if (data.singular.size) {
            try {
              let serialized = serializer.singular(data.singular);
              if (serialized) {
                
                
                if (!(name in o.data.last) || version > lastVersions[name]) {
                  o.data.last[name] = serialized;
                  lastVersions[name] = version;
                }
              }
            } catch (ex) {
              this._recordError("Error serializing singular data: " + name,
                                ex);
              continue;
            }
          }

          let dataDays = data.days;
          for (let i = 0; i < DAYS_IN_PAYLOAD; i++) {
            let date = new Date(now.getTime() - i * MILLISECONDS_PER_DAY);
            if (!dataDays.hasDay(date)) {
              continue;
            }
            let dateFormatted = this._formatDate(date);

            try {
              let serialized = serializer.daily(dataDays.getDay(date));
              if (!serialized) {
                continue;
              }

              if (!(dateFormatted in outputDataDays)) {
                outputDataDays[dateFormatted] = {};
              }

              
              
              
              if (!(dateFormatted in dayVersions)) {
                dayVersions[dateFormatted] = {};
              }

              if (!(name in outputDataDays[dateFormatted]) ||
                  version > dayVersions[dateFormatted][name]) {
                outputDataDays[dateFormatted][name] = serialized;
                dayVersions[dateFormatted][name] = version;
              }
            } catch (ex) {
              this._recordError("Error populating data for day: " + name, ex);
              continue;
            }
          }
        }
      }
    } else {
      o.notInitialized = 1;
      this._log.warn("Not initialized. Sending report with only error info.");
    }

    if (this._errors.length) {
      o.errors = this._errors.slice(0, 20);
    }

    if (this._initialized) {
      this._storage.compact();
    }

    if (!asObject) {
      TelemetryStopwatch.start(TELEMETRY_JSON_PAYLOAD_SERIALIZE, this);
      o = JSON.stringify(o);
      TelemetryStopwatch.finish(TELEMETRY_JSON_PAYLOAD_SERIALIZE, this);
    }

    if (this._providerManager) {
      yield this._providerManager.ensurePullOnlyProvidersUnregistered();
    }

    throw new Task.Result(o);
  },

  _now: function _now() {
    return new Date();
  },

  
  appInfoVersion: 1,
  appInfoFields: {
    
    vendor: "vendor",
    name: "name",
    id: "ID",
    version: "version",
    appBuildID: "appBuildID",
    platformVersion: "platformVersion",
    platformBuildID: "platformBuildID",

    
    os: "OS",
    xpcomabi: "XPCOMABI",
  },

  






  obtainAppInfo: function () {
    let out = {"_v": this.appInfoVersion};
    try {
      let ai = Services.appinfo;
      for (let [k, v] in Iterator(this.appInfoFields)) {
        out[k] = ai[v];
      }
    } catch (ex) {
      this._log.warn("Could not obtain Services.appinfo: " +
                     CommonUtils.exceptionStr(ex));
    }

    try {
      out["updateChannel"] = UpdateChannel.get();
    } catch (ex) {
      this._log.warn("Could not obtain update channel: " +
                     CommonUtils.exceptionStr(ex));
    }

    return out;
  },
});






























































this.HealthReporter = function (branch, policy, stateLeaf=null) {
  this._stateLeaf = stateLeaf;
  this._uploadInProgress = false;

  AbstractHealthReporter.call(this, branch, policy, TelemetryController.getSessionRecorder());

  if (!this.serverURI) {
    throw new Error("No server URI defined. Did you forget to define the pref?");
  }

  if (!this.serverNamespace) {
    throw new Error("No server namespace defined. Did you forget a pref?");
  }

  this._state = new HealthReporterState(this);
}

this.HealthReporter.prototype = Object.freeze({
  __proto__: AbstractHealthReporter.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  get lastSubmitID() {
    return this._state.lastSubmitID;
  },

  






  get lastPingDate() {
    return this._state.lastPingDate;
  },

  





  get serverURI() {
    return this._prefs.get("documentServerURI", null);
  },

  set serverURI(value) {
    if (!value) {
      throw new Error("serverURI must have a value.");
    }

    if (typeof(value) != "string") {
      throw new Error("serverURI must be a string: " + value);
    }

    this._prefs.set("documentServerURI", value);
  },

  


  get serverNamespace() {
    return this._prefs.get("documentServerNamespace", "metrics");
  },

  set serverNamespace(value) {
    if (!value) {
      throw new Error("serverNamespace must have a value.");
    }

    if (typeof(value) != "string") {
      throw new Error("serverNamespace must be a string: " + value);
    }

    this._prefs.set("documentServerNamespace", value);
  },

  


  get willUploadData() {
    return  this._policy.userNotifiedOfCurrentPolicy &&
            this._policy.healthReportUploadEnabled;
  },

  




  haveRemoteData: function () {
    return !!this._state.lastSubmitID;
  },

  




  requestDataUpload: function (request) {
    if (!this._initialized) {
      return Promise.reject(new Error("Not initialized."));
    }

    return Task.spawn(function doUpload() {
      yield this._providerManager.ensurePullOnlyProvidersRegistered();
      try {
        yield this.collectMeasurements();
        try {
          yield this._uploadData(request);
        } catch (ex) {
          this._onSubmitDataRequestFailure(ex);
        }
      } finally {
        yield this._providerManager.ensurePullOnlyProvidersUnregistered();
      }
    }.bind(this));
  },

  







  requestDeleteRemoteData: function (reason) {
    if (!this.haveRemoteData()) {
      return;
    }

    return this._policy.deleteRemoteData(reason);
  },

  


  _onInitError: function (error) {
    
    
    let inShutdown = this._shutdownRequested;
    let result;

    try {
      result = AbstractHealthReporter.prototype._onInitError.call(this, error);
    } catch (ex) {
      this._log.error("Error when calling _onInitError: " +
                      CommonUtils.exceptionStr(ex));
    }

    
    
    
    
    if (!inShutdown &&
        this._policy.healthReportUploadEnabled &&
        this._policy.ensureUserNotified()) {
      
      
      let request = {
        onNoDataAvailable: function () {},
        onSubmissionSuccess: function () {},
        onSubmissionFailureSoft: function () {},
        onSubmissionFailureHard: function () {},
        onUploadInProgress: function () {},
      };

      this._uploadData(request);
    }

    return result;
  },

  _onBagheeraResult: function (request, isDelete, date, result) {
    this._log.debug("Received Bagheera result.");

    return Task.spawn(function onBagheeraResult() {
      let hrProvider = this.getProvider("org.mozilla.healthreport");

      if (!result.transportSuccess) {
        
        
        if (hrProvider && !isDelete) {
          try {
            hrProvider.recordEvent("uploadTransportFailure", date);
          } catch (ex) {
            this._log.error("Error recording upload transport failure: " +
                            CommonUtils.exceptionStr(ex));
          }
        }

        request.onSubmissionFailureSoft("Network transport error.");
        throw new Task.Result(false);
      }

      if (!result.serverSuccess) {
        if (hrProvider && !isDelete) {
          try {
            hrProvider.recordEvent("uploadServerFailure", date);
          } catch (ex) {
            this._log.error("Error recording server failure: " +
                            CommonUtils.exceptionStr(ex));
          }
        }

        request.onSubmissionFailureHard("Server failure.");
        throw new Task.Result(false);
      }

      if (hrProvider && !isDelete) {
        try {
          hrProvider.recordEvent("uploadSuccess", date);
        } catch (ex) {
          this._log.error("Error recording upload success: " +
                          CommonUtils.exceptionStr(ex));
        }
      }

      if (isDelete) {
        this._log.warn("Marking delete as successful.");
        yield this._state.removeRemoteIDs([result.id]);
      } else {
        this._log.warn("Marking upload as successful.");
        yield this._state.updateLastPingAndRemoveRemoteIDs(date, result.deleteIDs);
      }

      request.onSubmissionSuccess(this._now());

      throw new Task.Result(true);
    }.bind(this));
  },

  _onSubmitDataRequestFailure: function (error) {
    this._log.error("Error processing request to submit data: " +
                    CommonUtils.exceptionStr(error));
  },

  _formatDate: function (date) {
    
    return date.toISOString().substr(0, 10);
  },

  _uploadData: function (request) {
    
    
    
    
    if (this._uploadInProgress) {
      this._log.warn("Upload requested but upload already in progress.");
      let provider = this.getProvider("org.mozilla.healthreport");
      let promise = provider.recordEvent("uploadAlreadyInProgress");
      request.onUploadInProgress("Upload already in progress.");
      return promise;
    }

    let id = CommonUtils.generateUUID();

    this._log.info("Uploading data to server: " + this.serverURI + " " +
                   this.serverNamespace + ":" + id);
    let client = new BagheeraClient(this.serverURI);
    let now = this._now();

    return Task.spawn(function doUpload() {
      try {
        
        
        
        this._uploadInProgress = true;
        let payload = yield this.getJSONPayload();

        let histogram = Services.telemetry.getHistogramById(TELEMETRY_PAYLOAD_SIZE_UNCOMPRESSED);
        histogram.add(payload.length);

        let lastID = this.lastSubmitID;
        yield this._state.addRemoteID(id);

        let hrProvider = this.getProvider("org.mozilla.healthreport");
        if (hrProvider) {
          let event = lastID ? "continuationUploadAttempt"
                             : "firstDocumentUploadAttempt";
          try {
            hrProvider.recordEvent(event, now);
          } catch (ex) {
            this._log.error("Error when recording upload attempt: " +
                            CommonUtils.exceptionStr(ex));
          }
        }

        TelemetryStopwatch.start(TELEMETRY_UPLOAD, this);
        let result;
        try {
          let options = {
            deleteIDs: this._state.remoteIDs.filter((x) => { return x != id; }),
            telemetryCompressed: TELEMETRY_PAYLOAD_SIZE_COMPRESSED,
          };
          result = yield client.uploadJSON(this.serverNamespace, id, payload,
                                           options);
          TelemetryStopwatch.finish(TELEMETRY_UPLOAD, this);
        } catch (ex) {
          TelemetryStopwatch.cancel(TELEMETRY_UPLOAD, this);
          if (hrProvider) {
            try {
              hrProvider.recordEvent("uploadClientFailure", now);
            } catch (ex) {
              this._log.error("Error when recording client failure: " +
                              CommonUtils.exceptionStr(ex));
            }
          }
          throw ex;
        }

        yield this._onBagheeraResult(request, false, now, result);
      } finally {
        this._uploadInProgress = false;
      }
    }.bind(this));
  },

  





  deleteRemoteData: function (request) {
    if (!this._state.lastSubmitID) {
      this._log.info("Received request to delete remote data but no data stored.");
      request.onNoDataAvailable();
      return;
    }

    this._log.warn("Deleting remote data.");
    let client = new BagheeraClient(this.serverURI);

    return Task.spawn(function* doDelete() {
      try {
        let result = yield client.deleteDocument(this.serverNamespace,
                                                 this.lastSubmitID);
        yield this._onBagheeraResult(request, true, this._now(), result);
      } catch (ex) {
        this._log.error("Error processing request to delete data: " +
                        CommonUtils.exceptionStr(error));
      } finally {
        
        
        
        if (!this.haveRemoteData()) {
          yield this._state.resetClientID();
        }
      }
    }.bind(this));
  },
});

