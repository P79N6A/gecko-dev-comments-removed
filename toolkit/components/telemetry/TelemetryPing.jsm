




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
const LOGGER_PREFIX = "TelemetryPing::";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_BRANCH_LOG = PREF_BRANCH + "log.";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_LOG_LEVEL = PREF_BRANCH_LOG + "level";
const PREF_LOG_DUMP = PREF_BRANCH_LOG + "dump";
const PREF_CACHED_CLIENTID = PREF_BRANCH + "cachedClientID"
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";

const PING_FORMAT_VERSION = 4;


const TELEMETRY_DELAY = 60000;

const TELEMETRY_TEST_DELAY = 100;

const DEFAULT_RETENTION_DAYS = 14;

const PING_SUBMIT_TIMEOUT_MS = 2 * 60 * 1000;

XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                   "@mozilla.org/base/telemetry;1",
                                   "nsITelemetry");
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryFile",
                                  "resource://gre/modules/TelemetryFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ThirdPartyCookieProbe",
                                  "resource://gre/modules/ThirdPartyCookieProbe.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryEnvironment",
                                  "resource://gre/modules/TelemetryEnvironment.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");





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

this.EXPORTED_SYMBOLS = ["TelemetryPing"];

this.TelemetryPing = Object.freeze({
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

  








  addPendingPing: function(aPingPath, aRemoveOriginal) {
    return Impl.addPendingPing(aPingPath, aRemoveOriginal);
  },

  













  send: function(aType, aPayload, aOptions = {}) {
    let options = aOptions;
    options.retentionDays = aOptions.retentionDays || DEFAULT_RETENTION_DAYS;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;

    return Impl.send(aType, aPayload, options);
  },

  













  savePendingPings: function(aType, aPayload, aOptions = {}) {
    let options = aOptions;
    options.retentionDays = aOptions.retentionDays || DEFAULT_RETENTION_DAYS;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;

    return Impl.savePendingPings(aType, aPayload, options);
  },

  



















  savePing: function(aType, aPayload, aOptions = {}) {
    let options = aOptions;
    options.retentionDays = aOptions.retentionDays || DEFAULT_RETENTION_DAYS;
    options.addClientId = aOptions.addClientId || false;
    options.addEnvironment = aOptions.addEnvironment || false;
    options.overwrite = aOptions.overwrite || false;

    return Impl.savePing(aType, aPayload, options);
  },

  




   get clientID() {
    return Impl.clientID;
   },

   


   get shutdown() {
    return Impl._shutdownBarrier.client;
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

  
  
  
  _shutdownBarrier: new AsyncShutdown.Barrier("TelemetryPing: Waiting for clients."),
  
  _connectionsBarrier: new AsyncShutdown.Barrier("TelemetryPing: Waiting for pending ping activity"),

  


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
      creationDate: (new Date()).toISOString(),
      version: PING_FORMAT_VERSION,
      application: this._getApplicationSection(),
      payload: aPayload,
    };

    if (aOptions.addClientId) {
      pingData.clientId = this._clientID;
    }

    if (aOptions.addEnvironment) {
      return TelemetryEnvironment.getEnvironmentData().then(environment => {
        pingData.environment = environment;
        return pingData;
      },
      error => {
        this._log.error("assemblePing - Rejection", error);
      });
    }

    return Promise.resolve(pingData);
  },

  popPayloads: function popPayloads() {
    this._log.trace("popPayloads");
    function payloadIter() {
      let iterator = TelemetryFile.popPendingPings();
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

  








  addPendingPing: function(aPingPath, aRemoveOriginal) {
    return TelemetryFile.addPendingPing(aPingPath).then(() => {
        if (aRemoveOriginal) {
          return OS.File.remove(aPingPath);
        }
      }, error => this._log.error("addPendingPing - Unable to add the pending ping", error));
  },

  















  send: function send(aType, aPayload, aOptions) {
    this._log.trace("send - Type " + aType + ", Server " + this._server +
                    ", aOptions " + JSON.stringify(aOptions));

    let promise = this.assemblePing(aType, aPayload, aOptions)
        .then(pingData => {
          
          let p = [
            
            this.doPing(pingData, false)
                .catch(() => TelemetryFile.savePing(pingData, true)),
            this.sendPersistedPings(),
          ];
          return Promise.all(p);
        },
        error => this._log.error("send - Rejection", error));

    this._trackPendingPingTask(promise);
    return promise;
  },

  




  sendPersistedPings: function sendPersistedPings() {
    this._log.trace("sendPersistedPings");

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

    return this.assemblePing(aType, aPayload, aOptions)
        .then(pingData => TelemetryFile.savePendingPings(pingData),
              error => this._log.error("savePendingPings - Rejection", error));
  },

  


















  savePing: function savePing(aType, aPayload, aOptions) {
    this._log.trace("savePing - Type " + aType + ", Server " + this._server +
                    ", aOptions " + JSON.stringify(aOptions));

    return this.assemblePing(aType, aPayload, aOptions)
      .then(pingData => {
        if ("filePath" in aOptions) {
          return TelemetryFile.savePingToFile(pingData, aOptions.filePath, aOptions.overwrite)
                              .then(() => { return pingData.id; });
        } else {
          return TelemetryFile.savePing(pingData, aOptions.overwrite)
                              .then(() => { return pingData.id; });
        }
      }, error => this._log.error("savePing - Rejection", error));
  },

  onPingRequestFinished: function(success, startTime, ping, isPersisted) {
    this._log.trace("onPingRequestFinished - success: " + success + ", persisted: " + isPersisted);

    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    hsuccess.add(success);
    hping.add(new Date() - startTime);

    if (success && isPersisted) {
      return TelemetryFile.cleanupPingFile(ping);
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

  


  enableTelemetryRecording: function enableTelemetryRecording(testing) {
    
#if !defined(MOZ_WIDGET_ANDROID)
    Telemetry.canRecordBase =
      Preferences.get("datareporting.healthreport.service.enabled", false) ||
      Preferences.get("browser.selfsupport.enabled", false);
#else
    
    Telemetry.canRecordBase = true;
#endif

#ifdef MOZILLA_OFFICIAL
    if (!Telemetry.isOfficialTelemetry && !testing) {
      
      
      
      Telemetry.canRecordExtended = false;
      this._log.config("enableTelemetryRecording - Can't send data, disabling Telemetry recording.");
      return false;
    }
#endif

    let enabled = Preferences.get(PREF_ENABLED, false);
    this._server = Preferences.get(PREF_SERVER, undefined);
    if (!enabled || !Telemetry.canRecordBase) {
      
      
      Telemetry.canRecordExtended = false;
      this._log.config("enableTelemetryRecording - Telemetry is disabled, turning off Telemetry recording.");
      return false;
    }

    return true;
  },

  









  setupTelemetry: function setupTelemetry(testing) {
    this._initStarted = true;
    if (testing && !this._log) {
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("setupTelemetry");

    if (this._delayedInitTask) {
      this._log.error("setupTelemetry - init task already running");
      return this._delayedInitTaskDeferred.promise;
    }

    if (this._initialized && !testing) {
      this._log.error("setupTelemetry - already initialized");
      return Promise.resolve();
    }

    
    this._thirdPartyCookies = new ThirdPartyCookieProbe();
    this._thirdPartyCookies.init();

    if (!this.enableTelemetryRecording(testing)) {
      this._log.config("setupTelemetry - Telemetry recording is disabled, skipping Telemetry setup.");
      return Promise.resolve();
    }

    
    
    
    
    this._clientID = Preferences.get(PREF_CACHED_CLIENTID, null);

    
    
    
    this._delayedInitTaskDeferred = Promise.defer();
    this._delayedInitTask = new DeferredTask(function* () {
      try {
        this._initialized = true;

        yield TelemetryEnvironment.init();

        yield TelemetryFile.loadSavedPings();
        
        
        if (TelemetryFile.pingsOverdue > 0) {
          this._log.trace("setupChromeProcess - Sending " + TelemetryFile.pingsOverdue +
                          " overdue pings now.");
          
          
          
          yield this.sendPersistedPings();
        }

        if ("@mozilla.org/datareporting/service;1" in Cc) {
          let drs = Cc["@mozilla.org/datareporting/service;1"]
                      .getService(Ci.nsISupports)
                      .wrappedJSObject;
          this._clientID = yield drs.getClientID();
          
          Preferences.set(PREF_CACHED_CLIENTID, this._clientID);
        } else {
          
          Preferences.reset(PREF_CACHED_CLIENTID);
        }

        Telemetry.asyncFetchTelemetryData(function () {});
        this._delayedInitTaskDeferred.resolve();
      } catch (e) {
        this._delayedInitTaskDeferred.reject(e);
      } finally {
        this._delayedInitTask = null;
        this._delayedInitTaskDeferred = null;
      }
    }.bind(this), testing ? TELEMETRY_TEST_DELAY : TELEMETRY_DELAY);

    AsyncShutdown.sendTelemetry.addBlocker("TelemetryPing: shutting down",
                                           () => this.shutdown(),
                                           () => this._getState());

    this._delayedInitTask.arm();
    return this._delayedInitTaskDeferred.promise;
  },

  
  _cleanupOnShutdown: Task.async(function*() {
    if (!this._initialized) {
      return;
    }

    try {
      
      yield this._shutdownBarrier.wait();
      
      yield this._connectionsBarrier.wait();

      
      try {
        yield TelemetryEnvironment.shutdown();
      } catch (e) {
        this._log.error("shutdown - environment shutdown failure", e);
      }
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
      
      Services.obs.addObserver(this, "content-child-shutdown", false);
      break;
    case "content-child-shutdown":
      
      Services.obs.removeObserver(this, "content-child-shutdown");
      Preferences.ignore(PREF_BRANCH_LOG, configureLogging);
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
};
