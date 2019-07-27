



"use strict";

this.EXPORTED_SYMBOLS = [
  "Experiments",
  "ExperimentsProvider",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManagerPrivate",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetrySession",
                                  "resource://gre/modules/TelemetrySession.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "Metrics",
                                  "resource://gre/modules/Metrics.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gCrashReporter",
                                   "@mozilla.org/xre/app-info;1",
                                   "nsICrashReporter");

const FILE_CACHE                = "experiments.json";
const EXPERIMENTS_CHANGED_TOPIC = "experiments-changed";
const MANIFEST_VERSION          = 1;
const CACHE_VERSION             = 1;

const KEEP_HISTORY_N_DAYS       = 180;
const MIN_EXPERIMENT_ACTIVE_SECONDS = 60;

const PREF_BRANCH               = "experiments.";
const PREF_ENABLED              = "enabled"; 
const PREF_ACTIVE_EXPERIMENT    = "activeExperiment"; 
const PREF_LOGGING              = "logging";
const PREF_LOGGING_LEVEL        = PREF_LOGGING + ".level"; 
const PREF_LOGGING_DUMP         = PREF_LOGGING + ".dump"; 
const PREF_MANIFEST_URI         = "manifest.uri"; 
const PREF_FORCE_SAMPLE         = "force-sample-value"; 

const PREF_HEALTHREPORT_ENABLED = "datareporting.healthreport.service.enabled";

const PREF_BRANCH_TELEMETRY     = "toolkit.telemetry.";
const PREF_TELEMETRY_ENABLED    = "enabled";

const URI_EXTENSION_STRINGS     = "chrome://mozapps/locale/extensions/extensions.properties";
const STRING_TYPE_NAME          = "type.%ID%.name";

const CACHE_WRITE_RETRY_DELAY_SEC = 60 * 3;
const MANIFEST_FETCH_TIMEOUT_MSEC = 60 * 3 * 1000; 

const TELEMETRY_LOG = {
  
  ACTIVATION_KEY: "EXPERIMENT_ACTIVATION",
  ACTIVATION: {
    
    ACTIVATED: "ACTIVATED",
    
    INSTALL_FAILURE: "INSTALL_FAILURE",
    
    
    REJECTED: "REJECTED",
  },

  
  TERMINATION_KEY: "EXPERIMENT_TERMINATION",
  TERMINATION: {
    
    SERVICE_DISABLED: "SERVICE_DISABLED",
    
    ADDON_UNINSTALLED: "ADDON_UNINSTALLED",
    
    FROM_API: "FROM_API",
    
    EXPIRED: "EXPIRED",
    
    
    RECHECK: "RECHECK",
  },
};

const gPrefs = new Preferences(PREF_BRANCH);
const gPrefsTelemetry = new Preferences(PREF_BRANCH_TELEMETRY);
let gExperimentsEnabled = false;
let gAddonProvider = null;
let gExperiments = null;
let gLogAppenderDump = null;
let gPolicyCounter = 0;
let gExperimentsCounter = 0;
let gExperimentEntryCounter = 0;
let gPreviousProviderCounter = 0;



let gActiveInstallURLs = new Set();



let gActiveUninstallAddonIDs = new Set();

let gLogger;
let gLogDumping = false;

function configureLogging() {
  if (!gLogger) {
    gLogger = Log.repository.getLogger("Browser.Experiments");
    gLogger.addAppender(new Log.ConsoleAppender(new Log.BasicFormatter()));
  }
  gLogger.level = gPrefs.get(PREF_LOGGING_LEVEL, Log.Level.Warn);

  let logDumping = gPrefs.get(PREF_LOGGING_DUMP, false);
  if (logDumping != gLogDumping) {
    if (logDumping) {
      gLogAppenderDump = new Log.DumpAppender(new Log.BasicFormatter());
      gLogger.addAppender(gLogAppenderDump);
    } else {
      gLogger.removeAppender(gLogAppenderDump);
      gLogAppenderDump = null;
    }
    gLogDumping = logDumping;
  }
}



function allResolvedOrRejected(promises) {
  if (!promises.length) {
    return Promise.resolve([]);
  }

  let countdown = promises.length;
  let deferred = Promise.defer();

  for (let p of promises) {
    let helper = () => {
      if (--countdown == 0) {
        deferred.resolve();
      }
    };
    Promise.resolve(p).then(helper, helper);
  }

  return deferred.promise;
}






function loadJSONAsync(file, options) {
  return Task.spawn(function() {
    let rawData = yield OS.File.read(file, options);
    
    let data;
    try {
      
      let converter = new TextDecoder();
      data = JSON.parse(converter.decode(rawData));
    } catch (ex) {
      gLogger.error("Experiments: Could not parse JSON: " + file + " " + ex);
      throw ex;
    }
    throw new Task.Result(data);
  });
}

function telemetryEnabled() {
  return gPrefsTelemetry.get(PREF_TELEMETRY_ENABLED, false);
}


function addonInstallForURL(url, hash) {
  let deferred = Promise.defer();
  AddonManager.getInstallForURL(url, install => deferred.resolve(install),
                                "application/x-xpinstall", hash);
  return deferred.promise;
}



function installedExperimentAddons() {
  let deferred = Promise.defer();
  AddonManager.getAddonsByTypes(["experiment"], (addons) => {
    deferred.resolve([a for (a of addons) if (!a.appDisabled)]);
  });
  return deferred.promise;
}



function uninstallAddons(addons) {
  let ids = new Set([a.id for (a of addons)]);
  let deferred = Promise.defer();

  let listener = {};
  listener.onUninstalled = addon => {
    if (!ids.has(addon.id)) {
      return;
    }

    ids.delete(addon.id);
    if (ids.size == 0) {
      AddonManager.removeAddonListener(listener);
      deferred.resolve();
    }
  };

  AddonManager.addAddonListener(listener);

  for (let addon of addons) {
    
    
    
    addon.userDisabled = true;
    addon.uninstall();
  }

  return deferred.promise;
}





let Experiments = {
  


  instance: function () {
    if (!gExperiments) {
      gExperiments = new Experiments.Experiments();
    }

    return gExperiments;
  },
};






Experiments.Policy = function () {
  this._log = Log.repository.getLoggerWithMessagePrefix(
    "Browser.Experiments.Policy",
    "Policy #" + gPolicyCounter++ + "::");

  
  
  this.ignoreHashes = false;
};

Experiments.Policy.prototype = {
  now: function () {
    return new Date();
  },

  random: function () {
    let pref = gPrefs.get(PREF_FORCE_SAMPLE);
    if (pref !== undefined) {
      let val = Number.parseFloat(pref);
      this._log.debug("random sample forced: " + val);
      if (isNaN(val) || val < 0) {
        return 0;
      }
      if (val > 1) {
        return 1;
      }
      return val;
    }
    return Math.random();
  },

  futureDate: function (offset) {
    return new Date(this.now().getTime() + offset);
  },

  oneshotTimer: function (callback, timeout, thisObj, name) {
    return CommonUtils.namedTimer(callback, timeout, thisObj, name);
  },

  updatechannel: function () {
    return UpdateChannel.get();
  },

  locale: function () {
    let chrome = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry);
    return chrome.getSelectedLocale("global");
  },

  


  healthReportPayload: function () {
    return Task.spawn(function*() {
      let reporter = Cc["@mozilla.org/datareporting/service;1"]
            .getService(Ci.nsISupports)
            .wrappedJSObject
            .healthReporter;
      yield reporter.onInit();
      let payload = yield reporter.collectAndObtainJSONPayload();
      return payload;
    });
  },

  telemetryPayload: function () {
    return TelemetrySession.getPayload();
  },

  



  delayCacheWrite: function(promise) {
    return promise;
  },
};

