



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
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");
Cu.import("resource://gre/modules/Metrics.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryPing",
                                  "resource://gre/modules/TelemetryPing.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");


XPCOMUtils.defineLazyGetter(this, "CertUtils",
  function() {
    var mod = {};
    Cu.import("resource://gre/modules/CertUtils.jsm", mod);
    return mod;
  });

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "gCrashReporter",
                                   "@mozilla.org/xre/app-info;1",
                                   "nsICrashReporter");
#endif

const FILE_CACHE                = "experiments.json";
const OBSERVER_TOPIC            = "experiments-changed";
const MANIFEST_VERSION          = 1;
const CACHE_VERSION             = 1;

const KEEP_HISTORY_N_DAYS       = 180;
const MIN_EXPERIMENT_ACTIVE_SECONDS = 60;

const PREF_BRANCH               = "experiments.";
const PREF_ENABLED              = "enabled"; 
const PREF_LOGGING              = "logging";
const PREF_LOGGING_LEVEL        = PREF_LOGGING + ".level"; 
const PREF_LOGGING_DUMP         = PREF_LOGGING + ".dump"; 
const PREF_MANIFEST_URI         = "manifest.uri"; 
const PREF_MANIFEST_CHECKCERT   = "manifest.cert.checkAttributes"; 
const PREF_MANIFEST_REQUIREBUILTIN = "manifest.cert.requireBuiltin"; 
const PREF_FORCE_SAMPLE = "force-sample-value"; 

const PREF_HEALTHREPORT_ENABLED = "datareporting.healthreport.service.enabled";

const PREF_BRANCH_TELEMETRY     = "toolkit.telemetry.";
const PREF_TELEMETRY_ENABLED    = "enabled";

const TELEMETRY_LOG = {
  
  ACTIVATION_KEY: "EXPERIMENT_ACTIVATION",
  ACTIVATION: {
    ACTIVATED: "ACTIVATED",             
    INSTALL_FAILURE: "INSTALL_FAILURE", 
    REJECTED: "REJECTED",               
                                        
  },

  
  TERMINATION_KEY: "EXPERIMENT_TERMINATION",
  TERMINATION: {
    USERDISABLED: "USERDISABLED", 
    FROM_API: "FROM_API",         
    EXPIRED: "EXPIRED",           
    RECHECK: "RECHECK",           
                                  
  },
};

const gPrefs = new Preferences(PREF_BRANCH);
const gPrefsTelemetry = new Preferences(PREF_BRANCH_TELEMETRY);
let gExperimentsEnabled = false;
let gExperiments = null;
let gLogAppenderDump = null;
let gPolicyCounter = 0;
let gExperimentsCounter = 0;
let gExperimentEntryCounter = 0;



let gActiveInstallURLs = new Set();

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
  AddonManager.getAddonsByTypes(["experiment"],
                                addons => deferred.resolve(addons));
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
      throw new Task.Result(payload);
    });
  },

  telemetryPayload: function () {
    return TelemetryPing.getPayload();
  },
};





Experiments.Experiments = function (policy=new Experiments.Policy()) {
  this._log = Log.repository.getLoggerWithMessagePrefix(
    "Browser.Experiments.Experiments",
    "Experiments #" + gExperimentsCounter++ + "::");

  this._policy = policy;

  
  
  
  
  
  this._experiments = null;
  this._refresh = false;
  this._terminateReason = null; 
  this._dirty = false;

  
  this._loadTask = null;

  
  this._pendingUninstall = null;

  
  
  
  
  this._mainTask = null;

  
  this._timer = null;

  this._shutdown = false;

  this.init();
};

