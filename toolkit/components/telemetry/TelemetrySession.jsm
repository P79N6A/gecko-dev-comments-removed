




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/debug.js", this);
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/DeferredTask.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

const IS_CONTENT_PROCESS = (function() {
  
  
  let runtime = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);
  return runtime.processType == Ci.nsIXULRuntime.PROCESS_TYPE_CONTENT;
})();


const PAYLOAD_VERSION = 1;
const PING_TYPE_MAIN = "main";
const RETENTION_DAYS = 14;

const REASON_DAILY = "daily";
const REASON_SAVED_SESSION = "saved-session";
const REASON_IDLE_DAILY = "idle-daily";
const REASON_GATHER_PAYLOAD = "gather-payload";
const REASON_TEST_PING = "test-ping";
const REASON_ENVIRONMENT_CHANGE = "environment-change";

const ENVIRONMENT_CHANGE_LISTENER = "TelemetrySession::onEnvironmentChange";

const SEC_IN_ONE_DAY  = 24 * 60 * 60;
const MS_IN_ONE_DAY   = SEC_IN_ONE_DAY * 1000;

const MIN_SUBSESSION_LENGTH_MS = 10 * 60 * 1000;



#expand const HISTOGRAMS_FILE_VERSION = "__HISTOGRAMS_FILE_VERSION__";

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetrySession::";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_PREVIOUS_BUILDID = PREF_BRANCH + "previousBuildID";
const PREF_CACHED_CLIENTID = PREF_BRANCH + "cachedClientID"
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";
const PREF_ASYNC_PLUGIN_INIT = "dom.ipc.plugins.asyncInit";

const MESSAGE_TELEMETRY_PAYLOAD = "Telemetry:Payload";


const TELEMETRY_INTERVAL = 60000;

const TELEMETRY_DELAY = 60000;

const TELEMETRY_TEST_DELAY = 100;




const IDLE_TIMEOUT_SECONDS = 5 * 60;

var gLastMemoryPoll = null;

let gWasDebuggerAttached = false;

function getLocale() {
  return Cc["@mozilla.org/chrome/chrome-registry;1"].
         getService(Ci.nsIXULChromeRegistry).
         getSelectedLocale('global');
}

XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                   "@mozilla.org/base/telemetry;1",
                                   "nsITelemetry");
XPCOMUtils.defineLazyServiceGetter(this, "idleService",
                                   "@mozilla.org/widget/idleservice;1",
                                   "nsIIdleService");
XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");
XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManagerPrivate",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryPing",
                                  "resource://gre/modules/TelemetryPing.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryFile",
                                  "resource://gre/modules/TelemetryFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ThirdPartyCookieProbe",
                                  "resource://gre/modules/ThirdPartyCookieProbe.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
                                  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryEnvironment",
                                  "resource://gre/modules/TelemetryEnvironment.jsm");

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}




let Policy = {
  now: () => new Date(),
  setDailyTimeout: (callback, delayMs) => setTimeout(callback, delayMs),
  clearDailyTimeout: (id) => clearTimeout(id),
};




function truncateToDays(date) {
  return new Date(date.getFullYear(),
                  date.getMonth(),
                  date.getDate(),
                  0, 0, 0, 0);
}





function toLocalTimeISOString(date) {
  function padNumber(number, places) {
    number = number.toString();
    while (number.length < places) {
      number = "0" + number;
    }
    return number;
  }

  let sign = (n) => n >= 0 ? "+" : "-";
  let tzOffset = date.getTimezoneOffset();

  
  return    padNumber(date.getFullYear(), 4)
    + "-" + padNumber(date.getMonth() + 1, 2)
    + "-" + padNumber(date.getDate(), 2)
    + "T" + padNumber(date.getHours(), 2)
    + ":" + padNumber(date.getMinutes(), 2)
    + ":" + padNumber(date.getSeconds(), 2);
    + "." + date.getMilliseconds()
    + sign(tzOffset) + Math.abs(Math.floor(tzOffset / 60))
    + ":" + Math.abs(tzOffset % 60);
}