function AlreadyShutdownError(message="already shut down") {
  Error.call(this, message);
  let error = new Error();
  this.name = "AlreadyShutdownError";
  this.message = message;
  this.stack = error.stack;
}
AlreadyShutdownError.prototype = Object.create(Error.prototype);
AlreadyShutdownError.prototype.constructor = AlreadyShutdownError;

function CacheWriteError(message="Error writing cache file") {
  Error.call(this, message);
  let error = new Error();
  this.name = "CacheWriteError";
  this.message = message;
  this.stack = error.stack;
}
CacheWriteError.prototype = Object.create(Error.prototype);
CacheWriteError.prototype.constructor = CacheWriteError;





Experiments.Experiments = function (policy=new Experiments.Policy()) {
  let log = Log.repository.getLoggerWithMessagePrefix(
      "Browser.Experiments.Experiments",
      "Experiments #" + gExperimentsCounter++ + "::");

  
  
  
  this._forensicsLogs = [];
  this._forensicsLogs.length = 20;
  this._log = Object.create(log);
  this._log.log = (level, string, params) => {
    this._forensicsLogs.shift();
    this._forensicsLogs.push(level + ": " + string);
    log.log(level, string, params);
  };

  this._log.trace("constructor");

  
  this._latestError = null;


  this._policy = policy;

  
  
  
  
  
  this._experiments = null;
  this._refresh = false;
  this._terminateReason = null; 
  this._dirty = false;

  
  this._loadTask = null;

  
  
  
  
  this._mainTask = null;

  
  this._timer = null;

  this._shutdown = false;
  this._networkRequest = null;

  
  
  this._firstEvaluate = true;

  this.init();
};

