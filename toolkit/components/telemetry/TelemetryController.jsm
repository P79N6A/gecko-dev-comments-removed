




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const myScope = this;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/debug.js", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/PromiseUtils.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/DeferredTask.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/TelemetryUtils.jsm", this);

const Utils = TelemetryUtils;

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetryController::";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_BRANCH_LOG = PREF_BRANCH + "log.";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_LOG_LEVEL = PREF_BRANCH_LOG + "level";
const PREF_LOG_DUMP = PREF_BRANCH_LOG + "dump";
const PREF_CACHED_CLIENTID = PREF_BRANCH + "cachedClientID";
const PREF_FHR_ENABLED = "datareporting.healthreport.service.enabled";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";
const PREF_SESSIONS_BRANCH = "datareporting.sessions.";
const PREF_UNIFIED = PREF_BRANCH + "unified";



const IS_UNIFIED_TELEMETRY = Preferences.get(PREF_UNIFIED, false);

const PING_FORMAT_VERSION = 4;


const TELEMETRY_DELAY = 60000;

const TELEMETRY_TEST_DELAY = 100;


const PING_TYPE_MAIN = "main";


const REASON_GATHER_PAYLOAD = "gather-payload";
const REASON_GATHER_SUBSESSION_PAYLOAD = "gather-subsession-payload";

XPCOMUtils.defineLazyModuleGetter(this, "ClientID",
                                  "resource://gre/modules/ClientID.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                   "@mozilla.org/base/telemetry;1",
                                   "nsITelemetry");
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStorage",
                                  "resource://gre/modules/TelemetryStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ThirdPartyCookieProbe",
                                  "resource://gre/modules/ThirdPartyCookieProbe.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryEnvironment",
                                  "resource://gre/modules/TelemetryEnvironment.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionRecorder",
                                  "resource://gre/modules/SessionRecorder.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryArchive",
                                  "resource://gre/modules/TelemetryArchive.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetrySession",
                                  "resource://gre/modules/TelemetrySession.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetrySend",
                                  "resource://gre/modules/TelemetrySend.jsm");





let gLogger = null;
let gLogAppenderDump = null;
function configureLogging() {
  if (!gLogger) {
    gLogger = Log.repository.getLogger(LOGGER_NAME);

    
    let consoleAppender = new Log.ConsoleAppender(new Log.BasicFormatter());
    gLogger.addAppender(consoleAppender);

    Preferences.observe(PREF_BRANCH_LOG, configureLogging);
  }

  
  gLogger.level = Log.Level[Preferences.get(PREF_LOG_LEVEL, "Warn")];

  
  let logDumping = Preferences.get(PREF_LOG_DUMP, false);
  if (logDumping != !!gLogAppenderDump) {
    if (logDumping) {
      gLogAppenderDump = new Log.DumpAppender(new Log.BasicFormatter());
      gLogger.addAppender(gLogAppenderDump);
    } else {
      gLogger.removeAppender(gLogAppenderDump);
      gLogAppenderDump = null;
    }
  }
}




let Policy = {
  now: () => new Date(),
}

this.EXPORTED_SYMBOLS = ["TelemetryController"];