Experiments.Experiments.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback, Ci.nsIObserver]),

  init: function () {
    configureLogging();

    gExperimentsEnabled = gPrefs.get(PREF_ENABLED, false);
    this._log.trace("enabled=" + gExperimentsEnabled + ", " + this.enabled);

    gPrefs.observe(PREF_LOGGING, configureLogging);
    gPrefs.observe(PREF_MANIFEST_URI, this.updateManifest, this);
    gPrefs.observe(PREF_ENABLED, this._toggleExperimentsEnabled, this);

    gPrefsTelemetry.observe(PREF_TELEMETRY_ENABLED, this._telemetryStatusChanged, this);

    AsyncShutdown.profileBeforeChange.addBlocker("Experiments.jsm shutdown",
      this.uninit.bind(this));

    this._startWatchingAddons();

    this._loadTask = Task.spawn(this._loadFromCache.bind(this));
    this._loadTask.then(
      () => {
        this._log.trace("_loadTask finished ok");
        this._loadTask = null;
        this._run();
      },
      (e) => {
        this._log.error("_loadFromCache caught error: " + e);
      }
    );
  },

  



  uninit: function () {
    if (!this._shutdown) {
      this._stopWatchingAddons();

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
      return this._mainTask;
    }
    return Promise.resolve();
  },

  _startWatchingAddons: function () {
    AddonManager.addAddonListener(this);
    AddonManager.addInstallListener(this);
  },

  _stopWatchingAddons: function () {
    AddonManager.removeInstallListener(this);
    AddonManager.removeAddonListener(this);
  },

  


  _checkForShutdown: function() {
    if (this._shutdown) {
      throw Error("uninit() already called");
    }
  },

  


  get enabled() {
    return gExperimentsEnabled;
  },

  


  set enabled(enabled) {
    this._log.trace("set enabled(" + enabled + ")");
    gPrefs.set(PREF_ENABLED, enabled);
  },

  _toggleExperimentsEnabled: function (enabled) {
    this._log.trace("_toggleExperimentsEnabled(" + enabled + ")");
    let wasEnabled = gExperimentsEnabled;
    gExperimentsEnabled = enabled && telemetryEnabled();

    if (wasEnabled == gExperimentsEnabled) {
      return;
    }

    if (gExperimentsEnabled) {
      this.updateManifest();
    } else {
      this.disableExperiment();
      if (this._timer) {
        this._timer.clear();
      }
    }
  },

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
        });
      }

      
      list.sort((a, b) => b.endDate - a.endDate);
      return list;
    }.bind(this));
  },

  


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
      this._mainTask = Task.spawn(this._main.bind(this));
      this._mainTask.then(
        () => {
          this._log.trace("_main finished, scheduling next run");
          this._mainTask = null;
          this._scheduleNextRun();
        },
        (e) => {
          this._log.error("_main caught error: " + e);
          this._mainTask = null;
        }
      );
    }
    return this._mainTask;
  },

  _main: function*() {
    do {
      this._log.trace("_main iteration");
      yield this._loadTask;
      if (this._refresh) {
        yield this._loadManifest();
      }
      yield this._evaluateExperiments();
      if (this._dirty) {
        yield this._saveToCache();
      }
      
      
    }
    while (this._refresh || this._terminateReason);
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

  

  onDisabled: function (addon) {
    this._log.trace("onDisabled() - addon id: " + addon.id);
    if (addon.id == this._pendingUninstall) {
      return;
    }
    let activeExperiment = this._getActiveExperiment();
    if (!activeExperiment || activeExperiment._addonId != addon.id) {
      return;
    }
    this.disableExperiment();
  },

  onUninstalled: function (addon) {
    this._log.trace("onUninstalled() - addon id: " + addon.id);
    if (addon.id == this._pendingUninstall) {
      this._log.trace("matches pending uninstall");
      return;
    }
    let activeExperiment = this._getActiveExperiment();
    if (!activeExperiment || activeExperiment._addonId != addon.id) {
      return;
    }
    this.disableExperiment();
  },

  onInstallStarted: function (install) {
    if (install.addon.type != "experiment") {
      return;
    }

    
    

    
    
    
    
    
    
    
    
    
    

    if (this._trackedAddonIds.has(install.addon.id)) {
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

    let deferred = Promise.defer();

    let log = this._log;
    xhr.onerror = function (e) {
      log.error("httpGetRequest::onError() - Error making request to " + url + ": " + e.error);
      deferred.reject(new Error("Experiments - XHR error for " + url + " - " + e.error));
    };

    xhr.onload = function (event) {
      if (xhr.status !== 200 && xhr.state !== 0) {
        log.error("httpGetRequest::onLoad() - Request to " + url + " returned status " + xhr.status);
        deferred.reject(new Error("Experiments - XHR status for " + url + " is " + xhr.status));
        return;
      }

      let certs = null;
      if (gPrefs.get(PREF_MANIFEST_CHECKCERT, true)) {
        certs = CertUtils.readCertPrefs(PREF_BRANCH + "manifest.certs.");
      }
      try {
        let allowNonBuiltin = !gPrefs.get(PREF_MANIFEST_REQUIREBUILTIN, true);
        CertUtils.checkCert(xhr.channel, allowNonBuiltin, certs);
      }
      catch (e) {
        log.error("manifest fetch failed certificate checks", [e]);
        deferred.reject(new Error("Experiments - manifest fetch failed certificate checks: " + e));
        return;
      }

      deferred.resolve(xhr.responseText);
    };

    if (xhr.channel instanceof Ci.nsISupportsPriority) {
      xhr.channel.priority = Ci.nsISupportsPriority.PRIORITY_LOWEST;
    }

    xhr.send(null);
    return deferred.promise;
  },

  


  get _cacheFilePath() {
    return OS.Path.join(OS.Constants.Path.profileDir, FILE_CACHE);
  },

  


  _saveToCache: function* () {
    this._log.trace("_saveToCache");
    let path = this._cacheFilePath;
    let textData = JSON.stringify({
      version: CACHE_VERSION,
      data: [e[1].toJSON() for (e of this._experiments.entries())],
    });

    let encoder = new TextEncoder();
    let data = encoder.encode(textData);
    let options = { tmpPath: path + ".tmp", compression: "lz4" };
    yield OS.File.writeAtomic(path, data, options);
    this._dirty = false;
    this._log.debug("_saveToCache saved to " + path);
  },

  


  _loadFromCache: function*() {
    this._log.trace("_loadFromCache");
    let path = this._cacheFilePath;
    try {
      let result = yield loadJSONAsync(path, { compression: "lz4" });
      this._populateFromCache(result);
    } catch (e if e instanceof OS.File.Error && e.becauseNoSuchFile) {
      
      this._experiments = new Map();
    }
  },

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
      if (experiments.has(id) || !entry.startDate || entry.shouldDiscard()) {
        this._log.trace("updateExperiments() - discarding entry for " + id);
        continue;
      }

      experiments.set(id, entry);
    }

    this._experiments = experiments;
    this._dirty = true;
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

  





  disableExperiment: function (userDisabled=true) {
    this._log.trace("disableExperiment()");

    this._terminateReason = userDisabled ? TELEMETRY_LOG.TERMINATION.USERDISABLED : TELEMETRY_LOG.TERMINATION.FROM_API;
    return this._run();
  },

  


  get _trackedAddonIds() {
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

    if (activeExperiment) {
      this._pendingUninstall = activeExperiment._addonId;
      try {
        let wasStopped;
        if (this._terminateReason) {
          yield activeExperiment.stop(this._terminateReason);
          wasStopped = true;
        } else {
          wasStopped = yield activeExperiment.maybeStop();
        }
        if (wasStopped) {
          this._dirty = true;
          this._log.debug("evaluateExperiments() - stopped experiment "
                        + activeExperiment.id);
          activeExperiment = null;
          activeChanged = true;
        } else if (activeExperiment.needsUpdate) {
          this._log.debug("evaluateExperiments() - updating experiment "
                        + activeExperiment.id);
          try {
            yield activeExperiment.stop();
            yield activeExperiment.start();
          } catch (e) {
            this._log.error(e);
            
            activeExperiment = null;
          }
          this._dirty = true;
          activeChanged = true;
        }
      } finally {
        this._pendingUninstall = null;
      }
    }
    this._terminateReason = null;

    if (!activeExperiment) {
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
          TelemetryLog.log(TELEMETRY_LOG.ACTIVATION_KEY, data);
        }

        if (applicable) {
          this._log.debug("evaluateExperiments() - activating experiment " + id);
          try {
            yield experiment.start();
            activeChanged = true;
            activeExperiment = experiment;
            this._dirty = true;
            break;
          } catch (e) {
            
          }
        }
      }
    }

    if (activeChanged) {
      Services.obs.notifyObservers(null, OBSERVER_TOPIC, null);
    }

