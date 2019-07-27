




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/debug.js", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
#ifndef MOZ_WIDGET_GONK
Cu.import("resource://gre/modules/LightweightThemeManager.jsm", this);
#endif
Cu.import("resource://gre/modules/ThirdPartyCookieProbe.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/AsyncShutdown.jsm", this);


const PAYLOAD_VERSION = 1;



#expand const HISTOGRAMS_FILE_VERSION = "__HISTOGRAMS_FILE_VERSION__";

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_PREVIOUS_BUILDID = PREF_BRANCH + "previousBuildID";


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
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManagerPrivate",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryFile",
                                  "resource://gre/modules/TelemetryFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
                                  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
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

this.EXPORTED_SYMBOLS = ["TelemetryPing"];

this.TelemetryPing = Object.freeze({
  



  getPayload: function() {
    return Impl.getPayload();
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
  




  testPing: function(aServer) {
    return Impl.testPing(aServer);
  },
  





  testLoadHistograms: function(aFile) {
    return Impl.testLoadHistograms(aFile);
  },
  



  submissionPath: function() {
    return Impl.submissionPath();
  },
  Constants: Object.freeze({
    PREF_ENABLED: PREF_ENABLED,
    PREF_SERVER: PREF_SERVER,
    PREF_PREVIOUS_BUILDID: PREF_PREVIOUS_BUILDID,
  }),
  


  reset: function() {
    this.uninstall();
    return this.setup();
  },
  


  setup: function() {
    return Impl.setup(true);
  },
  


  uninstall: function() {
    try {
      Impl.uninstall();
    } catch (ex) {
      
    }
  },
  







  getMetadata: function(reason) {
    return Impl.getMetadata(reason);
  },
  


  observe: function (aSubject, aTopic, aData) {
    return Impl.observe(aSubject, aTopic, aData);
  }
});

let Impl = {
  _histograms: {},
  _initialized: false,
  _prevValues: {},
  
  
  _uuid: generateUUID(),
  
  
  _startupHistogramRegex: /SQLITE|HTTP|SPDY|CACHE|DNS/,
  _slowSQLStartup: {},
  _prevSession: null,
  _hasWindowRestoredObserver: false,
  _hasXulWindowVisibleObserver: false,
  _startupIO : {},
  
  
  _previousBuildID: undefined,

  





  getSimpleMeasurements: function getSimpleMeasurements(forSavedSession) {
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
      ret.addonManager = AddonManagerPrivate.getSimpleMeasures();
    } catch (ex) {}
    try {
      ret.UITelemetry = UITelemetry.getSimpleMeasures();
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

    
    let debugService = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
    let isDebuggerAttached = debugService.isDebuggerAttached;
    gWasDebuggerAttached = gWasDebuggerAttached || isDebuggerAttached;
    ret.debuggerAttached = Number(gWasDebuggerAttached);

    ret.js = Cu.getJSEngineTelemetryValue();

    let shutdownDuration = Telemetry.lastShutdownDuration;
    if (shutdownDuration)
      ret.shutdownDuration = shutdownDuration;

    let failedProfileLockCount = Telemetry.failedProfileLockCount;
    if (failedProfileLockCount)
      ret.failedProfileLockCount = failedProfileLockCount;

    let maximalNumberOfConcurrentThreads = Telemetry.maximalNumberOfConcurrentThreads;
    if (maximalNumberOfConcurrentThreads)
      ret.maximalNumberOfConcurrentThreads = maximalNumberOfConcurrentThreads;

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

  getHistograms: function getHistograms(hls) {
    let registered = Telemetry.registeredHistograms([]);
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

  getThreadHangStats: function getThreadHangStats(stats) {
    stats.forEach((thread) => {
      thread.activity = this.packHistogram(thread.activity);
      thread.hangs.forEach((hang) => {
        hang.histogram = this.packHistogram(hang.histogram);
      });
    });
    return stats;
  },

  







  getMetadata: function getMetadata(reason) {
    let ai = Services.appinfo;
    let ret = {
      reason: reason,
      OS: ai.OS,
      appID: ai.ID,
      appVersion: ai.version,
      appName: ai.name,
      appBuildID: ai.appBuildID,
      appUpdateChannel: UpdateChannel.get(),
      platformBuildID: ai.platformBuildID,
      revision: HISTOGRAMS_FILE_VERSION,
      locale: getLocale()
    };

    
    
    
    if(Services.metro && Services.metro.immersive) {
      ret.appName = "MetroFirefox";
    }

    if (this._previousBuildID) {
      ret.previousBuildID = this._previousBuildID;
    }

    
    let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
    let fields = ["cpucount", "memsize", "arch", "version", "kernel_version",
                  "device", "manufacturer", "hardware", "tablet",
                  "hasMMX", "hasSSE", "hasSSE2", "hasSSE3",
                  "hasSSSE3", "hasSSE4A", "hasSSE4_1", "hasSSE4_2",
                  "hasEDSP", "hasARMv6", "hasARMv7", "hasNEON", "isWow64",
                  "profileHDDModel", "profileHDDRevision", "binHDDModel",
                  "binHDDRevision", "winHDDModel", "winHDDRevision"];
    for each (let field in fields) {
      let value;
      try {
        value = sysInfo.getProperty(field);
      } catch (e) {
        continue;
      }
      if (field == "memsize") {
        
        
        value = Math.round(value / 1024 / 1024);
      }
      ret[field] = value;
    }

    
    let gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);
    let gfxfields = ["adapterDescription", "adapterVendorID", "adapterDeviceID",
                     "adapterSubsysID", "adapterRAM", "adapterDriver",
                     "adapterDriverVersion", "adapterDriverDate",
                     "adapterDescription2", "adapterVendorID2",
                     "adapterDeviceID2", "adapterSubsysID2", "adapterRAM2",
                     "adapterDriver2", "adapterDriverVersion2",
                     "adapterDriverDate2", "isGPU2Active", "D2DEnabled",
                     "DWriteEnabled", "DWriteVersion"
                    ];

    if (gfxInfo) {
      for each (let field in gfxfields) {
        try {
          let value = gfxInfo[field];
          
          
          
          if (value !== "") {
            ret[field] = value;
          }
        } catch (e) {
          continue
        }
      }
    }

#ifndef MOZ_WIDGET_GONK
    let theme = LightweightThemeManager.currentTheme;
    if (theme)
      ret.persona = theme.id;
#endif

    if (this._addons)
      ret.addons = this._addons;

    let flashVersion = this.getFlashVersion();
    if (flashVersion)
      ret.flashVersion = flashVersion;

    try {
      let scope = {};
      Cu.import("resource:///modules/experiments/Experiments.jsm", scope);
      let experiments = scope.Experiments.instance()
      let activeExperiment = experiments.getActiveExperimentID();
      if (activeExperiment) {
        ret.activeExperiment = activeExperiment;
	ret.activeExperimentBranch = experiments.getActiveExperimentBranch();
      }
    } catch(e) {
      
    }

    return ret;
  },

  


  gatherMemory: function gatherMemory() {
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

  


  gatherStartupHistograms: function gatherStartupHistograms() {
    let info = Telemetry.registeredHistograms([]);
    let snapshots = Telemetry.histogramSnapshots;
    for (let name of info) {
      
      if (this.isInterestingStartupHistogram(name) && name in snapshots) {
        Telemetry.histogramFrom("STARTUP_" + name, name);
      }
    }
  },

  





  assemblePayloadWithMeasurements: function assemblePayloadWithMeasurements(simpleMeasurements, info) {
    let payloadObj = {
      ver: PAYLOAD_VERSION,
      simpleMeasurements: simpleMeasurements,
      histograms: this.getHistograms(Telemetry.histogramSnapshots),
      slowSQL: Telemetry.slowSQL,
      fileIOReports: Telemetry.fileIOReports,
      chromeHangs: Telemetry.chromeHangs,
      threadHangStats: this.getThreadHangStats(Telemetry.threadHangStats),
      lateWrites: Telemetry.lateWrites,
      addonHistograms: this.getAddonHistograms(),
      addonDetails: AddonManagerPrivate.getTelemetryDetails(),
      UIMeasurements: UITelemetry.getUIMeasurements(),
      log: TelemetryLog.entries(),
      info: info
    };

    if (Object.keys(this._slowSQLStartup).length != 0 &&
        (Object.keys(this._slowSQLStartup.mainThread).length ||
         Object.keys(this._slowSQLStartup.otherThreads).length)) {
      payloadObj.slowSQLStartup = this._slowSQLStartup;
    }

    return payloadObj;
  },

  getSessionPayload: function getSessionPayload(reason) {
    let measurements = this.getSimpleMeasurements(reason == "saved-session");
    let info = this.getMetadata(reason);
    return this.assemblePayloadWithMeasurements(measurements, info);
  },

  assemblePing: function assemblePing(payloadObj, reason) {
    let slug = this._uuid;
    return { slug: slug, reason: reason, payload: payloadObj };
  },

  getSessionPayloadAndSlug: function getSessionPayloadAndSlug(reason) {
    return this.assemblePing(this.getSessionPayload(reason), reason);
  },

  popPayloads: function popPayloads(reason) {
    function payloadIter() {
      if (reason != "overdue-flush") {
        yield this.getSessionPayloadAndSlug(reason);
      }
      let iterator = TelemetryFile.popPendingPings(reason);
      for (let data of iterator) {
        yield data;
      }
    }

    let payloadIterWithThis = payloadIter.bind(this);
    return { __iterator__: payloadIterWithThis };
  },

  


  send: function send(reason, server) {
    
    this.gatherMemory();
    return this.sendPingsFromIterator(server, reason,
                               Iterator(this.popPayloads(reason)));
  },

  sendPingsFromIterator: function sendPingsFromIterator(server, reason, i) {
    let p = [data for (data in i)].map((data) =>
      this.doPing(server, data).then(null, () => TelemetryFile.savePing(data, true)));

    return Promise.all(p);
  },

  finishPingRequest: function finishPingRequest(success, startTime, ping) {
    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    hsuccess.add(success);
    hping.add(new Date() - startTime);

    if (success) {
      return TelemetryFile.cleanupPingFile(ping);
    } else {
      return Promise.resolve();
    }
  },

  submissionPath: function submissionPath(ping) {
    let slug;
    if (!ping) {
      slug = this._uuid;
    } else {
      let info = ping.payload.info;
      let pathComponents = [ping.slug, info.reason, info.appName,
                            info.appVersion, info.appUpdateChannel,
                            info.appBuildID];
      slug = pathComponents.join("/");
    }
    return "/submit/telemetry/" + slug;
  },

  doPing: function doPing(server, ping) {
    let deferred = Promise.defer();
    let url = server + this.submissionPath(ping);
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json; charset=UTF-8");

    let startTime = new Date();

    function handler(success) {
      return function(event) {
        this.finishPingRequest(success, startTime, ping).then(() => {
          if (success) {
            deferred.resolve();
          } else {
            deferred.reject(event);
          }
        });
      };
    }
    request.addEventListener("error", handler(false).bind(this), false);
    request.addEventListener("load", handler(true).bind(this), false);

    request.setRequestHeader("Content-Encoding", "gzip");
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let utf8Payload = converter.ConvertFromUnicode(JSON.stringify(ping.payload));
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

  


  setup: function setup(aTesting) {
    
    this._thirdPartyCookies = new ThirdPartyCookieProbe();
    this._thirdPartyCookies.init();

    
    
    let previousBuildID = undefined;
    try {
      previousBuildID = Services.prefs.getCharPref(PREF_PREVIOUS_BUILDID);
    } catch (e) {
      
    }
    let thisBuildID = Services.appinfo.appBuildID;
    
    
    if (previousBuildID != thisBuildID) {
      this._previousBuildID = previousBuildID;
      Services.prefs.setCharPref(PREF_PREVIOUS_BUILDID, thisBuildID);
    }

#ifdef MOZILLA_OFFICIAL
    if (!Telemetry.canSend) {
      
      
      
      Telemetry.canRecord = false;
      return;
    }
#endif
    let enabled = false;
    try {
      enabled = Services.prefs.getBoolPref(PREF_ENABLED);
      this._server = Services.prefs.getCharPref(PREF_SERVER);
    } catch (e) {
      
    }
    if (!enabled) {
      
      
      Telemetry.canRecord = false;
      return;
    }

    AsyncShutdown.sendTelemetry.addBlocker(
      "Telemetry: shutting down",
      function condition(){
        this.uninstall();
        if (Telemetry.canSend) {
          return this.savePendingPings();
        }
      }.bind(this));

    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
    Services.obs.addObserver(this, "quit-application-granted", false);
#ifdef MOZ_WIDGET_ANDROID
    Services.obs.addObserver(this, "application-background", false);
#endif
    Services.obs.addObserver(this, "xul-window-visible", false);
    this._hasWindowRestoredObserver = true;
    this._hasXulWindowVisibleObserver = true;

    
    
    
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let deferred = Promise.defer();

    function timerCallback() {
      Task.spawn(function*(){
        this._initialized = true;

        yield TelemetryFile.loadSavedPings();
        
        
        if (TelemetryFile.pingsOverdue > 0) {
          
          
          
          yield this.send("overdue-flush", this._server);
        }

        this.attachObservers();
        this.gatherMemory();

        Telemetry.asyncFetchTelemetryData(function () {});
        delete this._timer;
        deferred.resolve();
      }.bind(this));
    }

    this._timer.initWithCallback(timerCallback.bind(this),
                                 aTesting ? TELEMETRY_TEST_DELAY : TELEMETRY_DELAY,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
    return deferred.promise;
  },

  testLoadHistograms: function testLoadHistograms(file) {
    return TelemetryFile.testLoadHistograms(file);
  },

  getFlashVersion: function getFlashVersion() {
    let host = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
    let tags = host.getPluginTags();

    for (let i = 0; i < tags.length; i++) {
      if (tags[i].name == "Shockwave Flash")
        return tags[i].version;
    }

    return null;
  },

  savePendingPings: function savePendingPings() {
    let sessionPing = this.getSessionPayloadAndSlug("saved-session");
    return TelemetryFile.savePendingPings(sessionPing);
  },

  testSaveHistograms: function testSaveHistograms(file) {
    return TelemetryFile.savePingToFile(this.getSessionPayloadAndSlug("saved-session"),
      file.path, true);
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
    Services.obs.removeObserver(this, "quit-application-granted");
#ifdef MOZ_WIDGET_ANDROID
    Services.obs.removeObserver(this, "application-background", false);
#endif
  },

  getPayload: function getPayload() {
    
    
    if (Object.keys(this._slowSQLStartup).length == 0) {
      this.gatherStartupHistograms();
      this._slowSQLStartup = Telemetry.slowSQL;
    }
    this.gatherMemory();
    return this.getSessionPayload("gather-payload");
  },

  gatherStartup: function gatherStartup() {
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

  sendIdlePing: function sendIdlePing(aTest, aServer) {
    if (this._isIdleObserver) {
      idleService.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._isIdleObserver = false;
    }
    if (aTest) {
      return this.send("test-ping", aServer);
    } else if (Telemetry.canSend) {
      return this.send("idle-daily", aServer);
    }
  },

  testPing: function testPing(server) {
    return this.sendIdlePing(true, server);
  },

  


  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "profile-after-change":
      return this.setup();
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
    case "idle-daily":
      
      
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
        let ping = this.getSessionPayloadAndSlug("saved-session");
        TelemetryFile.savePing(ping, true);
      }
      break;
#endif
    }
  },
};