let processInfo = {
  _initialized: false,
  _IO_COUNTERS: null,
  _kernel32: null,
  _GetProcessIoCounters: null,
  _GetCurrentProcess: null,
  getCounters: function() {
    let isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
    if (isWindows)
      return this.getCounters_Windows();
    return null;
  },
  getCounters_Windows: function() {
    if (!this._initialized){
      Cu.import("resource://gre/modules/ctypes.jsm");
      this._IO_COUNTERS = new ctypes.StructType("IO_COUNTERS", [
        {'readOps': ctypes.unsigned_long_long},
        {'writeOps': ctypes.unsigned_long_long},
        {'otherOps': ctypes.unsigned_long_long},
        {'readBytes': ctypes.unsigned_long_long},
        {'writeBytes': ctypes.unsigned_long_long},
        {'otherBytes': ctypes.unsigned_long_long} ]);
      try {
        this._kernel32 = ctypes.open("Kernel32.dll");
        this._GetProcessIoCounters = this._kernel32.declare("GetProcessIoCounters",
          ctypes.winapi_abi,
          ctypes.bool, 
          ctypes.voidptr_t, 
          this._IO_COUNTERS.ptr); 
        this._GetCurrentProcess = this._kernel32.declare("GetCurrentProcess",
          ctypes.winapi_abi,
          ctypes.voidptr_t); 
        this._initialized = true;
      } catch (err) {
        return null;
      }
    }
    let io = new this._IO_COUNTERS();
    if(!this._GetProcessIoCounters(this._GetCurrentProcess(), io.address()))
      return null;
    return [parseInt(io.readBytes), parseInt(io.writeBytes)];
  }
};

this.EXPORTED_SYMBOLS = ["TelemetrySession"];

this.TelemetrySession = Object.freeze({
  Constants: Object.freeze({
    PREF_ENABLED: PREF_ENABLED,
    PREF_SERVER: PREF_SERVER,
    PREF_PREVIOUS_BUILDID: PREF_PREVIOUS_BUILDID,
  }),
  


  testPing: function() {
    return Impl.testPing();
  },
  





  getPayload: function(reason, clearSubsession = false) {
    return Impl.getPayload(reason, clearSubsession);
  },
  





  testSaveHistograms: function(aFile) {
    return Impl.testSaveHistograms(aFile);
  },
  


  gatherStartup: function() {
    return Impl.gatherStartup();
  },
  




  setAddOns: function(aAddOns) {
    return Impl.setAddOns(aAddOns);
  },
  







  getMetadata: function(reason) {
    return Impl.getMetadata(reason);
  },
  


  reset: function() {
    this.uninstall();
    return this.setup();
  },
  




  shutdown: function(aForceSavePending = true) {
    return Impl.shutdown(aForceSavePending);
  },
  


  setup: function() {
    return Impl.setupChromeProcess(true);
  },
  


  uninstall: function() {
    try {
      Impl.uninstall();
    } catch (ex) {
      
    }
  },
  


  observe: function (aSubject, aTopic, aData) {
    return Impl.observe(aSubject, aTopic, aData);
  },
  




   get clientID() {
    return Impl.clientID;
   },
});