#ifdef MOZ_CRASHREPORTER
    if (activeExperiment) {
      gCrashReporter.annotateCrashReport("ActiveExperiment", activeExperiment.id);
    }
#endif
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
  ]),

  DATE_KEYS: new Set([
    "_startDate",
    "_endDate",
  ]),

  




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
        condition: () => !data.maxStartTime || now <= data.maxStartTime },
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

      let sandbox = Cu.Sandbox(nullprincipal);
      let context = {};
      context.healthReportPayload = yield this._policy.healthReportPayload();
      context.telemetryPayload    = yield this._policy.telemetryPayload();

      try {
        Cu.evalInSandbox(jsfilter, sandbox);
      } catch (e) {
        this._log.error("runFilterFunction() - failed to eval jsfilter: " + e.message);
        throw ["jsfilter-evalfailed"];
      }

      
      
      sandbox._hr = JSON.stringify(yield this._policy.healthReportPayload());
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

  



  start: function () {
    this._log.trace("start() for " + this.id);

    return Task.spawn(function* ExperimentEntry_start_task() {
      let addons = yield installedExperimentAddons();
      if (addons.length > 0) {
        this._log.error("start() - there are already "
                        + addons.length + " experiment addons installed");
        yield uninstallAddons(addons);
      }

      yield this._installAddon();
    }.bind(this));
  },

  
  _installAddon: function* () {
    let deferred = Promise.defer();

    let install = yield addonInstallForURL(this._manifestData.xpiURL,
                                           this._manifestData.xpiHash);
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

        
        install.addon.userDisabled = false;
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

        deferred.resolve();
      },
    };

    ["onDownloadCancelled", "onDownloadFailed", "onInstallCancelled", "onInstallFailed"]
      .forEach(what => {
        listener[what] = install => failureHandler(install, what)
      });

    install.addListener(listener);
    install.install();

    return deferred.promise;
  },

  






  stop: function (terminationKind, terminationReason) {
    this._log.trace("stop() - id=" + this.id + ", terminationKind=" + terminationKind);
    if (!this._enabled) {
      this._log.warning("stop() - experiment not enabled: " + id);
      return Promise.reject();
    }

    this._enabled = false;
    let deferred = Promise.defer();
    let updateDates = () => {
      let now = this._policy.now();
      this._lastChangedDate = now;
      this._endDate = now;
    };

    AddonManager.getAddonByID(this._addonId, addon => {
      if (!addon) {
        let message = "could not get Addon for " + this.id;
        this._log.warn("stop() - " + message);
        updateDates();
        deferred.resolve();
        return;
      }

      updateDates();
      this._logTermination(terminationKind, terminationReason);
      deferred.resolve(uninstallAddons([addon]));
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

  




  maybeStop: function () {
    this._log.trace("maybeStop()");

    return Task.spawn(function ExperimentEntry_maybeStop_task() {
      let result = yield this._shouldStop();
      if (result.shouldStop) {
        let expireReasons = ["endTime", "maxActiveSeconds"];
        if (expireReasons.indexOf(result.reason[0]) != -1) {
          yield this.stop(TELEMETRY_LOG.TERMINATION.EXPIRED);
        } else {
          yield this.stop(TELEMETRY_LOG.TERMINATION.RECHECK, result.reason);
        }
      }

      throw new Task.Result(result.shouldStop);
    }.bind(this));
  },

  _shouldStop: function () {
    let data = this._manifestData;
    let now = this._policy.now() / 1000; 
    let maxActiveSec = data.maxActiveSeconds || 0;

    if (!this._enabled) {
      return Promise.resolve({shouldStop: false});
    }

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

const FIELD_DAILY_LAST_TEXT = {type: Metrics.Storage.FIELD_DAILY_LAST_TEXT};

ExperimentsLastActiveMeasurement1.prototype = Object.freeze({
  __proto__: Metrics.Measurement.prototype,

  name: "info",
  version: 1,

  fields: {
    lastActive: FIELD_DAILY_LAST_TEXT,
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
  ],

  _OBSERVERS: [
    OBSERVER_TOPIC,
  ],

  postInit: function () {
    this._experiments = Experiments.instance();

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
      case OBSERVER_TOPIC:
        this.recordLastActiveExperiment();
        break;
    }
  },

  collectDailyData: function () {
    return this.recordLastActiveExperiment();
  },

  recordLastActiveExperiment: function () {
    let m = this.getMeasurement(ExperimentsLastActiveMeasurement1.prototype.name,
                                ExperimentsLastActiveMeasurement1.prototype.version);

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
      }.bind(this));
    });
  },
});
