




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

const PING_FORMAT_VERSION = 4;


const TELEMETRY_DELAY = 60000;

const TELEMETRY_TEST_DELAY = 100;

const DEFAULT_RETENTION_DAYS = 14;

const PING_SUBMIT_TIMEOUT_MS = 2 * 60 * 1000;

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

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}




function isNewPingFormat(aPing) {
  return ("id" in aPing) && ("application" in aPing) &&
         ("version" in aPing) && (aPing.version >= 2);
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
    return this.setup();
  },
  


  setup: function() {
    return Impl.setupTelemetry(true);
  },

  


  observe: function (aSubject, aTopic, aData) {
    return Impl.observe(aSubject, aTopic, aData);
  },

  


  setServer: function(aServer) {
    return Impl.setServer(aServer);
  },

  








  addPendingPingFromFile: function(aPingPath, aRemoveOriginal) {
    return Impl.addPendingPingFromFile(aPingPath, aRemoveOriginal);
  },

  















  submitExternalPing: function(aType, aPayload, aOptions = {}) {
    aOptions.addClientId = aOptions.addClientId || false;
    aOptions.addEnvironment = aOptions.addEnvironment || false;

    return Impl.submitExternalPing(aType, aPayload, aOptions);
  },

  














  savePendingPings: function(aType, aPayload, aOptions = {}) {
    let options = aOptions;
    options.retentionDays = aOptions.retentionDays || DEFAULT_RETENTION_DAYS;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;

    return Impl.savePendingPings(aType, aPayload, options);
  },

  


















  addPendingPing: function(aType, aPayload, aOptions = {}) {
    let options = aOptions;
    options.retentionDays = aOptions.retentionDays || DEFAULT_RETENTION_DAYS;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;
    options.overwrite = aOptions.overwrite || false;

    return Impl.addPendingPing(aType, aPayload, options);
  },

  




















  savePing: function(aType, aPayload, aFilePath, aOptions = {}) {
    let options = aOptions;
    options.retentionDays = aOptions.retentionDays || DEFAULT_RETENTION_DAYS;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;
    options.overwrite = aOptions.overwrite || false;

    return Impl.savePing(aType, aPayload, aFilePath, options);
  },

  




  sendPersistedPings: function() {
    return Impl.sendPersistedPings();
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
  _log: null,
  _prevValues: {},
  
  
  _previousBuildID: undefined,
  _clientID: null,
  
  _delayedInitTask: null,
  
  _delayedInitTaskDeferred: null,
  
  _sessionRecorder: null,
  
  
  
  _shutdownBarrier: new AsyncShutdown.Barrier("TelemetryController: Waiting for clients."),
  
  _connectionsBarrier: new AsyncShutdown.Barrier("TelemetryController: Waiting for pending ping activity"),
  
  _testMode: false,

  
  _pendingPingRequests: new Map(),

  


  _getApplicationSection: function() {
    
    
    let arch = null;
    try {
      arch = Services.sysinfo.get("arch");
    } catch (e) {
      this._log.trace("assemblePing - Unable to get system architecture.", e);
    }

    let updateChannel = null;
    try {
      updateChannel = UpdateChannel.get();
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
    this._log.trace("assemblePing - Type " + aType + ", Server " + this._server +
                    ", aOptions " + JSON.stringify(aOptions));

    
    
    
    let payload = Cu.cloneInto(aPayload, myScope);

    
    let pingData = {
      type: aType,
      id: generateUUID(),
      creationDate: (Policy.now()).toISOString(),
      version: PING_FORMAT_VERSION,
      application: this._getApplicationSection(),
      payload: aPayload,
    };

    if (aOptions.addClientId) {
      pingData.clientId = this._clientID;
    }

    if (aOptions.addEnvironment) {
      pingData.environment = aOptions.overrideEnvironment || TelemetryEnvironment.currentEnvironment;
    }

    return pingData;
  },

  popPayloads: function popPayloads() {
    this._log.trace("popPayloads");
    function payloadIter() {
      let iterator = TelemetryStorage.popPendingPings();
      for (let data of iterator) {
        yield data;
      }
    }

    let payloadIterWithThis = payloadIter.bind(this);
    return { __iterator__: payloadIterWithThis };
  },

  


  setServer: function (aServer) {
    this._server = aServer;
  },

  



  _trackPendingPingTask: function (aPromise) {
    this._connectionsBarrier.client.addBlocker("Waiting for ping task", aPromise);
  },

  








  addPendingPingFromFile: function(aPingPath, aRemoveOriginal) {
    return TelemetryStorage.addPendingPing(aPingPath).then(() => {
        if (aRemoveOriginal) {
          return OS.File.remove(aPingPath);
        }
      }, error => this._log.error("addPendingPingFromFile - Unable to add the pending ping", error));
  },

  















  submitExternalPing: function send(aType, aPayload, aOptions) {
    this._log.trace("submitExternalPing - Type " + aType + ", Server " + this._server +
                    ", aOptions " + JSON.stringify(aOptions));

    let pingData = this.assemblePing(aType, aPayload, aOptions);
    
    let archivePromise = TelemetryArchive.promiseArchivePing(pingData)
      .catch(e => this._log.error("submitExternalPing - Failed to archive ping " + pingData.id, e));

    
    let p = [
      archivePromise,
      
      this.doPing(pingData, false)
          .catch(() => TelemetryStorage.savePing(pingData, true)),
      this.sendPersistedPings(),
    ];

    let promise = Promise.all(p);
    this._trackPendingPingTask(promise);
    return promise.then(() => pingData.id);
  },

  




  sendPersistedPings: function sendPersistedPings() {
    this._log.trace("sendPersistedPings - Can send: " + this._canSend());
    if (!this._canSend()) {
      this._log.trace("sendPersistedPings - Telemetry is not allowed to send pings.");
      return Promise.resolve();
    }

    let pingsIterator = Iterator(this.popPayloads());
    let p = [for (data of pingsIterator) this.doPing(data, true).catch((e) => {
      this._log.error("sendPersistedPings - doPing rejected", e);
    })];

    let promise = Promise.all(p);
    this._trackPendingPingTask(promise);
    return promise;
  },

  















  savePendingPings: function savePendingPings(aType, aPayload, aOptions) {
    this._log.trace("savePendingPings - Type " + aType + ", Server " + this._server +
                    ", aOptions " + JSON.stringify(aOptions));

    let pingData = this.assemblePing(aType, aPayload, aOptions);
    return TelemetryStorage.savePendingPings(pingData);
  },

  

















  addPendingPing: function addPendingPing(aType, aPayload, aOptions) {
    this._log.trace("addPendingPing - Type " + aType + ", Server " + this._server +
                    ", aOptions " + JSON.stringify(aOptions));

    let pingData = this.assemblePing(aType, aPayload, aOptions);

    let savePromise = TelemetryStorage.savePing(pingData, aOptions.overwrite);
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
    this._log.trace("savePing - Type " + aType + ", Server " + this._server +
                    ", File Path " + aFilePath + ", aOptions " + JSON.stringify(aOptions));
    let pingData = this.assemblePing(aType, aPayload, aOptions);
    return TelemetryStorage.savePingToFile(pingData, aFilePath, aOptions.overwrite)
                        .then(() => pingData.id);
  },

  onPingRequestFinished: function(success, startTime, ping, isPersisted) {
    this._log.trace("onPingRequestFinished - success: " + success + ", persisted: " + isPersisted);

    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    hsuccess.add(success);
    hping.add(new Date() - startTime);

    if (success && isPersisted) {
      return TelemetryStorage.cleanupPingFile(ping);
    } else {
      return Promise.resolve();
    }
  },

  submissionPath: function submissionPath(ping) {
    
    let pathComponents;
    if (isNewPingFormat(ping)) {
      
      
      let app = ping.application;
      pathComponents = [
        ping.id, ping.type, app.name, app.version, app.channel, app.buildId
      ];
    } else {
      
      if (!("slug" in ping)) {
        
        ping.slug = generateUUID();
      }

      
      let payload = ("payload" in ping) ? ping.payload : null;
      if (payload && ("info" in payload)) {
        let info = ping.payload.info;
        pathComponents = [ ping.slug, info.reason, info.appName, info.appVersion,
                           info.appUpdateChannel, info.appBuildID ];
      } else {
        
        pathComponents = [ ping.slug ];
      }
    }

    let slug = pathComponents.join("/");
    return "/submit/telemetry/" + slug;
  },

  doPing: function doPing(ping, isPersisted) {
    if (!this._canSend()) {
      
      this._log.trace("doPing - Sending is disabled.");
      return Promise.resolve();
    }

    this._log.trace("doPing - Server " + this._server + ", Persisted " + isPersisted);
    const isNewPing = isNewPingFormat(ping);
    const version = isNewPing ? PING_FORMAT_VERSION : 1;
    const url = this._server + this.submissionPath(ping) + "?v=" + version;

    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.timeout = PING_SUBMIT_TIMEOUT_MS;

    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json; charset=UTF-8");

    this._pendingPingRequests.set(url, request);

    let startTime = new Date();
    let deferred = PromiseUtils.defer();

    let onRequestFinished = (success, event) => {
      let onCompletion = () => {
        if (success) {
          deferred.resolve();
        } else {
          deferred.reject(event);
        }
      };

      this._pendingPingRequests.delete(url);
      this.onPingRequestFinished(success, startTime, ping, isPersisted)
        .then(() => onCompletion(),
              (error) => {
                this._log.error("doPing - request success: " + success + ", error" + error);
                onCompletion();
              });
    };

    let errorhandler = (event) => {
      this._log.error("doPing - error making request to " + url + ": " + event.type);
      onRequestFinished(false, event);
    };
    request.onerror = errorhandler;
    request.ontimeout = errorhandler;
    request.onabort = errorhandler;

    request.onload = (event) => {
      let status = request.status;
      let statusClass = status - (status % 100);
      let success = false;

      if (statusClass === 200) {
        
        this._log.info("doPing - successfully loaded, status: " + status);
        success = true;
      } else if (statusClass === 400) {
        
        this._log.error("doPing - error submitting to " + url + ", status: " + status
                        + " - ping request broken?");
        
        
        success = true;
      } else if (statusClass === 500) {
        
        this._log.error("doPing - error submitting to " + url + ", status: " + status
                        + " - server error, should retry later");
      } else {
        
        this._log.error("doPing - error submitting to " + url + ", status: " + status
                        + ", type: " + event.type);
      }

      onRequestFinished(success, event);
    };

    
    let networkPayload = isNewPing ? ping : ping.payload;
    request.setRequestHeader("Content-Encoding", "gzip");
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let utf8Payload = converter.ConvertFromUnicode(JSON.stringify(networkPayload));
    utf8Payload += converter.Finish();
    let payloadStream = Cc["@mozilla.org/io/string-input-stream;1"]
                        .createInstance(Ci.nsIStringInputStream);
    payloadStream.data = this.gzipCompressString(utf8Payload);
    request.send(payloadStream);

    return deferred.promise;
  },

  gzipCompressString: function gzipCompressString(string) {
    let observer = {
      buffer: "",
      onStreamComplete: function(loader, context, status, length, result) {
        this.buffer = String.fromCharCode.apply(this, result);
      }
    };

    let scs = Cc["@mozilla.org/streamConverters;1"]
              .getService(Ci.nsIStreamConverterService);
    let listener = Cc["@mozilla.org/network/stream-loader;1"]
                  .createInstance(Ci.nsIStreamLoader);
    listener.init(observer);
    let converter = scs.asyncConvertData("uncompressed", "gzip",
                                         listener, null);
    let stringStream = Cc["@mozilla.org/io/string-input-stream;1"]
                       .createInstance(Ci.nsIStringInputStream);
    stringStream.data = string;
    converter.onStartRequest(null, null);
    converter.onDataAvailable(null, null, stringStream, 0, string.length);
    converter.onStopRequest(null, null, null);
    return observer.buffer;
  },

  




  enableTelemetryRecording: function enableTelemetryRecording() {
    
#if !defined(MOZ_WIDGET_ANDROID)
    Telemetry.canRecordBase = Preferences.get(PREF_FHR_ENABLED, false);
#else
    
    Telemetry.canRecordBase = true;
#endif

#ifdef MOZILLA_OFFICIAL
    if (!Telemetry.isOfficialTelemetry && !this._testMode) {
      
      
      
      Telemetry.canRecordExtended = false;
      this._log.config("enableTelemetryRecording - Can't send data, disabling extended Telemetry recording.");
    }
#endif

    let enabled = Preferences.get(PREF_ENABLED, false);
    this._server = Preferences.get(PREF_SERVER, undefined);
    if (!enabled || !Telemetry.canRecordBase) {
      
      
      Telemetry.canRecordExtended = false;
      this._log.config("enableTelemetryRecording - Disabling extended Telemetry recording.");
    }

    return Telemetry.canRecordBase;
  },

  









  setupTelemetry: function setupTelemetry(testing) {
    this._initStarted = true;
    this._testMode = testing;
    if (this._testMode && !this._log) {
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("setupTelemetry");

    if (this._delayedInitTask) {
      this._log.error("setupTelemetry - init task already running");
      return this._delayedInitTaskDeferred.promise;
    }

    if (this._initialized && !this._testMode) {
      this._log.error("setupTelemetry - already initialized");
      return Promise.resolve();
    }

    
    
    
    if (!this._sessionRecorder && Preferences.get(PREF_FHR_ENABLED, true)) {
      this._sessionRecorder = new SessionRecorder(PREF_SESSIONS_BRANCH);
      this._sessionRecorder.onStartup();
    }

    
    this._thirdPartyCookies = new ThirdPartyCookieProbe();
    this._thirdPartyCookies.init();

    if (!this.enableTelemetryRecording()) {
      this._log.config("setupChromeProcess - Telemetry recording is disabled, skipping Chrome process setup.");
      return Promise.resolve();
    }

    
    
    
    
    this._clientID = Preferences.get(PREF_CACHED_CLIENTID, null);

    
    
    
    this._delayedInitTaskDeferred = Promise.defer();
    this._delayedInitTask = new DeferredTask(function* () {
      try {
        this._initialized = true;

        yield TelemetryStorage.loadSavedPings();
        
        
        if (TelemetryStorage.pingsOverdue > 0) {
          this._log.trace("setupChromeProcess - Sending " + TelemetryStorage.pingsOverdue +
                          " overdue pings now.");
          
          
          
          yield this.sendPersistedPings();
        }

        
        this._clientID = yield ClientID.getClientID();
        Preferences.set(PREF_CACHED_CLIENTID, this._clientID);

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

  
  _cleanupOnShutdown: Task.async(function*() {
    if (!this._initialized) {
      return;
    }

    Preferences.ignore(PREF_BRANCH_LOG, configureLogging);

    
    for (let [url, request] of this._pendingPingRequests) {
      this._log.trace("_cleanupOnShutdown - aborting ping request for " + url);
      try {
        request.abort();
      } catch (e) {
        this._log.error("_cleanupOnShutdown - failed to abort request to " + url, e);
      }
    }
    this._pendingPingRequests.clear();

    
    try {
      
      yield this._shutdownBarrier.wait();
      
      yield this._connectionsBarrier.wait();
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
    
    if (!this._log) {
      
      
      configureLogging();
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("observe - " + aTopic + " notified.");

    switch (aTopic) {
    case "profile-after-change":
      
      return this.setupTelemetry();
    case "app-startup":
      
      
      
      if (!this.enableTelemetryRecording()) {
        this._log.trace("observe - Content process recording disabled.");
        return;
      }
      break;
    }
  },

  get clientID() {
    return this._clientID;
  },

  




  _canSend: function() {
    return (Telemetry.isOfficialTelemetry || this._testMode) &&
           Preferences.get(PREF_FHR_UPLOAD_ENABLED, false);
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
};