let Impl = {
  _histograms: {},
  _initialized: false,
  _log: null,
  _prevValues: {},
  
  
  _uuid: generateUUID(),
  
  
  _startupHistogramRegex: /SQLITE|HTTP|SPDY|CACHE|DNS/,
  _slowSQLStartup: {},
  _prevSession: null,
  _hasWindowRestoredObserver: false,
  _hasXulWindowVisibleObserver: false,
  _startupIO : {},
  
  
  _previousBuildID: undefined,
  
  
  
  
  _childTelemetry: [],
  
  _subsessionStartDate: null,
  
  _dailyTimerId: null,

  





  getSimpleMeasurements: function getSimpleMeasurements(forSavedSession) {
    this._log.trace("getSimpleMeasurements");

    let si = Services.startup.getStartupInfo();

    
    var ret = {
      
      uptime: Math.round((new Date() - si.process) / 60000)
    }

    
    var appTimestamps = {};
    try {
      let o = {};
      Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", o);
      appTimestamps = o.TelemetryTimestamps.get();
    } catch (ex) {}
    try {
      if (!IS_CONTENT_PROCESS) {
        ret.addonManager = AddonManagerPrivate.getSimpleMeasures();
      }
    } catch (ex) {}
    try {
      if (!IS_CONTENT_PROCESS) {
        ret.UITelemetry = UITelemetry.getSimpleMeasures();
      }
    } catch (ex) {}

    if (si.process) {
      for each (let field in Object.keys(si)) {
        if (field == "process")
          continue;
        ret[field] = si[field] - si.process
      }

      for (let p in appTimestamps) {
        if (!(p in ret) && appTimestamps[p])
          ret[p] = appTimestamps[p] - si.process;
      }
    }

    ret.startupInterrupted = Number(Services.startup.interrupted);

    ret.js = Cu.getJSEngineTelemetryValue();

    let maximalNumberOfConcurrentThreads = Telemetry.maximalNumberOfConcurrentThreads;
    if (maximalNumberOfConcurrentThreads) {
      ret.maximalNumberOfConcurrentThreads = maximalNumberOfConcurrentThreads;
    }

    if (IS_CONTENT_PROCESS) {
      return ret;
    }

    

    
    let debugService = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
    let isDebuggerAttached = debugService.isDebuggerAttached;
    gWasDebuggerAttached = gWasDebuggerAttached || isDebuggerAttached;
    ret.debuggerAttached = Number(gWasDebuggerAttached);

    let shutdownDuration = Telemetry.lastShutdownDuration;
    if (shutdownDuration)
      ret.shutdownDuration = shutdownDuration;

    let failedProfileLockCount = Telemetry.failedProfileLockCount;
    if (failedProfileLockCount)
      ret.failedProfileLockCount = failedProfileLockCount;

    for (let ioCounter in this._startupIO)
      ret[ioCounter] = this._startupIO[ioCounter];

    let hasPingBeenSent = false;
    try {
      hasPingBeenSent = Telemetry.getHistogramById("TELEMETRY_SUCCESS").snapshot().sum > 0;
    } catch(e) {
    }
    if (!forSavedSession || hasPingBeenSent) {
      ret.savedPings = TelemetryFile.pingsLoaded;
    }

    ret.activeTicks = -1;
    if ("@mozilla.org/datareporting/service;1" in Cc) {
      let drs = Cc["@mozilla.org/datareporting/service;1"]
                  .getService(Ci.nsISupports)
                  .wrappedJSObject;

      let sr = drs.getSessionRecorder();
      if (sr) {
        ret.activeTicks = sr.activeTicks;
      }
    }

    ret.pingsOverdue = TelemetryFile.pingsOverdue;
    ret.pingsDiscarded = TelemetryFile.pingsDiscarded;

    return ret;
  },

  




















  packHistogram: function packHistogram(hgram) {
    let r = hgram.ranges;;
    let c = hgram.counts;
    let retgram = {
      range: [r[1], r[r.length - 1]],
      bucket_count: r.length,
      histogram_type: hgram.histogram_type,
      values: {},
      sum: hgram.sum
    };

    if (hgram.histogram_type == Telemetry.HISTOGRAM_EXPONENTIAL) {
      retgram.log_sum = hgram.log_sum;
      retgram.log_sum_squares = hgram.log_sum_squares;
    } else {
      retgram.sum_squares_lo = hgram.sum_squares_lo;
      retgram.sum_squares_hi = hgram.sum_squares_hi;
    }

    let first = true;
    let last = 0;

    for (let i = 0; i < c.length; i++) {
      let value = c[i];
      if (!value)
        continue;

      
      if (i && first) {
        retgram.values[r[i - 1]] = 0;
      }
      first = false;
      last = i + 1;
      retgram.values[r[i]] = value;
    }

    
    if (last && last < c.length)
      retgram.values[r[last]] = 0;
    return retgram;
  },

  getHistograms: function getHistograms(subsession, clearSubsession) {
    this._log.trace("getHistograms - subsession: " + subsession + ", clearSubsession: " + clearSubsession);

    let registered =
      Telemetry.registeredHistograms(Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTIN, []);
    let hls = subsession ? Telemetry.snapshotSubsessionHistograms(clearSubsession)
                         : Telemetry.histogramSnapshots;
    let ret = {};

    for (let name of registered) {
      for (let n of [name, "STARTUP_" + name]) {
        if (n in hls) {
          ret[n] = this.packHistogram(hls[n]);
        }
      }
    }

    return ret;
  },

  getAddonHistograms: function getAddonHistograms() {
    this._log.trace("getAddonHistograms");

    let ahs = Telemetry.addonHistogramSnapshots;
    let ret = {};

    for (let addonName in ahs) {
      let addonHistograms = ahs[addonName];
      let packedHistograms = {};
      for (let name in addonHistograms) {
        packedHistograms[name] = this.packHistogram(addonHistograms[name]);
      }
      if (Object.keys(packedHistograms).length != 0)
        ret[addonName] = packedHistograms;
    }

    return ret;
  },

  getKeyedHistograms: function(subsession, clearSubsession) {
    this._log.trace("getKeyedHistograms - subsession: " + subsession + ", clearSubsession: " + clearSubsession);

    let registered =
      Telemetry.registeredKeyedHistograms(Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTIN, []);
    let ret = {};

    for (let id of registered) {
      ret[id] = {};
      let keyed = Telemetry.getKeyedHistogramById(id);
      let snapshot = null;
      if (subsession) {
        snapshot = clearSubsession ? keyed.snapshotSubsessionAndClear()
                                   : keyed.subsessionSnapshot();
      } else {
        snapshot = keyed.snapshot();
      }
      for (let key of Object.keys(snapshot)) {
        ret[id][key] = this.packHistogram(snapshot[key]);
      }
    }

    return ret;
  },

  getThreadHangStats: function getThreadHangStats(stats) {
    this._log.trace("getThreadHangStats");

    stats.forEach((thread) => {
      thread.activity = this.packHistogram(thread.activity);
      thread.hangs.forEach((hang) => {
        hang.histogram = this.packHistogram(hang.histogram);
      });
    });
    return stats;
  },

  







  getMetadata: function getMetadata(reason) {
    this._log.trace("getMetadata - Reason " + reason);

    let subsessionStartDate = toLocalTimeISOString(truncateToDays(this._subsessionStartDate));

    let ai = Services.appinfo;
    let ret = {
      reason: reason,
      OS: ai.OS,
      appVersion: ai.version, 
      appName: ai.name, 
      appBuildID: ai.appBuildID, 
      appUpdateChannel: UpdateChannel.get(), 
      platformBuildID: ai.platformBuildID,
      revision: HISTOGRAMS_FILE_VERSION,
      asyncPluginInit: Preferences.get(PREF_ASYNC_PLUGIN_INIT, false)

      subsessionStartDate: subsessionStartDate,
    };

    
    
    
    if(Services.metro && Services.metro.immersive) {
      ret.appName = "MetroFirefox";
    }

    if (this._previousBuildID) {
      ret.previousBuildID = this._previousBuildID;
    }

    if (this._addons)
      ret.addons = this._addons;

    let flashVersion = this.getFlashVersion();
    if (flashVersion)
      ret.flashVersion = flashVersion;

    return ret;
  },

  


  gatherMemory: function gatherMemory() {
    this._log.trace("gatherMemory");

    let mgr;
    try {
      mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
            getService(Ci.nsIMemoryReporterManager);
    } catch (e) {
      
      return;
    }

    let histogram = Telemetry.getHistogramById("TELEMETRY_MEMORY_REPORTER_MS");
    let startTime = new Date();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let boundHandleMemoryReport = this.handleMemoryReport.bind(this);
    function h(id, units, amountName) {
      try {
        
        
        
        
        let amount = mgr[amountName];
        NS_ASSERT(amount !== undefined,
                  "telemetry accessed an unknown distinguished amount");
        boundHandleMemoryReport(id, units, amount);
      } catch (e) {
      };
    }
    let b = (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_BYTES, n);
    let c = (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_COUNT, n);
    let cc= (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE, n);
    let p = (id, n) => h(id, Ci.nsIMemoryReporter.UNITS_PERCENTAGE, n);

    b("MEMORY_VSIZE", "vsize");
    b("MEMORY_VSIZE_MAX_CONTIGUOUS", "vsizeMaxContiguous");
    b("MEMORY_RESIDENT", "residentFast");
    b("MEMORY_HEAP_ALLOCATED", "heapAllocated");
    p("MEMORY_HEAP_COMMITTED_UNUSED_RATIO", "heapOverheadRatio");
    b("MEMORY_JS_GC_HEAP", "JSMainRuntimeGCHeap");
    b("MEMORY_JS_MAIN_RUNTIME_TEMPORARY_PEAK", "JSMainRuntimeTemporaryPeak");
    c("MEMORY_JS_COMPARTMENTS_SYSTEM", "JSMainRuntimeCompartmentsSystem");
    c("MEMORY_JS_COMPARTMENTS_USER", "JSMainRuntimeCompartmentsUser");
    b("MEMORY_IMAGES_CONTENT_USED_UNCOMPRESSED", "imagesContentUsedUncompressed");
    b("MEMORY_STORAGE_SQLITE", "storageSQLite");
    cc("MEMORY_EVENTS_VIRTUAL", "lowMemoryEventsVirtual");
    cc("MEMORY_EVENTS_PHYSICAL", "lowMemoryEventsPhysical");
    c("GHOST_WINDOWS", "ghostWindows");
    cc("PAGE_FAULTS_HARD", "pageFaultsHard");

    histogram.add(new Date() - startTime);
  },

  handleMemoryReport: function(id, units, amount) {
    let val;
    if (units == Ci.nsIMemoryReporter.UNITS_BYTES) {
      val = Math.floor(amount / 1024);
    }
    else if (units == Ci.nsIMemoryReporter.UNITS_PERCENTAGE) {
      
      val = Math.floor(amount / 100);
    }
    else if (units == Ci.nsIMemoryReporter.UNITS_COUNT) {
      val = amount;
    }
    else if (units == Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE) {
      
      

      if (!(id in this._prevValues)) {
        
        
        
        this._prevValues[id] = amount;
        return;
      }

      val = amount - this._prevValues[id];
      this._prevValues[id] = amount;
    }
    else {
      NS_ASSERT(false, "Can't handle memory reporter with units " + units);
      return;
    }

    let h = this._histograms[id];
    if (!h) {
      h = Telemetry.getHistogramById(id);
      this._histograms[id] = h;
    }
    h.add(val);
  },

  



  isInterestingStartupHistogram: function isInterestingStartupHistogram(name) {
    return this._startupHistogramRegex.test(name);
  },

  getChildPayloads: function getChildPayloads() {
    return [for (child of this._childTelemetry) child.payload];
  },

  


  gatherStartupHistograms: function gatherStartupHistograms() {
    this._log.trace("gatherStartupHistograms");

    let info =
      Telemetry.registeredHistograms(Ci.nsITelemetry.DATASET_RELEASE_CHANNEL_OPTIN, []);
    let snapshots = Telemetry.histogramSnapshots;
    for (let name of info) {
      
      if (this.isInterestingStartupHistogram(name) && name in snapshots) {
        Telemetry.histogramFrom("STARTUP_" + name, name);
      }
    }
  },

  





  assemblePayloadWithMeasurements: function(simpleMeasurements, info, reason, clearSubsession) {
    const isSubsession = !this._isClassicReason(reason);
    this._log.trace("assemblePayloadWithMeasurements - reason: " + reason +
                    ", submitting subsession data: " + isSubsession);

    
    let payloadObj = {
      ver: PAYLOAD_VERSION,
      simpleMeasurements: simpleMeasurements,
      histograms: this.getHistograms(isSubsession, clearSubsession),
      keyedHistograms: this.getKeyedHistograms(isSubsession, clearSubsession),
      chromeHangs: Telemetry.chromeHangs,
      threadHangStats: this.getThreadHangStats(Telemetry.threadHangStats),
      log: TelemetryLog.entries(),
    };

    if (IS_CONTENT_PROCESS) {
      return payloadObj;
    }

    
    payloadObj.info = info;
    payloadObj.slowSQL = Telemetry.slowSQL;
    payloadObj.fileIOReports = Telemetry.fileIOReports;
    payloadObj.lateWrites = Telemetry.lateWrites;
    payloadObj.addonHistograms = this.getAddonHistograms();
    payloadObj.addonDetails = AddonManagerPrivate.getTelemetryDetails();
    payloadObj.UIMeasurements = UITelemetry.getUIMeasurements();

    if (Object.keys(this._slowSQLStartup).length != 0 &&
        (Object.keys(this._slowSQLStartup.mainThread).length ||
         Object.keys(this._slowSQLStartup.otherThreads).length)) {
      payloadObj.slowSQLStartup = this._slowSQLStartup;
    }

    let clientID = TelemetryPing.clientID;
    if (clientID && Preferences.get(PREF_FHR_UPLOAD_ENABLED, false)) {
      payloadObj.clientID = clientID;
    }

    if (this._childTelemetry.length) {
      payloadObj.childPayloads = this.getChildPayloads();
    }

    return payloadObj;
  },

  getSessionPayload: function getSessionPayload(reason, clearSubsession) {
    this._log.trace("getSessionPayload - reason: " + reason + ", clearSubsession: " + clearSubsession);

    let measurements = this.getSimpleMeasurements(reason == REASON_SAVED_SESSION);
    let info = !IS_CONTENT_PROCESS ? this.getMetadata(reason) : null;
    let payload = this.assemblePayloadWithMeasurements(measurements, info, reason, clearSubsession);

    if (clearSubsession) {
      this._subsessionStartDate = Policy.now();
      this._rescheduleDailyTimer();
    }

    return payload;
  },

  


  send: function send(reason) {
    this._log.trace("send - Reason " + reason);
    
    this.gatherMemory();

    const isSubsession = !this._isClassicReason(reason);
    let payload = this.getSessionPayload(reason, isSubsession);
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };
    return TelemetryPing.send(PING_TYPE_MAIN, payload, options);
  },

  attachObservers: function attachObservers() {
    if (!this._initialized)
      return;
    Services.obs.addObserver(this, "cycle-collector-begin", false);
    Services.obs.addObserver(this, "idle-daily", false);
  },

  detachObservers: function detachObservers() {
    if (!this._initialized)
      return;
    Services.obs.removeObserver(this, "idle-daily");
    Services.obs.removeObserver(this, "cycle-collector-begin");
    if (this._isIdleObserver) {
      idleService.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._isIdleObserver = false;
    }
  },

  


  enableTelemetryRecording: function enableTelemetryRecording(testing) {

#ifdef MOZILLA_OFFICIAL
    if (!Telemetry.canSend && !testing) {
      
      
      
      Telemetry.canRecord = false;
      this._log.config("enableTelemetryRecording - Can't send data, disabling Telemetry recording.");
      return false;
    }
#endif

    let enabled = Preferences.get(PREF_ENABLED, false);
    this._server = Preferences.get(PREF_SERVER, undefined);
    if (!enabled) {
      
      
      Telemetry.canRecord = false;
      this._log.config("enableTelemetryRecording - Telemetry is disabled, turning off Telemetry recording.");
      return false;
    }

    return true;
  },

  


  setupChromeProcess: function setupChromeProcess(testing) {
    if (testing && !this._log) {
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("setupChromeProcess");

    this._sessionStartDate = Policy.now();
    this._subsessionStartDate = this._sessionStartDate;

    
    this._thirdPartyCookies = new ThirdPartyCookieProbe();
    this._thirdPartyCookies.init();

    
    
    let previousBuildID = Preferences.get(PREF_PREVIOUS_BUILDID, undefined);
    let thisBuildID = Services.appinfo.appBuildID;
    
    
    if (previousBuildID != thisBuildID) {
      this._previousBuildID = previousBuildID;
      Preferences.set(PREF_PREVIOUS_BUILDID, thisBuildID);
    }

    if (!this.enableTelemetryRecording(testing)) {
      this._log.config("setupChromeProcess - Telemetry recording is disabled, skipping Chrome process setup.");
      return Promise.resolve();
    }

    AsyncShutdown.sendTelemetry.addBlocker(
      "TelemetrySession: shutting down", () => this.shutdown());

    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
#ifdef MOZ_WIDGET_ANDROID
    Services.obs.addObserver(this, "application-background", false);
#endif
    Services.obs.addObserver(this, "xul-window-visible", false);
    this._hasWindowRestoredObserver = true;
    this._hasXulWindowVisibleObserver = true;

    ppmm.addMessageListener(MESSAGE_TELEMETRY_PAYLOAD, this);

    
    
    
    let deferred = Promise.defer();
    let delayedTask = new DeferredTask(function* () {
      this._initialized = true;

      this.attachObservers();
      this.gatherMemory();

      Telemetry.asyncFetchTelemetryData(function () {});
      this._rescheduleDailyTimer();

      TelemetryEnvironment.registerChangeListener(ENVIRONMENT_CHANGE_LISTENER,
                                                  () => this._onEnvironmentChange());

      deferred.resolve();

    }.bind(this), testing ? TELEMETRY_TEST_DELAY : TELEMETRY_DELAY);

    delayedTask.arm();
    return deferred.promise;
  },

  


  setupContentProcess: function setupContentProcess() {
    this._log.trace("setupContentProcess");

    if (!this.enableTelemetryRecording()) {
      return;
    }

    Services.obs.addObserver(this, "content-child-shutdown", false);

    this.gatherStartupHistograms();

    let delayedTask = new DeferredTask(function* () {
      this._initialized = true;

      this.attachObservers();
      this.gatherMemory();
    }.bind(this), TELEMETRY_DELAY);

    delayedTask.arm();
  },

  getFlashVersion: function getFlashVersion() {
    this._log.trace("getFlashVersion");
    let host = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let tags = host.getPluginTags();

    for (let i = 0; i < tags.length; i++) {
      if (tags[i].name == "Shockwave Flash")
        return tags[i].version;
    }

    return null;
  },

  receiveMessage: function receiveMessage(message) {
    this._log.trace("receiveMessage - Message name " + message.name);
    switch (message.name) {
    case MESSAGE_TELEMETRY_PAYLOAD:
    {
      let target = message.target;
      for (let child of this._childTelemetry) {
        if (child.source.get() === target) {
          
          child.payload = message.data;
          return;
        }
      }
      
      this._childTelemetry.push({
        source: Cu.getWeakReference(target),
        payload: message.data,
      });
      break;
    }
    default:
      throw new Error("Telemetry.receiveMessage: bad message name");
    }
  },

  sendContentProcessPing: function sendContentProcessPing(reason) {
    this._log.trace("sendContentProcessPing - Reason " + reason);
    const isSubsession = !this._isClassicReason(reason);
    let payload = this.getSessionPayload(reason, isSubsession);
    cpmm.sendAsyncMessage(MESSAGE_TELEMETRY_PAYLOAD, payload);
  },

  savePendingPings: function savePendingPings() {
    this._log.trace("savePendingPings");
    let payload = this.getSessionPayload(REASON_SAVED_SESSION, false);
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };
    return TelemetryPing.savePendingPings(PING_TYPE_MAIN, payload, options);
  },

  testSaveHistograms: function testSaveHistograms(file) {
    this._log.trace("testSaveHistograms - Path: " + file.path);
    let payload = this.getSessionPayload(REASON_SAVED_SESSION, false);
    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
      overwrite: true,
      filePath: file.path,
    };
    return TelemetryPing.testSavePingToFile(PING_TYPE_MAIN, payload, options);
  },

  


  uninstall: function uninstall() {
    this.detachObservers();
    if (this._hasWindowRestoredObserver) {
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      this._hasWindowRestoredObserver = false;
    }
    if (this._hasXulWindowVisibleObserver) {
      Services.obs.removeObserver(this, "xul-window-visible");
      this._hasXulWindowVisibleObserver = false;
    }
#ifdef MOZ_WIDGET_ANDROID
    Services.obs.removeObserver(this, "application-background", false);
#endif
  },

  getPayload: function getPayload(reason, clearSubsession) {
    this._log.trace("getPayload - clearSubsession: " + clearSubsession);
    reason = reason || REASON_GATHER_PAYLOAD;
    
    
    if (Object.keys(this._slowSQLStartup).length == 0) {
      this.gatherStartupHistograms();
      this._slowSQLStartup = Telemetry.slowSQL;
    }
    this.gatherMemory();
    return this.getSessionPayload(reason, clearSubsession);
  },

  gatherStartup: function gatherStartup() {
    this._log.trace("gatherStartup");
    let counters = processInfo.getCounters();
    if (counters) {
      [this._startupIO.startupSessionRestoreReadBytes,
        this._startupIO.startupSessionRestoreWriteBytes] = counters;
    }
    this.gatherStartupHistograms();
    this._slowSQLStartup = Telemetry.slowSQL;
  },

  setAddOns: function setAddOns(aAddOns) {
    this._addons = aAddOns;
  },

  sendIdlePing: function sendIdlePing(aTest) {
    this._log.trace("sendIdlePing");
    if (this._isIdleObserver) {
      idleService.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._isIdleObserver = false;
    }
    if (aTest) {
      return this.send(REASON_TEST_PING);
    } else if (Telemetry.canSend) {
      return this.send(REASON_IDLE_DAILY);
    }
  },

  testPing: function testPing() {
    return this.sendIdlePing(true);
  },

  


  observe: function (aSubject, aTopic, aData) {
    if (!this._log) {
      this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    this._log.trace("observe - " + aTopic + " notified.");

    switch (aTopic) {
    case "profile-after-change":
      
      return this.setupChromeProcess();
    case "app-startup":
      
      return this.setupContentProcess();
    case "content-child-shutdown":
      
      Services.obs.removeObserver(this, "content-child-shutdown");
      this.uninstall();

      if (Telemetry.canSend) {
        this.sendContentProcessPing(REASON_SAVED_SESSION);
      }
      break;
    case "cycle-collector-begin":
      let now = new Date();
      if (!gLastMemoryPoll
          || (TELEMETRY_INTERVAL <= now - gLastMemoryPoll)) {
        gLastMemoryPoll = now;
        this.gatherMemory();
      }
      break;
    case "xul-window-visible":
      Services.obs.removeObserver(this, "xul-window-visible");
      this._hasXulWindowVisibleObserver = false;
      var counters = processInfo.getCounters();
      if (counters) {
        [this._startupIO.startupWindowVisibleReadBytes,
          this._startupIO.startupWindowVisibleWriteBytes] = counters;
      }
      break;
    case "sessionstore-windows-restored":
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      this._hasWindowRestoredObserver = false;
      
      let debugService = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
      gWasDebuggerAttached = debugService.isDebuggerAttached;
      this.gatherStartup();
      break;
    case REASON_IDLE_DAILY:
      
      
      Services.tm.mainThread.dispatch((function() {
        
        Services.obs.notifyObservers(null, "gather-telemetry", null);
        
        idleService.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
        this._isIdleObserver = true;
      }).bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      break;
    case "idle":
      this.sendIdlePing(false, this._server);
      break;

#ifdef MOZ_WIDGET_ANDROID
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    case "application-background":
      if (Telemetry.canSend) {
        let payload = this.getSessionPayload(REASON_SAVED_SESSION, false);
        let options = {
          retentionDays: RETENTION_DAYS,
          addClientId: true,
          addEnvironment: true,
          overwrite: true,
        };
        TelemetryPing.savePing(PING_TYPE_MAIN, payload, options);
      }
      break;
#endif
    }
  },

  




  shutdown: function(testing = false) {
    TelemetryEnvironment.unregisterChangeListener(ENVIRONMENT_CHANGE_LISTENER);

    if (this._dailyTimerId) {
      Policy.clearDailyTimeout(this._dailyTimerId);
      this._dailyTimerId = null;
    }
    this.uninstall();
    if (Telemetry.canSend || testing) {
      return this.savePendingPings();
    }
    return Promise.resolve();
  },

  _rescheduleDailyTimer: function() {
    if (this._dailyTimerId) {
      this._log.trace("_rescheduleDailyTimer - clearing existing timeout");
      Policy.clearDailyTimeout(this._dailyTimerId);
    }

    let now = Policy.now();
    let midnight = truncateToDays(now).getTime() + MS_IN_ONE_DAY;
    let msUntilCollection = midnight - now.getTime();
    if (msUntilCollection < MIN_SUBSESSION_LENGTH_MS) {
      msUntilCollection += MS_IN_ONE_DAY;
    }

    this._log.trace("_rescheduleDailyTimer - now: " + now
                    + ", scheduled: " + new Date(now.getTime() + msUntilCollection));
    this._dailyTimerId = Policy.setDailyTimeout(() => this._onDailyTimer(), msUntilCollection);
  },

  _onDailyTimer: function() {
    if (!this._initialized) {
      if (this._log) {
        this._log.warn("_onDailyTimer - not initialized");
      } else {
        Cu.reportError("TelemetrySession._onDailyTimer - not initialized");
      }
      return;
    }

    this._log.trace("_onDailyTimer");
    let payload = this.getSessionPayload(REASON_DAILY, true);

    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };
    let promise = TelemetryPing.send(PING_TYPE_MAIN, payload, options);

    this._rescheduleDailyTimer();
    
    return promise;
  },

  _onEnvironmentChange: function() {
    this._log.trace("_onEnvironmentChange");
    let payload = this.getSessionPayload(REASON_ENVIRONMENT_CHANGE, true);

    let options = {
      retentionDays: RETENTION_DAYS,
      addClientId: true,
      addEnvironment: true,
    };
    let promise = TelemetryPing.send(PING_TYPE_MAIN, payload, options);
  },

  _isClassicReason: function(reason) {
    const classicReasons = [
      REASON_SAVED_SESSION,
      REASON_IDLE_DAILY,
      REASON_GATHER_PAYLOAD,
      REASON_TEST_PING,
    ];
    return classicReasons.indexOf(reason) != -1;
  },
};