Experiments.Experiments.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback, Ci.nsIObserver]),

  



  get isReady() {
    return !this._shutdown;
  },

  init: function () {
    this._shutdown = false;
    configureLogging();

    gExperimentsEnabled = gPrefs.get(PREF_ENABLED, false);
    this._log.trace("enabled=" + gExperimentsEnabled + ", " + this.enabled);

    gPrefs.observe(PREF_LOGGING, configureLogging);
    gPrefs.observe(PREF_MANIFEST_URI, this.updateManifest, this);
    gPrefs.observe(PREF_ENABLED, this._toggleExperimentsEnabled, this);

    gPrefsTelemetry.observe(PREF_TELEMETRY_ENABLED, this._telemetryStatusChanged, this);

    AddonManager.shutdown.addBlocker("Experiments.jsm shutdown",
      this.uninit.bind(this),
      this._getState.bind(this)
    );

    this._registerWithAddonManager();

    this._loadTask = this._loadFromCache();

    return this._loadTask.then(
      () => {
        this._log.trace("_loadTask finished ok");
        this._loadTask = null;
        return this._run();
      },
      (e) => {
        this._log.error("_loadFromCache caught error: " + e);
        this._latestError = e;
        throw e;
      }
    );
  },

  










  uninit: Task.async(function* () {
    this._log.trace("uninit: started");
    yield this._loadTask;
    this._log.trace("uninit: finished with _loadTask");

    if (!this._shutdown) {
      this._log.trace("uninit: no previous shutdown");
      this._unregisterWithAddonManager();

      gPrefs.ignore(PREF_LOGGING, configureLogging);
      gPrefs.ignore(PREF_MANIFEST_URI, this.updateManifest, this);
      gPrefs.ignore(PREF_ENABLED, this._toggleExperimentsEnabled, this);

      gPrefsTelemetry.ignore(PREF_TELEMETRY_ENABLED, this._telemetryStatusChanged, this);

      if (this._timer) {
        this._timer.clear();
      }
    }

    this._shutdown = true;
    if (this._mainTask) {
      if (this._networkRequest) {
	try {
	  this._log.trace("Aborting pending network request: " + this._networkRequest);
	  this._networkRequest.abort();
	} catch (e) {
	  
	}
      }
      try {
        this._log.trace("uninit: waiting on _mainTask");
        yield this._mainTask;
      } catch (e if e instanceof AlreadyShutdownError) {
        
      } catch (e) {
        this._latestError = e;
        throw e;
      }
    }

    this._log.info("Completed uninitialization.");
  }),

  
  _getState: function() {
    let state = {
      isShutdown: this._shutdown,
      isEnabled: gExperimentsEnabled,
      isRefresh: this._refresh,
      isDirty: this._dirty,
      isFirstEvaluate: this._firstEvaluate,
      hasLoadTask: !!this._loadTask,
      hasMainTask: !!this._mainTask,
      hasTimer: !!this._hasTimer,
      hasAddonProvider: !!gAddonProvider,
      latestLogs: this._forensicsLogs,
      experiments: this._experiments ? this._experiments.keys() : null,
      terminateReason: this._terminateReason,
    };
    if (this._latestError) {
      if (typeof this._latestError == "object") {
        state.latestError = {
          message: this._latestError.message,
          stack: this._latestError.stack
        };
      } else {
        state.latestError = "" + this._latestError;
      }
    }
    return state;
  },

  _registerWithAddonManager: function (previousExperimentsProvider) {
    this._log.trace("Registering instance with Addon Manager.");

    AddonManager.addAddonListener(this);
    AddonManager.addInstallListener(this);

    if (!gAddonProvider) {
      
      
      this._log.trace("Registering previous experiment add-on provider.");
      gAddonProvider = previousExperimentsProvider || new Experiments.PreviousExperimentProvider(this);
      AddonManagerPrivate.registerProvider(gAddonProvider, [
          new AddonManagerPrivate.AddonType("experiment",
                                            URI_EXTENSION_STRINGS,
                                            STRING_TYPE_NAME,
                                            AddonManager.VIEW_TYPE_LIST,
                                            11000,
                                            AddonManager.TYPE_UI_HIDE_EMPTY),
      ]);
    }

  },

  _unregisterWithAddonManager: function () {
    this._log.trace("Unregistering instance with Addon Manager.");

    this._log.trace("Removing install listener from add-on manager.");
    AddonManager.removeInstallListener(this);
    this._log.trace("Removing addon listener from add-on manager.");
    AddonManager.removeAddonListener(this);
    this._log.trace("Finished unregistering with addon manager.");

    if (gAddonProvider) {
      this._log.trace("Unregistering previous experiment add-on provider.");
      AddonManagerPrivate.unregisterProvider(gAddonProvider);
      gAddonProvider = null;
    }
  },

  



  _setPreviousExperimentsProvider: function (provider) {
    this._unregisterWithAddonManager();
    this._registerWithAddonManager(provider);
  },

  


  _checkForShutdown: function() {
    if (this._shutdown) {
      throw new AlreadyShutdownError("uninit() already called");
    }
  },

  


  get enabled() {
    return gExperimentsEnabled;
  },

  


  set enabled(enabled) {
    this._log.trace("set enabled(" + enabled + ")");
    gPrefs.set(PREF_ENABLED, enabled);
  },

  _toggleExperimentsEnabled: Task.async(function* (enabled) {
    this._log.trace("_toggleExperimentsEnabled(" + enabled + ")");
    let wasEnabled = gExperimentsEnabled;
    gExperimentsEnabled = enabled && telemetryEnabled();

    if (wasEnabled == gExperimentsEnabled) {
      return;
    }

    if (gExperimentsEnabled) {
      yield this.updateManifest();
    } else {
      yield this.disableExperiment(TELEMETRY_LOG.TERMINATION.SERVICE_DISABLED);
      if (this._timer) {
        this._timer.clear();
      }
    }
  }),

  _telemetryStatusChanged: function () {
    this._toggleExperimentsEnabled(gExperimentsEnabled);
  },

  

















  getExperiments: function () {
    return Task.spawn(function*() {
      yield this._loadTask;
      let list = [];

      for (let [id, experiment] of this._experiments) {
        if (!experiment.startDate) {
          
          continue;
        }

        list.push({
          id: id,
          name: experiment._name,
          description: experiment._description,
          active: experiment.enabled,
          endDate: experiment.endDate.getTime(),
          detailURL: experiment._homepageURL,
	  branch: experiment.branch,
        });
      }

      
      list.sort((a, b) => b.endDate - a.endDate);
      return list;
    }.bind(this));
  },

  



  getActiveExperiment: function () {
    let experiment = this._getActiveExperiment();
    if (!experiment) {
      return null;
    }

    let info = {
      id: experiment.id,
      name: experiment._name,
      description: experiment._description,
      active: experiment.enabled,
      endDate: experiment.endDate.getTime(),
      detailURL: experiment._homepageURL,
    };

    return info;
  },

  





  



  setExperimentBranch: Task.async(function*(id, branchstr) {
    yield this._loadTask;
    let e = this._experiments.get(id);
    if (!e) {
      throw new Error("Experiment not found");
    }
    e.branch = String(branchstr);
    this._log.trace("setExperimentBranch(" + id + ", " + e.branch + ") _dirty=" + this._dirty);
    this._dirty = true;
    Services.obs.notifyObservers(null, EXPERIMENTS_CHANGED_TOPIC, null);
    yield this._run();
  }),
  









  getExperimentBranch: Task.async(function*(id=null) {
    yield this._loadTask;
    let e;
    if (id) {
      e = this._experiments.get(id);
      if (!e) {
        throw new Error("Experiment not found");
      }
    } else {
      e = this._getActiveExperiment();
      if (e === null) {
	throw new Error("No active experiment");
      }
    }
    return e.branch;
  }),

  


  _dateIsTodayUTC: function (d) {
    let now = this._policy.now();

    return stripDateToMidnight(now).getTime() == stripDateToMidnight(d).getTime();
  },

  









  lastActiveToday: function () {
    return Task.spawn(function* getMostRecentActiveExperimentTask() {
      let experiments = yield this.getExperiments();

      
      
      for (let experiment of experiments) {
        if (experiment.active) {
          return experiment;
        }

        if (experiment.endDate && this._dateIsTodayUTC(experiment.endDate)) {
          return experiment;
        }
      }
      return null;
    }.bind(this));
  },

  _run: function() {
    this._log.trace("_run");
    this._checkForShutdown();
    if (!this._mainTask) {
      this._mainTask = Task.spawn(function*() {
        try {
          yield this._main();
	} catch (e if e instanceof CacheWriteError) {
	  
        } catch (e) {
          this._log.error("_main caught error: " + e);
          return;
        } finally {
          this._mainTask = null;
        }
        this._log.trace("_main finished, scheduling next run");
        try {
          yield this._scheduleNextRun();
        } catch (ex if ex instanceof AlreadyShutdownError) {
          
        }
      }.bind(this));
    }
    return this._mainTask;
  },

  _main: function*() {
    do {
      this._log.trace("_main iteration");
      yield this._loadTask;
      if (!gExperimentsEnabled) {
        this._refresh = false;
      }

      if (this._refresh) {
        yield this._loadManifest();
      }
      yield this._evaluateExperiments();
      if (this._dirty) {
        yield this._saveToCache();
      }
      
      
    }
    while (this._refresh || this._terminateReason || this._dirty);
  },

  _loadManifest: function*() {
    this._log.trace("_loadManifest");
    let uri = Services.urlFormatter.formatURLPref(PREF_BRANCH + PREF_MANIFEST_URI);

    this._checkForShutdown();

    this._refresh = false;
    try {
      let responseText = yield this._httpGetRequest(uri);
      this._log.trace("_loadManifest() - responseText=\"" + responseText + "\"");

      if (this._shutdown) {
        return;
      }

      let data = JSON.parse(responseText);
      this._updateExperiments(data);
    } catch (e) {
      this._log.error("_loadManifest - failure to fetch/parse manifest (continuing anyway): " + e);
    }
  },

  






  updateManifest: function () {
    this._log.trace("updateManifest()");

    if (!gExperimentsEnabled) {
      return Promise.reject(new Error("experiments are disabled"));
    }

    if (this._shutdown) {
      return Promise.reject(Error("uninit() alrady called"));
    }

    this._refresh = true;
    return this._run();
  },

  notify: function (timer) {
    this._log.trace("notify()");
    this._checkForShutdown();
    return this._run();
  },

  

  onUninstalled: function (addon) {
    this._log.trace("onUninstalled() - addon id: " + addon.id);
    if (gActiveUninstallAddonIDs.has(addon.id)) {
      this._log.trace("matches pending uninstall");
      return;
    }
    let activeExperiment = this._getActiveExperiment();
    if (!activeExperiment || activeExperiment._addonId != addon.id) {
      return;
    }

    this.disableExperiment(TELEMETRY_LOG.TERMINATION.ADDON_UNINSTALLED);
  },

  onInstallStarted: function (install) {
    if (install.addon.type != "experiment") {
      return;
    }

    this._log.trace("onInstallStarted() - " + install.addon.id);
    if (install.addon.appDisabled) {
      
      return;
    }

    
    

    
    
    
    
    
    
    
    
    
    

    if (this._trackedAddonIds.has(install.addon.id)) {
      this._log.info("onInstallStarted allowing install because add-on ID " +
                     "tracked by us.");
      return;
    }

    if (gActiveInstallURLs.has(install.sourceURI.spec)) {
      this._log.info("onInstallStarted allowing install because install " +
                     "tracked by us.");
      return;
    }

    this._log.warn("onInstallStarted cancelling install of unknown " +
                   "experiment add-on: " + install.addon.id);
    return false;
  },

  

  _getExperimentByAddonId: function (addonId) {
    for (let [, entry] of this._experiments) {
      if (entry._addonId === addonId) {
        return entry;
      }
    }

    return null;
  },

  



  _httpGetRequest: function (url) {
    this._log.trace("httpGetRequest(" + url + ")");
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    try {
      xhr.open("GET", url);
    } catch (e) {
      this._log.error("httpGetRequest() - Error opening request to " + url + ": " + e);
      return Promise.reject(new Error("Experiments - Error opening XHR for " + url));
    }

    this._networkRequest = xhr;
    let deferred = Promise.defer();

    let log = this._log;
    let errorhandler = (evt) => {
      log.error("httpGetRequest::onError() - Error making request to " + url + ": " + evt.type);
      deferred.reject(new Error("Experiments - XHR error for " + url + " - " + evt.type));
      this._networkRequest = null;
    };
    xhr.onerror = errorhandler;
    xhr.ontimeout = errorhandler;
    xhr.onabort = errorhandler;

    xhr.onload = (event) => {
      if (xhr.status !== 200 && xhr.state !== 0) {
        log.error("httpGetRequest::onLoad() - Request to " + url + " returned status " + xhr.status);
        deferred.reject(new Error("Experiments - XHR status for " + url + " is " + xhr.status));
	this._networkRequest = null;
        return;
      }

      deferred.resolve(xhr.responseText);
      this._networkRequest = null;
    };

    if (xhr.channel instanceof Ci.nsISupportsPriority) {
      xhr.channel.priority = Ci.nsISupportsPriority.PRIORITY_LOWEST;
    }

    xhr.timeout = MANIFEST_FETCH_TIMEOUT_MSEC;
    xhr.send(null);
    return deferred.promise;
  },

  


  get _cacheFilePath() {
    return OS.Path.join(OS.Constants.Path.profileDir, FILE_CACHE);
  },

  


  _saveToCache: function* () {
    this._log.trace("_saveToCache");
    let path = this._cacheFilePath;
    this._dirty = false;
    try {
      let textData = JSON.stringify({
        version: CACHE_VERSION,
        data: [e[1].toJSON() for (e of this._experiments.entries())],
      });

      let encoder = new TextEncoder();
      let data = encoder.encode(textData);
      let options = { tmpPath: path + ".tmp", compression: "lz4" };
      yield this._policy.delayCacheWrite(OS.File.writeAtomic(path, data, options));
    } catch (e) {
      
      this._dirty = true;
      this._log.error("_saveToCache failed and caught error: " + e);
      throw new CacheWriteError();
    }

    this._log.debug("_saveToCache saved to " + path);
  },

  


  _loadFromCache: Task.async(function* () {
    this._log.trace("_loadFromCache");
    let path = this._cacheFilePath;
    try {
      let result = yield loadJSONAsync(path, { compression: "lz4" });
      this._populateFromCache(result);
    } catch (e if e instanceof OS.File.Error && e.becauseNoSuchFile) {
      
      this._experiments = new Map();
    }
  }),

  _populateFromCache: function (data) {
    this._log.trace("populateFromCache() - data: " + JSON.stringify(data));

    
    
    if (CACHE_VERSION !== data.version) {
      throw new Error("Experiments::_populateFromCache() - invalid cache version");
    }

    let experiments = new Map();
    for (let item of data.data) {
      let entry = new Experiments.ExperimentEntry(this._policy);
      if (!entry.initFromCacheData(item)) {
        continue;
      }
      experiments.set(entry.id, entry);
    }

    this._experiments = experiments;
  },

  



  _updateExperiments: function (manifestObject) {
    this._log.trace("_updateExperiments() - experiments: " + JSON.stringify(manifestObject));

    if (manifestObject.version !== MANIFEST_VERSION) {
      this._log.warning("updateExperiments() - unsupported version " + manifestObject.version);
    }

    let experiments = new Map(); 

    
    for (let data of manifestObject.experiments) {
      let entry = this._experiments.get(data.id);

      if (entry) {
        if (!entry.updateFromManifestData(data)) {
          this._log.error("updateExperiments() - Invalid manifest data for " + data.id);
          continue;
        }
      } else {
        entry = new Experiments.ExperimentEntry(this._policy);
        if (!entry.initFromManifestData(data)) {
          continue;
        }
      }

      if (entry.shouldDiscard()) {
        continue;
      }

      experiments.set(entry.id, entry);
    }

    
    
    for (let [id, entry] of this._experiments) {
      if (experiments.has(id)) {
        continue;
      }

      if (!entry.startDate || entry.shouldDiscard()) {
        this._log.trace("updateExperiments() - discarding entry for " + id);
        continue;
      }

      experiments.set(id, entry);
    }

    this._experiments = experiments;
    this._dirty = true;
  },

  getActiveExperimentID: function() {
    if (!this._experiments) {
      return null;
    }
    let e = this._getActiveExperiment();
    if (!e) {
      return null;
    }
    return e.id;
  },

  getActiveExperimentBranch: function() {
    if (!this._experiments) {
      return null;
    }
    let e = this._getActiveExperiment();
    if (!e) {
      return null;
    }
    return e.branch;
  },

  _getActiveExperiment: function () {
    let enabled = [experiment for ([,experiment] of this._experiments) if (experiment._enabled)];

    if (enabled.length == 1) {
      return enabled[0];
    }

    if (enabled.length > 1) {
      this._log.error("getActiveExperimentId() - should not have more than 1 active experiment");
      throw new Error("have more than 1 active experiment");
    }

    return null;
  },

  




  disableExperiment: function (reason) {
    if (!reason) {
      throw new Error("Must specify a termination reason.");
    }

    this._log.trace("disableExperiment()");
    this._terminateReason = reason;
    return this._run();
  },

  


  get _trackedAddonIds() {
    if (!this._experiments) {
      return new Set();
    }

    return new Set([e._addonId for ([,e] of this._experiments) if (e._addonId)]);
  },

  



  _evaluateExperiments: function*() {
    this._log.trace("_evaluateExperiments");

    this._checkForShutdown();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let installedExperiments = yield installedExperimentAddons();
    let expectedAddonIds = this._trackedAddonIds;
    let unknownAddons = [a for (a of installedExperiments) if (!expectedAddonIds.has(a.id))];
    if (unknownAddons.length) {
      this._log.warn("_evaluateExperiments() - unknown add-ons in AddonManager: " +
                     [a.id for (a of unknownAddons)].join(", "));

      yield uninstallAddons(unknownAddons);
    }

    let activeExperiment = this._getActiveExperiment();
    let activeChanged = false;
    let now = this._policy.now();

    if (!activeExperiment) {
      
      gPrefs.set(PREF_ACTIVE_EXPERIMENT, false);
    }

    
    
    
    if (activeExperiment) {
      let changes;
      let shouldStopResult = yield activeExperiment.shouldStop();
      if (shouldStopResult.shouldStop) {
        let expireReasons = ["endTime", "maxActiveSeconds"];
        let kind, reason;

        if (expireReasons.indexOf(shouldStopResult.reason[0]) != -1) {
          kind = TELEMETRY_LOG.TERMINATION.EXPIRED;
          reason = null;
        } else {
          kind = TELEMETRY_LOG.TERMINATION.RECHECK;
          reason = shouldStopResult.reason;
        }
        changes = yield activeExperiment.stop(kind, reason);
      }
      else if (this._terminateReason) {
        changes = yield activeExperiment.stop(this._terminateReason);
      }
      else {
        changes = yield activeExperiment.reconcileAddonState();
      }

      if (changes) {
        this._dirty = true;
        activeChanged = true;
      }

      if (!activeExperiment._enabled) {
        activeExperiment = null;
        activeChanged = true;
      }
    }

    this._terminateReason = null;

    if (!activeExperiment && gExperimentsEnabled) {
      for (let [id, experiment] of this._experiments) {
        let applicable;
        let reason = null;
        try {
          applicable = yield experiment.isApplicable();
        }
        catch (e) {
          applicable = false;
          reason = e;
        }

        if (!applicable && reason && reason[0] != "was-active") {
          
          let desc = TELEMETRY_LOG.ACTIVATION;
          let data = [TELEMETRY_LOG.ACTIVATION.REJECTED, id];
          data = data.concat(reason);
          const key = TELEMETRY_LOG.ACTIVATION_KEY;
          TelemetryLog.log(key, data);
          this._log.trace("evaluateExperiments() - added " + key + " to TelemetryLog: " + JSON.stringify(data));
        }

        if (!applicable) {
          continue;
        }

        this._log.debug("evaluateExperiments() - activating experiment " + id);
        try {
          yield experiment.start();
          activeChanged = true;
          activeExperiment = experiment;
          this._dirty = true;
          break;
        } catch (e) {
          
          this._log.error("evaluateExperiments() - Unable to start experiment: " + e.message);
          experiment._enabled = false;
          yield experiment.reconcileAddonState();
        }
      }
    }

    gPrefs.set(PREF_ACTIVE_EXPERIMENT, activeExperiment != null);

    if (activeChanged || this._firstEvaluate) {
      Services.obs.notifyObservers(null, EXPERIMENTS_CHANGED_TOPIC, null);
      this._firstEvaluate = false;
    }

    if ("@mozilla.org/toolkit/crash-reporter;1" in Cc && activeExperiment) {
      try {
        gCrashReporter.annotateCrashReport("ActiveExperiment", activeExperiment.id);
        gCrashReporter.annotateCrashReport("ActiveExperimentBranch", activeExperiment.branch);
      } catch (e) {
        
      }
    }
  },

  


  _scheduleNextRun: function () {
    this._checkForShutdown();

    if (this._timer) {
      this._timer.clear();
    }

    if (!gExperimentsEnabled || this._experiments.length == 0) {
      return;
    }

    let time = null;
    let now = this._policy.now().getTime();
    if (this._dirty) {
      
      time = now + 1000 * CACHE_WRITE_RETRY_DELAY_SEC;
    }

    for (let [id, experiment] of this._experiments) {
      let scheduleTime = experiment.getScheduleTime();
      if (scheduleTime > now) {
        if (time !== null) {
          time = Math.min(time, scheduleTime);
        } else {
          time = scheduleTime;
        }
      }
    }

    if (time === null) {
      
      return;
    }

    this._log.trace("scheduleExperimentEvaluation() - scheduling for "+time+", now: "+now);
    this._policy.oneshotTimer(this.notify, time - now, this, "_timer");
  },
};






