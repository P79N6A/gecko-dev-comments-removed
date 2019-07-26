



"use strict";

#ifndef MERGED_COMPARTMENT

this.EXPORTED_SYMBOLS = ["HealthReporter"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

Cu.import("resource://gre/modules/Metrics.jsm");
Cu.import("resource://services-common/bagheeraclient.js");
Cu.import("resource://services-common/async.js");
#endif

Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");




const OLDEST_ALLOWED_YEAR = 2012;

const DAYS_IN_PAYLOAD = 180;

const DEFAULT_DATABASE_NAME = "healthreport.sqlite";























































function HealthReporter(branch, policy, sessionRecorder) {
  if (!branch.endsWith(".")) {
    throw new Error("Branch must end with a period (.): " + branch);
  }

  if (!policy) {
    throw new Error("Must provide policy to HealthReporter constructor.");
  }

  this._log = Log4Moz.repository.getLogger("Services.HealthReport.HealthReporter");
  this._log.info("Initializing health reporter instance against " + branch);

  this._branch = branch;
  this._prefs = new Preferences(branch);

  if (!this.serverURI) {
    throw new Error("No server URI defined. Did you forget to define the pref?");
  }

  if (!this.serverNamespace) {
    throw new Error("No server namespace defined. Did you forget a pref?");
  }

  this._policy = policy;
  this.sessionRecorder = sessionRecorder;

  this._dbName = this._prefs.get("dbName") || DEFAULT_DATABASE_NAME;

  this._storage = null;
  this._storageInProgress = false;
  this._collector = null;
  this._collectorInProgress = false;
  this._initialized = false;
  this._initializeHadError = false;
  this._initializedDeferred = Promise.defer();
  this._shutdownRequested = false;
  this._shutdownInitiated = false;
  this._shutdownComplete = false;
  this._shutdownCompleteCallback = null;

  this._ensureDirectoryExists(this._stateDir)
      .then(this._onStateDirCreated.bind(this),
            this._onInitError.bind(this));

}

HealthReporter.prototype = Object.freeze({
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  




  get initialized() {
    return this._initialized;
  },

  






  get lastPingDate() {
    return CommonUtils.getDatePref(this._prefs, "lastPingTime", 0, this._log,
                                   OLDEST_ALLOWED_YEAR);
  },

  set lastPingDate(value) {
    CommonUtils.setDatePref(this._prefs, "lastPingTime", value,
                            OLDEST_ALLOWED_YEAR);
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

  








  get lastSubmitID() {
    return this._prefs.get("lastSubmitID", null) || null;
  },

  set lastSubmitID(value) {
    this._prefs.set("lastSubmitID", value || "");
  },

  


  get willUploadData() {
    return this._policy.dataSubmissionPolicyAccepted &&
           this._policy.healthReportUploadEnabled;
  },

  




  haveRemoteData: function () {
    return !!this.lastSubmitID;
  },

  
  
  
  
  

  _onInitError: function (error) {
    this._log.error("Error during initialization: " +
                    CommonUtils.exceptionStr(error));
    this._initializeHadError = true;
    this._initiateShutdown();
    this._initializedDeferred.reject(error);

    
    
  },

  _onStateDirCreated: function () {
    
    
    Services.obs.addObserver(this, "quit-application", false);
    Services.obs.addObserver(this, "profile-before-change", false);

    this._storageInProgress = true;
    Metrics.Storage(this._dbName).then(this._onStorageCreated.bind(this),
                                       this._onInitError.bind(this));
  },

  
  _onStorageCreated: function (storage) {
    this._log.info("Storage initialized.");
    this._storage = storage;
    this._storageInProgress = false;

    if (this._shutdownRequested) {
      this._initiateShutdown();
      return;
    }

    Task.spawn(this._initializeCollector.bind(this))
        .then(this._onCollectorInitialized.bind(this),
              this._onInitError.bind(this));
  },

  _initializeCollector: function () {
    if (this._collector) {
      throw new Error("Collector has already been initialized.");
    }

    this._log.info("Initializing collector.");
    this._collector = new Metrics.Collector(this._storage);
    this._collectorInProgress = true;

    let catString = this._prefs.get("service.providerCategories") || "";
    if (catString.length) {
      for (let category of catString.split(",")) {
        yield this.registerProvidersFromCategoryManager(category);
      }
    }
  },

  _onCollectorInitialized: function () {
    this._log.debug("Collector initialized.");
    this._collectorInProgress = false;

    if (this._shutdownRequested) {
      this._initiateShutdown();
      return;
    }

    this._log.info("HealthReporter started.");
    this._initialized = true;
    Services.obs.addObserver(this, "idle-daily", false);

    
    this._storage.compact();
    this._initializedDeferred.resolve(this);
  },

  
  observe: function (subject, topic, data) {
    switch (topic) {
      case "quit-application":
        Services.obs.removeObserver(this, "quit-application");
        this._initiateShutdown();
        break;

      case "profile-before-change":
        Services.obs.removeObserver(this, "profile-before-change");
        this._waitForShutdown();
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

    if (this._collectorInProgress) {
      this._log.warn("Collector is in progress of initializing. Waiting to finish.");
      return;
    }

    
    
    
    if (this._storageInProgress) {
      this._log.warn("Storage is in progress of initializing. Waiting to finish.");
      return;
    }

    this._log.warn("Initiating main shutdown procedure.");

    
    
    this._shutdownInitiated = true;

    
    
    try {
      Services.obs.removeObserver(this, "idle-daily");
    } catch (ex) { }

    
    if (this._collector) {
      let onShutdown = this._onCollectorShutdown.bind(this);
      Task.spawn(this._shutdownCollector.bind(this))
          .then(onShutdown, onShutdown);
      return;
    }

    this._log.warn("Don't have collector. Proceeding to storage shutdown.");
    this._shutdownStorage();
  },

  _shutdownCollector: function () {
    this._log.info("Shutting down collector.");
    for (let provider of this._collector.providers) {
      try {
        yield provider.shutdown();
      } catch (ex) {
        this._log.warn("Error when shutting down provider: " +
                       CommonUtils.exceptionStr(ex));
      }
    }
  },

  _onCollectorShutdown: function () {
    this._log.info("Collector shut down.");
    this._collector = null;
    this._shutdownStorage();
  },

  _shutdownStorage: function () {
    if (!this._storage) {
      this._onShutdownComplete();
    }

    this._log.info("Shutting down storage.");
    let onClose = this._onStorageClose.bind(this);
    this._storage.close().then(onClose, onClose);
  },

  _onStorageClose: function (error) {
    this._log.info("Storage has been closed.");

    if (error) {
      this._log.warn("Error when closing storage: " +
                     CommonUtils.exceptionStr(error));
    }

    this._storage = null;
    this._onShutdownComplete();
  },

  _onShutdownComplete: function () {
    this._log.warn("Shutdown complete.");
    this._shutdownComplete = true;

    if (this._shutdownCompleteCallback) {
      this._shutdownCompleteCallback();
    }
  },

  _waitForShutdown: function () {
    if (this._shutdownComplete) {
      return;
    }

    this._shutdownCompleteCallback = Async.makeSpinningCallback();
    this._shutdownCompleteCallback.wait();
    this._shutdownCompleteCallback = null;
  },

  




  _shutdown: function () {
    this._initiateShutdown();
    this._waitForShutdown();
  },

  


  onInit: function () {
    if (this._initializeHadError) {
      throw new Error("Service failed to initialize.");
    }

    if (this._initialized) {
      return Promise.resolve(this);
    }

    return this._initializedDeferred.promise;
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

  
  
  

  








  registerProvider: function (provider) {
    return this._collector.registerProvider(provider);
  },

  



























  registerProvidersFromCategoryManager: function (category) {
    this._log.info("Registering providers from category: " + category);
    let cm = Cc["@mozilla.org/categorymanager;1"]
               .getService(Ci.nsICategoryManager);

    let promises = [];
    let enumerator = cm.enumerateCategory(category);
    while (enumerator.hasMoreElements()) {
      let entry = enumerator.getNext()
                            .QueryInterface(Ci.nsISupportsCString)
                            .toString();

      let uri = cm.getCategoryEntry(category, entry);
      this._log.info("Attempting to load provider from category manager: " +
                     entry + " from " + uri);

      try {
        let ns = {};
        Cu.import(uri, ns);

        let provider = new ns[entry]();
        provider.initPreferences(this._branch + "provider.");
        provider.healthReporter = this;
        promises.push(this.registerProvider(provider));
      } catch (ex) {
        this._log.warn("Error registering provider from category manager: " +
                       entry + "; " + CommonUtils.exceptionStr(ex));
        continue;
      }
    }

    return Task.spawn(function wait() {
      for (let promise of promises) {
        yield promise;
      }
    });
  },

  


  collectMeasurements: function () {
    return this._collector.collectConstantData();
  },

  




  requestDataUpload: function (request) {
    this.collectMeasurements()
        .then(this._uploadData.bind(this, request),
              this._onSubmitDataRequestFailure.bind(this));
  },

  







  requestDeleteRemoteData: function (reason) {
    if (!this.lastSubmitID) {
      return;
    }

    return this._policy.deleteRemoteData(reason);
  },

  getJSONPayload: function () {
    return Task.spawn(this._getJSONPayload.bind(this, this._now()));
  },

  _getJSONPayload: function (now) {
    let pingDateString = this._formatDate(now);
    this._log.info("Producing JSON payload for " + pingDateString);

    let o = {
      version: 1,
      thisPingDate: pingDateString,
      data: {last: {}, days: {}},
    };

    let outputDataDays = o.data.days;

    
    
    
    let errors = [];

    let lastPingDate = this.lastPingDate;
    if (lastPingDate.getTime() > 0) {
      o.lastPingDate = this._formatDate(lastPingDate);
    }

    for (let provider of this._collector.providers) {
      let providerName = provider.name;

      let providerEntry = {
        measurements: {},
      };

      for (let [measurementKey, measurement] of provider.measurements) {
        let name = providerName + "." + measurement.name;

        let serializer;
        try {
          
          
          serializer = measurement.serializer(measurement.SERIALIZE_JSON);
        } catch (ex) {
          this._log.warn("Error obtaining serializer for measurement: " + name +
                         ": " + CommonUtils.exceptionStr(ex));
          errors.push("Could not obtain serializer: " + name);
          continue;
        }

        let data;
        try {
          data = yield measurement.getValues();
        } catch (ex) {
          this._log.warn("Error obtaining data for measurement: " +
                         name + ": " + CommonUtils.exceptionStr(ex));
          errors.push("Could not obtain data: " + name);
          continue;
        }

        if (data.singular.size) {
          try {
            o.data.last[name] = serializer.singular(data.singular);
          } catch (ex) {
            this._log.warn("Error serializing data: " + CommonUtils.exceptionStr(ex));
            errors.push("Error serializing singular: " + name);
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

            outputDataDays[dateFormatted][name] = serialized;
          } catch (ex) {
            this._log.warn("Error populating data for day: " +
                           CommonUtils.exceptionStr(ex));
            errors.push("Could not serialize day: " + name +
                        " ( " + dateFormatted + ")");
            continue;
          }
        }
      }
    }

    if (errors.length) {
      o.errors = errors;
    }

    this._storage.compact();
    throw new Task.Result(JSON.stringify(o));
  },

  _onBagheeraResult: function (request, isDelete, result) {
    this._log.debug("Received Bagheera result.");

    let promise = Promise.resolve(null);

    if (!result.transportSuccess) {
      request.onSubmissionFailureSoft("Network transport error.");
      return promise;
    }

    if (!result.serverSuccess) {
      request.onSubmissionFailureHard("Server failure.");
      return promise;
    }

    let now = this._now();

    if (isDelete) {
      this.lastSubmitID = null;
    } else {
      this.lastSubmitID = result.id;
      this.lastPingDate = now;
    }

    request.onSubmissionSuccess(now);

    return promise;
  },

  _onSubmitDataRequestFailure: function (error) {
    this._log.error("Error processing request to submit data: " +
                    CommonUtils.exceptionStr(error));
  },

  _formatDate: function (date) {
    
    return date.toISOString().substr(0, 10);
  },


  _uploadData: function (request) {
    let id = CommonUtils.generateUUID();

    this._log.info("Uploading data to server: " + this.serverURI + " " +
                   this.serverNamespace + ":" + id);
    let client = new BagheeraClient(this.serverURI);

    return Task.spawn(function doUpload() {
      let payload = yield this.getJSONPayload();
      yield this._saveLastPayload(payload);
      let result = yield client.uploadJSON(this.serverNamespace, id, payload,
                                           this.lastSubmitID);
      yield this._onBagheeraResult(request, false, result);
    }.bind(this));
  },

  deleteRemoteData: function (request) {
    if (!this.lastSubmitID) {
      this._log.info("Received request to delete remote data but no data stored.");
      request.onNoDataAvailable();
      return;
    }

    this._log.warn("Deleting remote data.");
    let client = new BagheeraClient(this.serverURI);

    return client.deleteDocument(this.serverNamespace, this.lastSubmitID)
                 .then(this._onBagheeraResult.bind(this, request, true),
                       this._onSubmitDataRequestFailure.bind(this));

  },

  get _stateDir() {
    let profD = OS.Constants.Path.profileDir;

    
    if (!profD || !profD.length) {
      throw new Error("Could not obtain profile directory. OS.File not " +
                      "initialized properly?");
    }

    return OS.Path.join(profD, "healthreport");
  },

  _ensureDirectoryExists: function (path) {
    let deferred = Promise.defer();

    OS.File.makeDir(path).then(
      function onResult() {
        deferred.resolve(true);
      },
      function onError(error) {
        if (error.becauseExists) {
          deferred.resolve(true);
          return;
        }

        deferred.reject(error);
      }
    );

    return deferred.promise;
  },

  get _lastPayloadPath() {
    return OS.Path.join(this._stateDir, "lastpayload.json");
  },

  _saveLastPayload: function (payload) {
    let path = this._lastPayloadPath;
    let pathTmp = path + ".tmp";

    let encoder = new TextEncoder();
    let buffer = encoder.encode(payload);

    return OS.File.writeAtomic(path, buffer, {tmpPath: pathTmp});
  },

  








  getLastPayload: function () {
    let path = this._lastPayloadPath;

    return OS.File.read(path).then(
      function onData(buffer) {
        let decoder = new TextDecoder();
        let json = JSON.parse(decoder.decode(buffer));

        return Promise.resolve(json);
      },
      function onError(error) {
        return Promise.reject(error);
      }
    );
  },

  _now: function _now() {
    return new Date();
  },

});