this.TelemetryController = Object.freeze({
  Constants: Object.freeze({
    PREF_ENABLED: PREF_ENABLED,
    PREF_LOG_LEVEL: PREF_LOG_LEVEL,
    PREF_LOG_DUMP: PREF_LOG_DUMP,
    PREF_SERVER: PREF_SERVER,
  }),
  


  initLogging: function() {
    configureLogging();
  },
  


  reset: function() {
    Impl._clientID = null;
    TelemetryStorage.reset();
    TelemetrySend.reset();

    return this.setup();
  },
  


  setup: function() {
    return Impl.setupTelemetry(true);
  },

  


  setupContent: function() {
    return Impl.setupContentTelemetry(true);
  },

  


  observe: function (aSubject, aTopic, aData) {
    return Impl.observe(aSubject, aTopic, aData);
  },

  




















  submitExternalPing: function(aType, aPayload, aOptions = {}) {
    aOptions.addClientId = aOptions.addClientId || false;
    aOptions.addEnvironment = aOptions.addEnvironment || false;

    return Impl.submitExternalPing(aType, aPayload, aOptions);
  },

  





  getCurrentPingData: function(aSubsession = false) {
    return Impl.getCurrentPingData(aSubsession);
  },

  
















  addPendingPing: function(aType, aPayload, aOptions = {}) {
    let options = aOptions;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;
    options.overwrite = aOptions.overwrite || false;

    return Impl.addPendingPing(aType, aPayload, options);
  },

  





  checkAbortedSessionPing: function() {
    return Impl.checkAbortedSessionPing();
  },

  





  saveAbortedSessionPing: function(aPayload) {
    return Impl.saveAbortedSessionPing(aPayload);
  },

  




  removeAbortedSessionPing: function() {
    return Impl.removeAbortedSessionPing();
  },

  


















  savePing: function(aType, aPayload, aFilePath, aOptions = {}) {
    let options = aOptions;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;
    options.overwrite = aOptions.overwrite || false;

    return Impl.savePing(aType, aPayload, aFilePath, options);
  },

  




  get clientID() {
    return Impl.clientID;
  },

  


  get shutdown() {
    return Impl._shutdownBarrier.client;
  },

  



  getSessionRecorder: function() {
    return Impl._sessionRecorder;
  },

  




  promiseInitialized: function() {
    return Impl.promiseInitialized();
  },
});