Experiments.ExperimentEntry = function (policy) {
  this._policy = policy || new Experiments.Policy();
  this._log = Log.repository.getLoggerWithMessagePrefix(
    "Browser.Experiments.Experiments",
    "ExperimentEntry #" + gExperimentEntryCounter++ + "::");

  
  this._enabled = false;
  
  this._startDate = null;
  
  this._endDate = null;
  
  this._manifestData = null;
  
  this._needsUpdate = false;
  
  this._randomValue = null;
  
  this._lastChangedDate = null;
  
  this._failedStart = false;
  
  this._branch = null;

  
  this._name = null;
  this._description = null;
  this._homepageURL = null;
  this._addonId = null;
};

Experiments.ExperimentEntry.prototype = {
  MANIFEST_REQUIRED_FIELDS: new Set([
    "id",
    "xpiURL",
    "xpiHash",
    "startTime",
    "endTime",
    "maxActiveSeconds",
    "appName",
    "channel",
  ]),

  MANIFEST_OPTIONAL_FIELDS: new Set([
    "maxStartTime",
    "minVersion",
    "maxVersion",
    "version",
    "minBuildID",
    "maxBuildID",
    "buildIDs",
    "os",
    "locale",
    "sample",
    "disabled",
    "frozen",
    "jsfilter",
  ]),

  SERIALIZE_KEYS: new Set([
    "_enabled",
    "_manifestData",
    "_needsUpdate",
    "_randomValue",
    "_failedStart",
    "_name",
    "_description",
    "_homepageURL",
    "_addonId",
    "_startDate",
    "_endDate",
    "_branch",
  ]),

  DATE_KEYS: new Set([
    "_startDate",
    "_endDate",
  ]),

  UPGRADE_KEYS: new Map([
    ["_branch", null],
  ]),

  ADDON_CHANGE_NONE: 0,
  ADDON_CHANGE_INSTALL: 1,
  ADDON_CHANGE_UNINSTALL: 2,
  ADDON_CHANGE_ENABLE: 4,

  




  initFromManifestData: function (data) {
    if (!this._isManifestDataValid(data)) {
      return false;
    }

    this._manifestData = data;

    this._randomValue = this._policy.random();
    this._lastChangedDate = this._policy.now();

    return true;
  },

  get enabled() {
    return this._enabled;
  },

  get id() {
    return this._manifestData.id;
  },

  get branch() {
    return this._branch;
  },

  set branch(v) {
    this._branch = v;
  },

  get startDate() {
    return this._startDate;
  },

  get endDate() {
    if (!this._startDate) {
      return null;
    }

    let endTime = 0;

    if (!this._enabled) {
      return this._endDate;
    }

    let maxActiveMs = 1000 * this._manifestData.maxActiveSeconds;
    endTime = Math.min(1000 * this._manifestData.endTime,
                       this._startDate.getTime() + maxActiveMs);

    return new Date(endTime);
  },

  get needsUpdate() {
    return this._needsUpdate;
  },

  




  initFromCacheData: function (data) {
    for (let [key, dval] of this.UPGRADE_KEYS) {
      if (!(key in data)) {
        data[key] = dval;
      }
    }

    for (let key of this.SERIALIZE_KEYS) {
      if (!(key in data) && !this.DATE_KEYS.has(key)) {
        this._log.error("initFromCacheData() - missing required key " + key);
        return false;
      }
    };

    if (!this._isManifestDataValid(data._manifestData)) {
      return false;
    }

    
    

    this.SERIALIZE_KEYS.forEach(key => {
      if (!this.DATE_KEYS.has(key)) {
        this[key] = data[key];
      }
    });

    this.DATE_KEYS.forEach(key => {
      if (key in data) {
        let date = new Date();
        date.setTime(data[key]);
        this[key] = date;
      }
    });

    this._lastChangedDate = this._policy.now();

    return true;
  },

  


  toJSON: function () {
    let obj = {};

    

    this.SERIALIZE_KEYS.forEach(key => {
      if (!this.DATE_KEYS.has(key)) {
        obj[key] = this[key];
      }
    });

    this.DATE_KEYS.forEach(key => {
      if (this[key]) {
        obj[key] = this[key].getTime();
      }
    });

    return obj;
  },

  




  updateFromManifestData: function (data) {
    let old = this._manifestData;

    if (!this._isManifestDataValid(data)) {
      return false;
    }

    if (this._enabled) {
      if (old.xpiHash !== data.xpiHash) {
        
        this._needsUpdate = true;
      }
    } else if (this._failedStart &&
               (old.xpiHash !== data.xpiHash) ||
               (old.xpiURL !== data.xpiURL)) {
      
      
      this._failedStart = false;
    }

    this._manifestData = data;
    this._lastChangedDate = this._policy.now();

    return true;
  },

  





  isApplicable: function () {
    let versionCmp = Cc["@mozilla.org/xpcom/version-comparator;1"]
                              .getService(Ci.nsIVersionComparator);
    let app = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
    let runtime = Cc["@mozilla.org/xre/app-info;1"]
                    .getService(Ci.nsIXULRuntime);

    let locale = this._policy.locale();
    let channel = this._policy.updatechannel();
    let data = this._manifestData;

    let now = this._policy.now() / 1000; 
    let minActive = MIN_EXPERIMENT_ACTIVE_SECONDS;
    let maxActive = data.maxActiveSeconds || 0;
    let startSec = (this.startDate || 0) / 1000;

    this._log.trace("isApplicable() - now=" + now
                    + ", randomValue=" + this._randomValue
                    + ", data=" + JSON.stringify(this._manifestData));

    

    if (!this.enabled && this._endDate) {
      return Promise.reject(["was-active"]);
    }

    

    let simpleChecks = [
      { name: "failedStart",
        condition: () => !this._failedStart },
      { name: "disabled",
        condition: () => !data.disabled },
      { name: "frozen",
        condition: () => !data.frozen || this._enabled },
      { name: "startTime",
        condition: () => now >= data.startTime },
      { name: "endTime",
        condition: () => now < data.endTime },
      { name: "maxStartTime",
        condition: () => this._startDate || !data.maxStartTime || now <= data.maxStartTime },
      { name: "maxActiveSeconds",
        condition: () => !this._startDate || now <= (startSec + maxActive) },
      { name: "appName",
        condition: () => !data.appName || data.appName.indexOf(app.name) != -1 },
      { name: "minBuildID",
        condition: () => !data.minBuildID || app.platformBuildID >= data.minBuildID },
      { name: "maxBuildID",
        condition: () => !data.maxBuildID || app.platformBuildID <= data.maxBuildID },
      { name: "buildIDs",
        condition: () => !data.buildIDs || data.buildIDs.indexOf(app.platformBuildID) != -1 },
      { name: "os",
        condition: () => !data.os || data.os.indexOf(runtime.OS) != -1 },
      { name: "channel",
        condition: () => !data.channel || data.channel.indexOf(channel) != -1 },
      { name: "locale",
        condition: () => !data.locale || data.locale.indexOf(locale) != -1 },
      { name: "sample",
        condition: () => data.sample === undefined || this._randomValue <= data.sample },
      { name: "version",
        condition: () => !data.version || data.version.indexOf(app.version) != -1 },
      { name: "minVersion",
        condition: () => !data.minVersion || versionCmp.compare(app.version, data.minVersion) >= 0 },
      { name: "maxVersion",
        condition: () => !data.maxVersion || versionCmp.compare(app.version, data.maxVersion) <= 0 },
    ];

    for (let check of simpleChecks) {
      let result = check.condition();
      if (!result) {
        this._log.debug("isApplicable() - id="
                        + data.id + " - test '" + check.name + "' failed");
        return Promise.reject([check.name]);
      }
    }

    if (data.jsfilter) {
      return this._runFilterFunction(data.jsfilter);
    }

    return Promise.resolve(true);
  },

  



  _runFilterFunction: function (jsfilter) {
    this._log.trace("runFilterFunction() - filter: " + jsfilter);

    return Task.spawn(function ExperimentEntry_runFilterFunction_task() {
      const nullprincipal = Cc["@mozilla.org/nullprincipal;1"].createInstance(Ci.nsIPrincipal);
      let options = {
        sandboxName: "telemetry experiments jsfilter sandbox",
        wantComponents: false,
      };

      let sandbox = Cu.Sandbox(nullprincipal, options);
      try {
        Cu.evalInSandbox(jsfilter, sandbox);
      } catch (e) {
        this._log.error("runFilterFunction() - failed to eval jsfilter: " + e.message);
        throw ["jsfilter-evalfailed"];
      }

      
      
      sandbox._hr = yield this._policy.healthReportPayload();
      Object.defineProperty(sandbox, "_t",
        { get: () => JSON.stringify(this._policy.telemetryPayload()) });

      let result = false;
      try {
        result = !!Cu.evalInSandbox("filter({healthReportPayload: JSON.parse(_hr), telemetryPayload: JSON.parse(_t)})", sandbox);
      }
      catch (e) {
        this._log.debug("runFilterFunction() - filter function failed: "
                      + e.message + ", " + e.stack);
        throw ["jsfilter-threw", e.message];
      }
      finally {
        Cu.nukeSandbox(sandbox);
      }

      if (!result) {
        throw ["jsfilter-false"];
      }

      throw new Task.Result(true);
    }.bind(this));
  },

  




  start: Task.async(function* () {
    this._log.trace("start() for " + this.id);

    this._enabled = true;
    return yield this.reconcileAddonState();
  }),

  
  _installAddon: Task.async(function* () {
    let deferred = Promise.defer();

    let hash = this._policy.ignoreHashes ? null : this._manifestData.xpiHash;

    let install = yield addonInstallForURL(this._manifestData.xpiURL, hash);
    gActiveInstallURLs.add(install.sourceURI.spec);

    let failureHandler = (install, handler) => {
      let message = "AddonInstall " + handler + " for " + this.id + ", state=" +
                   (install.state || "?") + ", error=" + install.error;
      this._log.error("_installAddon() - " + message);
      this._failedStart = true;
      gActiveInstallURLs.delete(install.sourceURI.spec);

      TelemetryLog.log(TELEMETRY_LOG.ACTIVATION_KEY,
                      [TELEMETRY_LOG.ACTIVATION.INSTALL_FAILURE, this.id]);

      deferred.reject(new Error(message));
    };

    let listener = {
      _expectedID: null,

      onDownloadEnded: install => {
        this._log.trace("_installAddon() - onDownloadEnded for " + this.id);

        if (install.existingAddon) {
          this._log.warn("_installAddon() - onDownloadEnded, addon already installed");
        }

        if (install.addon.type !== "experiment") {
          this._log.error("_installAddon() - onDownloadEnded, wrong addon type");
          install.cancel();
        }
      },

      onInstallStarted: install => {
        this._log.trace("_installAddon() - onInstallStarted for " + this.id);

        if (install.existingAddon) {
          this._log.warn("_installAddon() - onInstallStarted, addon already installed");
        }

        if (install.addon.type !== "experiment") {
          this._log.error("_installAddon() - onInstallStarted, wrong addon type");
          return false;
        }
      },

      onInstallEnded: install => {
        this._log.trace("_installAddon() - install ended for " + this.id);
        gActiveInstallURLs.delete(install.sourceURI.spec);

        this._lastChangedDate = this._policy.now();
        this._startDate = this._policy.now();
        this._enabled = true;

        TelemetryLog.log(TELEMETRY_LOG.ACTIVATION_KEY,
                       [TELEMETRY_LOG.ACTIVATION.ACTIVATED, this.id]);

        let addon = install.addon;
        this._name = addon.name;
        this._addonId = addon.id;
        this._description = addon.description || "";
        this._homepageURL = addon.homepageURL || "";

        
        if (addon.userDisabled) {
          this._log.trace("Add-on is disabled. Enabling.");
          listener._expectedID = addon.id;
          AddonManager.addAddonListener(listener);
          addon.userDisabled = false;
        } else {
          this._log.trace("Add-on is enabled. start() completed.");
          deferred.resolve();
        }
      },

      onEnabled: addon => {
        this._log.info("onEnabled() for " + addon.id);

        if (addon.id != listener._expectedID) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        deferred.resolve();
      },
    };

    ["onDownloadCancelled", "onDownloadFailed", "onInstallCancelled", "onInstallFailed"]
      .forEach(what => {
        listener[what] = install => failureHandler(install, what)
      });

    install.addListener(listener);
    install.install();

    return yield deferred.promise;
  }),

  








  stop: Task.async(function* (terminationKind, terminationReason) {
    this._log.trace("stop() - id=" + this.id + ", terminationKind=" + terminationKind);
    if (!this._enabled) {
      throw new Error("Must not call stop() on an inactive experiment.");
    }

    this._enabled = false;
    let now = this._policy.now();
    this._lastChangedDate = now;
    this._endDate = now;

    let changes = yield this.reconcileAddonState();
    this._logTermination(terminationKind, terminationReason);

    if (terminationKind == TELEMETRY_LOG.TERMINATION.ADDON_UNINSTALLED) {
      changes |= this.ADDON_CHANGE_UNINSTALL;
    }

    return changes;
  }),

  






  reconcileAddonState: Task.async(function* () {
    this._log.trace("reconcileAddonState()");

    if (!this._enabled) {
      if (!this._addonId) {
        this._log.trace("reconcileAddonState() - Experiment is not enabled and " +
                        "has no add-on. Doing nothing.");
        return this.ADDON_CHANGE_NONE;
      }

      let addon = yield this._getAddon();
      if (!addon) {
        this._log.trace("reconcileAddonState() - Inactive experiment has no " +
                        "add-on. Doing nothing.");
        return this.ADDON_CHANGE_NONE;
      }

      this._log.info("reconcileAddonState() - Uninstalling add-on for inactive " +
                     "experiment: " + addon.id);
      gActiveUninstallAddonIDs.add(addon.id);
      yield uninstallAddons([addon]);
      gActiveUninstallAddonIDs.delete(addon.id);
      return this.ADDON_CHANGE_UNINSTALL;
    }

    

    let changes = 0;

    
    let currentAddon = yield this._getAddon();

    
    
    if (currentAddon && this._needsUpdate) {
      this._log.info("reconcileAddonState() - Uninstalling add-on because update " +
                     "needed: " + currentAddon.id);
      gActiveUninstallAddonIDs.add(currentAddon.id);
      yield uninstallAddons([currentAddon]);
      gActiveUninstallAddonIDs.delete(currentAddon.id);
      changes |= this.ADDON_CHANGE_UNINSTALL;
    }

    if (!currentAddon || this._needsUpdate) {
      this._log.info("reconcileAddonState() - Installing add-on.");
      yield this._installAddon();
      changes |= this.ADDON_CHANGE_INSTALL;
    }

    let addon = yield this._getAddon();
    if (!addon) {
      throw new Error("Could not obtain add-on for experiment that should be " +
                      "enabled.");
    }

    
    if (!addon.userDisabled) {
      return changes;
    }

    let deferred = Promise.defer();

    
    let listener = {
      onEnabled: enabledAddon => {
        if (enabledAddon.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        deferred.resolve();
      },
    };

    this._log.info("Activating add-on: " + addon.id);
    AddonManager.addAddonListener(listener);
    addon.userDisabled = false;
    yield deferred.promise;
    changes |= this.ADDON_CHANGE_ENABLE;

    this._log.info("Add-on has been enabled: " + addon.id);
    return changes;
   }),

  




  _getAddon: function () {
    if (!this._addonId) {
      return Promise.resolve(null);
    }

    let deferred = Promise.defer();

    AddonManager.getAddonByID(this._addonId, (addon) => {
      if (addon && addon.appDisabled) {
        
        addon = null;
      }

      deferred.resolve(addon);
    });

    return deferred.promise;
  },

  _logTermination: function (terminationKind, terminationReason) {
    if (terminationKind === undefined) {
      return;
    }

    if (!(terminationKind in TELEMETRY_LOG.TERMINATION)) {
      this._log.warn("stop() - unknown terminationKind " + terminationKind);
      return;
    }

    let data = [terminationKind, this.id];
    if (terminationReason) {
      data = data.concat(terminationReason);
    }

    TelemetryLog.log(TELEMETRY_LOG.TERMINATION_KEY, data);
  },

  


  shouldStop: function () {
    if (!this._enabled) {
      throw new Error("shouldStop must not be called on disabled experiments.");
    }

    let data = this._manifestData;
    let now = this._policy.now() / 1000; 
    let maxActiveSec = data.maxActiveSeconds || 0;

    let deferred = Promise.defer();
    this.isApplicable().then(
      () => deferred.resolve({shouldStop: false}),
      reason => deferred.resolve({shouldStop: true, reason: reason})
    );

    return deferred.promise;
  },

  


  shouldDiscard: function () {
    let limit = this._policy.now();
    limit.setDate(limit.getDate() - KEEP_HISTORY_N_DAYS);
    return (this._lastChangedDate < limit);
  },

  



  getScheduleTime: function () {
    if (this._enabled) {
      let now = this._policy.now();
      let startTime = this._startDate.getTime();
      let maxActiveTime = startTime + 1000 * this._manifestData.maxActiveSeconds;
      return Math.min(1000 * this._manifestData.endTime,  maxActiveTime);
    }

    if (this._endDate) {
      return this._endDate.getTime();
    }

    return 1000 * this._manifestData.startTime;
  },

  


  _isManifestDataValid: function (data) {
    this._log.trace("isManifestDataValid() - data: " + JSON.stringify(data));

    for (let key of this.MANIFEST_REQUIRED_FIELDS) {
      if (!(key in data)) {
        this._log.error("isManifestDataValid() - missing required key: " + key);
        return false;
      }
    }

    for (let key in data) {
      if (!this.MANIFEST_OPTIONAL_FIELDS.has(key) &&
          !this.MANIFEST_REQUIRED_FIELDS.has(key)) {
        this._log.error("isManifestDataValid() - unknown key: " + key);
        return false;
      }
    }

    return true;
  },
};








let stripDateToMidnight = function (d) {
  let m = new Date(d);
  m.setUTCHours(0, 0, 0, 0);

  return m;
};

function ExperimentsLastActiveMeasurement1() {
  Metrics.Measurement.call(this);
}
function ExperimentsLastActiveMeasurement2() {
  Metrics.Measurement.call(this);
}

const FIELD_DAILY_LAST_TEXT = {type: Metrics.Storage.FIELD_DAILY_LAST_TEXT};

ExperimentsLastActiveMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "info",
  version: 1,

  fields: {
    lastActive: FIELD_DAILY_LAST_TEXT,
  }
});
ExperimentsLastActiveMeasurement2.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "info",
  version: 2,

  fields: {
    lastActive: FIELD_DAILY_LAST_TEXT,
    lastActiveBranch: FIELD_DAILY_LAST_TEXT,
  }
});

this.ExperimentsProvider = function () {
  Metrics.Provider.call(this);

  this._experiments = null;
};

ExperimentsProvider.prototype = Object.freeze({
  __proto__: Metrics.Provider.prototype,

  name: "org.mozilla.experiments",

  measurementTypes: [
    ExperimentsLastActiveMeasurement1,
    ExperimentsLastActiveMeasurement2,
  ],

  _OBSERVERS: [
    EXPERIMENTS_CHANGED_TOPIC,
  ],

  postInit: function () {
    for (let o of this._OBSERVERS) {
      Services.obs.addObserver(this, o, false);
    }

    return Promise.resolve();
  },

  onShutdown: function () {
    for (let o of this._OBSERVERS) {
      Services.obs.removeObserver(this, o);
    }

    return Promise.resolve();
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case EXPERIMENTS_CHANGED_TOPIC:
        this.recordLastActiveExperiment();
        break;
    }
  },

  collectDailyData: function () {
    return this.recordLastActiveExperiment();
  },

  recordLastActiveExperiment: function () {
    if (!gExperimentsEnabled) {
      return Promise.resolve();
    }

    if (!this._experiments) {
      this._experiments = Experiments.instance();
    }

    let m = this.getMeasurement(ExperimentsLastActiveMeasurement2.prototype.name,
                                ExperimentsLastActiveMeasurement2.prototype.version);

    return this.enqueueStorageOperation(() => {
      return Task.spawn(function* recordTask() {
        let todayActive = yield this._experiments.lastActiveToday();
        if (!todayActive) {
          this._log.info("No active experiment on this day: " +
                         this._experiments._policy.now());
          return;
        }

        this._log.info("Recording last active experiment: " + todayActive.id);
        yield m.setDailyLastText("lastActive", todayActive.id,
                                 this._experiments._policy.now());
        let branch = todayActive.branch;
        if (branch) {
          yield m.setDailyLastText("lastActiveBranch", branch,
                                   this._experiments._policy.now());
        }
      }.bind(this));
    });
  },
});