let Impl = {
  _initialized: false,
  _initStarted: false, 
  _logger: null,
  _prevValues: {},
  
  
  _previousBuildID: undefined,
  _clientID: null,
  
  _delayedInitTask: null,
  
  _delayedInitTaskDeferred: null,

  
  _sessionRecorder: null,
  
  
  
  _shutdownBarrier: new AsyncShutdown.Barrier("TelemetryController: Waiting for clients."),
  
  _connectionsBarrier: new AsyncShutdown.Barrier("TelemetryController: Waiting for pending ping activity"),
  
  _testMode: false,

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    return this._logger;
  },

  


  _getApplicationSection: function() {
    
    
    let arch = null;
    try {
      arch = Services.sysinfo.get("arch");
    } catch (e) {
      this._log.trace("assemblePing - Unable to get system architecture.", e);
    }

    let updateChannel = null;
    try {
      updateChannel = UpdateChannel.get(false);
    } catch (e) {
      this._log.trace("assemblePing - Unable to get update channel.", e);
    }

    return {
      architecture: arch,
      buildId: Services.appinfo.appBuildID,
      name: Services.appinfo.name,
      version: Services.appinfo.version,
      vendor: Services.appinfo.vendor,
      platformVersion: Services.appinfo.platformVersion,
      xpcomAbi: Services.appinfo.XPCOMABI,
      channel: updateChannel,
    };
  },

  













  assemblePing: function assemblePing(aType, aPayload, aOptions = {}) {
    this._log.trace("assemblePing - Type " + aType + ", aOptions " + JSON.stringify(aOptions));

    
    
    
    let payload = Cu.cloneInto(aPayload, myScope);

    
    let pingData = {
      type: aType,
      id: Utils.generateUUID(),
      creationDate: (Policy.now()).toISOString(),
      version: PING_FORMAT_VERSION,
      application: this._getApplicationSection(),
      payload: payload,
    };

    if (aOptions.addClientId) {
      pingData.clientId = this._clientID;
    }

    if (aOptions.addEnvironment) {
      pingData.environment = aOptions.overrideEnvironment || TelemetryEnvironment.currentEnvironment;
    }

    return pingData;
  },

  



  _trackPendingPingTask: function (aPromise) {
    this._connectionsBarrier.client.addBlocker("Waiting for ping task", aPromise);
  },

  















  submitExternalPing: function send(aType, aPayload, aOptions) {
    this._log.trace("submitExternalPing - type: " + aType + ", aOptions: " + JSON.stringify(aOptions));

    
    const typeUuid = /^[a-z0-9][a-z0-9-]+[a-z0-9]$/i;
    if (!typeUuid.test(aType)) {
      this._log.error("submitExternalPing - invalid ping type: " + aType);
      let histogram = Telemetry.getKeyedHistogramById("TELEMETRY_INVALID_PING_TYPE_SUBMITTED");
      histogram.add(aType, 1);
      return Promise.reject(new Error("Invalid type string submitted."));
    }

    const pingData = this.assemblePing(aType, aPayload, aOptions);
    this._log.trace("submitExternalPing - ping assembled, id: " + pingData.id);

    
    let archivePromise = TelemetryArchive.promiseArchivePing(pingData)
      .catch(e => this._log.error("submitExternalPing - Failed to archive ping " + pingData.id, e));
    let p = [ archivePromise ];

    p.push(TelemetrySend.submitPing(pingData));

    let promise = Promise.all(p);
    this._trackPendingPingTask(promise);
    return promise.then(() => pingData.id);
  },

  















  addPendingPing: function addPendingPing(aType, aPayload, aOptions) {
    this._log.trace("addPendingPing - Type " + aType + ", aOptions " + JSON.stringify(aOptions));

    let pingData = this.assemblePing(aType, aPayload, aOptions);

    let savePromise = TelemetryStorage.savePendingPing(pingData);
    let archivePromise = TelemetryArchive.promiseArchivePing(pingData).catch(e => {
      this._log.error("addPendingPing - Failed to archive ping " + pingData.id, e);
    });

    
    let promises = [
      savePromise,
      archivePromise,
    ];
    return Promise.all(promises).then(() => pingData.id);
  },

  

















  savePing: function savePing(aType, aPayload, aFilePath, aOptions) {
    this._log.trace("savePing - Type " + aType + ", File Path " + aFilePath +
                    ", aOptions " + JSON.stringify(aOptions));
    let pingData = this.assemblePing(aType, aPayload, aOptions);
    return TelemetryStorage.savePingToFile(pingData, aFilePath, aOptions.overwrite)
                        .then(() => pingData.id);
  },

  




  checkAbortedSessionPing: Task.async(function*() {
    let ping = yield TelemetryStorage.loadAbortedSessionPing();
    this._log.trace("checkAbortedSessionPing - found aborted-session ping: " + !!ping);
    if (!ping) {
      return;
    }

    try {
      yield TelemetryStorage.addPendingPing(ping);
      yield TelemetryArchive.promiseArchivePing(ping);
    } catch (e) {
      this._log.error("checkAbortedSessionPing - Unable to add the pending ping", e);
    } finally {
      yield TelemetryStorage.removeAbortedSessionPing();
    }
  }),

  





  saveAbortedSessionPing: function(aPayload) {
    this._log.trace("saveAbortedSessionPing");
    const options = {addClientId: true, addEnvironment: true};
    const pingData = this.assemblePing(PING_TYPE_MAIN, aPayload, options);
    return TelemetryStorage.saveAbortedSessionPing(pingData);
  },

  removeAbortedSessionPing: function() {
    return TelemetryStorage.removeAbortedSessionPing();
  },

  




  enableTelemetryRecording: function enableTelemetryRecording() {
    const enabled = Preferences.get(PREF_ENABLED, false);

    
    Telemetry.canRecordBase = enabled || IS_UNIFIED_TELEMETRY;

#ifdef MOZILLA_OFFICIAL
    if (!Telemetry.isOfficialTelemetry && !this._testMode) {
      
      
      
      Telemetry.canRecordExtended = false;
      this._log.config("enableTelemetryRecording - Can't send data, disabling extended Telemetry recording.");
    }
#endif

    if (!enabled || !Telemetry.canRecordBase) {
      
      
      Telemetry.canRecordExtended = false;
      this._log.config("enableTelemetryRecording - Disabling extended Telemetry recording.");
    }

    return Telemetry.canRecordBase;
  },

  










  setupTelemetry: function setupTelemetry(testing) {
    this._initStarted = true;
    this._testMode = testing;

    this._log.trace("setupTelemetry");

    if (this._delayedInitTask) {
      this._log.error("setupTelemetry - init task already running");
      return this._delayedInitTaskDeferred.promise;
    }

    if (this._initialized && !this._testMode) {
      this._log.error("setupTelemetry - already initialized");
      return Promise.resolve();
    }

    
    
    
    if (!this._sessionRecorder &&
        (Preferences.get(PREF_FHR_ENABLED, true) || IS_UNIFIED_TELEMETRY)) {
      this._sessionRecorder = new SessionRecorder(PREF_SESSIONS_BRANCH);
      this._sessionRecorder.onStartup();
    }

    if (!this.enableTelemetryRecording()) {
      this._log.config("setupChromeProcess - Telemetry recording is disabled, skipping Chrome process setup.");
      return Promise.resolve();
    }

    
    
    
    
    this._clientID = Preferences.get(PREF_CACHED_CLIENTID, null);

    
    
    
    this._delayedInitTaskDeferred = Promise.defer();
    this._delayedInitTask = new DeferredTask(function* () {
      try {
        
        this._initialized = true;

        yield TelemetrySend.setup(this._testMode);

        
        this._clientID = yield ClientID.getClientID();
        Preferences.set(PREF_CACHED_CLIENTID, this._clientID);

        
        
        TelemetryStorage.runCleanPingArchiveTask();

        Telemetry.asyncFetchTelemetryData(function () {});
        this._delayedInitTaskDeferred.resolve();
      } catch (e) {
        this._delayedInitTaskDeferred.reject(e);
      } finally {
        this._delayedInitTask = null;
      }
    }.bind(this), this._testMode ? TELEMETRY_TEST_DELAY : TELEMETRY_DELAY);

    AsyncShutdown.sendTelemetry.addBlocker("TelemetryController: shutting down",
                                           () => this.shutdown(),
                                           () => this._getState());

    this._delayedInitTask.arm();
    return this._delayedInitTaskDeferred.promise;
  },

  



  setupContentTelemetry: function (testing = false) {
    this._testMode = testing;

    
    
    if (!this.enableTelemetryRecording()) {
      this._log.trace("setupContentTelemetry - Content process recording disabled.");
      return;
    }
  },

  
  _cleanupOnShutdown: Task.async(function*() {
    if (!this._initialized) {
      return;
    }

    Preferences.ignore(PREF_BRANCH_LOG, configureLogging);

    
    try {
      
      yield this._shutdownBarrier.wait();

      
      yield TelemetrySend.shutdown();

      
      yield this._connectionsBarrier.wait();

      
      yield TelemetryStorage.shutdown();
    } finally {
      
      this._initialized = false;
      this._initStarted = false;
    }
  }),

  shutdown: function() {
    this._log.trace("shutdown");

    
    
    
    
    
    

    
    if (!this._initStarted) {
      return Promise.resolve();
    }

    
    if (!this._delayedInitTask) {
      
      return this._cleanupOnShutdown();
    }

    
    return this._delayedInitTask.finalize().then(() => this._cleanupOnShutdown());
  },

  


  observe: function (aSubject, aTopic, aData) {
    
    if (aTopic == "profile-after-change" || aTopic == "app-startup") {
      
      
      configureLogging();
    }

    this._log.trace("observe - " + aTopic + " notified.");

    switch (aTopic) {
    case "profile-after-change":
      
      return this.setupTelemetry();
    case "app-startup":
      
      return this.setupContentTelemetry();
      break;
    }
  },

  get clientID() {
    return this._clientID;
  },

  


  _getState: function() {
    return {
      initialized: this._initialized,
      initStarted: this._initStarted,
      haveDelayedInitTask: !!this._delayedInitTask,
      shutdownBarrier: this._shutdownBarrier.state,
      connectionsBarrier: this._connectionsBarrier.state,
    };
  },

  




  promiseInitialized: function() {
    return this._delayedInitTaskDeferred.promise;
  },

  getCurrentPingData: function(aSubsession) {
    this._log.trace("getCurrentPingData - subsession: " + aSubsession)

    const reason = aSubsession ? REASON_GATHER_SUBSESSION_PAYLOAD : REASON_GATHER_PAYLOAD;
    const type = PING_TYPE_MAIN;
    const payload = TelemetrySession.getPayload(reason);
    const options = { addClientId: true, addEnvironment: true };
    const ping = this.assemblePing(type, payload, options);

    return ping;
  },
};