this.Experiments.PreviousExperimentProvider = function (experiments) {
  this._experiments = experiments;
  this._experimentList = [];
  this._log = Log.repository.getLoggerWithMessagePrefix(
    "Browser.Experiments.Experiments",
    "PreviousExperimentProvider #" + gPreviousProviderCounter++ + "::");
}

this.Experiments.PreviousExperimentProvider.prototype = Object.freeze({
  get name() "PreviousExperimentProvider",

  startup: function () {
    this._log.trace("startup()");
    Services.obs.addObserver(this, EXPERIMENTS_CHANGED_TOPIC, false);
  },

  shutdown: function () {
    this._log.trace("shutdown()");
    try {
      Services.obs.removeObserver(this, EXPERIMENTS_CHANGED_TOPIC);
    } catch(e) {
      
    }
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case EXPERIMENTS_CHANGED_TOPIC:
        this._updateExperimentList();
        break;
    }
  },

  getAddonByID: function (id, cb) {
    for (let experiment of this._experimentList) {
      if (experiment.id == id) {
        cb(new PreviousExperimentAddon(experiment));
        return;
      }
    }

    cb(null);
  },

  getAddonsByTypes: function (types, cb) {
    if (types && types.length > 0 && types.indexOf("experiment") == -1) {
      cb([]);
      return;
    }

    cb([new PreviousExperimentAddon(e) for (e of this._experimentList)]);
  },

  _updateExperimentList: function () {
    return this._experiments.getExperiments().then((experiments) => {
      let list = [e for (e of experiments) if (!e.active)];

      let newMap = new Map([[e.id, e] for (e of list)]);
      let oldMap = new Map([[e.id, e] for (e of this._experimentList)]);

      let added = [e.id for (e of list) if (!oldMap.has(e.id))];
      let removed = [e.id for (e of this._experimentList) if (!newMap.has(e.id))];

      for (let id of added) {
        this._log.trace("updateExperimentList() - adding " + id);
        let wrapper = new PreviousExperimentAddon(newMap.get(id));
        AddonManagerPrivate.callInstallListeners("onExternalInstall", null, wrapper, null, false);
        AddonManagerPrivate.callAddonListeners("onInstalling", wrapper, false);
      }

      for (let id of removed) {
        this._log.trace("updateExperimentList() - removing " + id);
        let wrapper = new PreviousExperimentAddon(oldMap.get(id));
        AddonManagerPrivate.callAddonListeners("onUninstalling", plugin, false);
      }

      this._experimentList = list;

      for (let id of added) {
        let wrapper = new PreviousExperimentAddon(newMap.get(id));
        AddonManagerPrivate.callAddonListeners("onInstalled", wrapper);
      }

      for (let id of removed) {
        let wrapper = new PreviousExperimentAddon(oldMap.get(id));
        AddonManagerPrivate.callAddonListeners("onUninstalled", wrapper);
      }

      return this._experimentList;
    });
  },
});




function PreviousExperimentAddon(experiment) {
  this._id = experiment.id;
  this._name = experiment.name;
  this._endDate = experiment.endDate;
  this._description = experiment.description;
}

PreviousExperimentAddon.prototype = Object.freeze({
  

  get appDisabled() {
    return true;
  },

  get blocklistState() {
    Ci.nsIBlocklistService.STATE_NOT_BLOCKED
  },

  get creator() {
    return new AddonManagerPrivate.AddonAuthor("");
  },

  get foreignInstall() {
    return false;
  },

  get id() {
    return this._id;
  },

  get isActive() {
    return false;
  },

  get isCompatible() {
    return true;
  },

  get isPlatformCompatible() {
    return true;
  },

  get name() {
    return this._name;
  },

  get pendingOperations() {
    return AddonManager.PENDING_NONE;
  },

  get permissions() {
    return 0;
  },

  get providesUpdatesSecurely() {
    return true;
  },

  get scope() {
    return AddonManager.SCOPE_PROFILE;
  },

  get type() {
    return "experiment";
  },

  get userDisabled() {
    return true;
  },

  get version() {
    return null;
  },

  

  

  get description() {
    return this._description;
  },

  get updateDate() {
    return new Date(this._endDate);
  },

  

  

  isCompatibleWith: function (appVersion, platformVersion) {
    return true;
  },

  findUpdates: function (listener, reason, appVersion, platformVersion) {
    AddonManagerPrivate.callNoUpdateListeners(this, listener, reason,
                                              appVersion, platformVersion);
  },

  

  



   get endDate() {
     return this._endDate;
   },

});
