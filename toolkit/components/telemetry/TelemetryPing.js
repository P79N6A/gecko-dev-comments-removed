




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/LightweightThemeManager.jsm");
Cu.import("resource://gre/modules/ctypes.jsm"); 


const PAYLOAD_VERSION = 1;

const PREF_SERVER = "toolkit.telemetry.server";
const PREF_ENABLED = "toolkit.telemetry.enabled";

const TELEMETRY_INTERVAL = 60000;

const TELEMETRY_DELAY = 60000;

const PR_WRONLY = 0x2;
const PR_CREATE_FILE = 0x8;
const PR_TRUNCATE = 0x20;
const PR_EXCL = 0x80;
const RW_OWNER = 0600;
const RWX_OWNER = 0700;













const MEM_HISTOGRAMS = {
  "js-gc-heap": "MEMORY_JS_GC_HEAP",
  "js-compartments/system": "MEMORY_JS_COMPARTMENTS_SYSTEM",
  "js-compartments/user": "MEMORY_JS_COMPARTMENTS_USER",
  "explicit": "MEMORY_EXPLICIT",
  "resident": "MEMORY_RESIDENT",
  "storage-sqlite": "MEMORY_STORAGE_SQLITE",
  "images-content-used-uncompressed":
    "MEMORY_IMAGES_CONTENT_USED_UNCOMPRESSED",
  "heap-allocated": "MEMORY_HEAP_ALLOCATED",
  "heap-committed-unused": "MEMORY_HEAP_COMMITTED_UNUSED",
  "heap-committed-unused-ratio": "MEMORY_HEAP_COMMITTED_UNUSED_RATIO",
  "page-faults-hard": "PAGE_FAULTS_HARD",
  "low-memory-events/virtual": "LOW_MEMORY_EVENTS_VIRTUAL",
  "low-memory-events/physical": "LOW_MEMORY_EVENTS_PHYSICAL",
  "ghost-windows": "GHOST_WINDOWS"
};




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

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}







function getSimpleMeasurements() {
  let si = Services.startup.getStartupInfo();

  var ret = {
    
    uptime: Math.round((new Date() - si.process) / 60000)
  }

  
  var appTimestamps = {};
  try {
    let o = {};
    Cu.import("resource:///modules/TelemetryTimestamps.jsm", o);
    appTimestamps = o.TelemetryTimestamps.get();
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

  ret.startupInterrupted = new Number(Services.startup.interrupted);

  
  let debugService = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
  let isDebuggerAttached = debugService.isDebuggerAttached;
  gWasDebuggerAttached = gWasDebuggerAttached || isDebuggerAttached;
  ret.debuggerAttached = new Number(gWasDebuggerAttached);

  ret.js = Cc["@mozilla.org/js/xpc/XPConnect;1"]
           .getService(Ci.nsIJSEngineTelemetryStats)
           .telemetryValue;

  let shutdownDuration = Services.startup.lastShutdownDuration;
  if (shutdownDuration)
    ret.shutdownDuration = shutdownDuration;

  return ret;
}






function getUpdateChannel() {
  var channel = "default";
  var prefName;
  var prefValue;

  var defaults = Services.prefs.getDefaultBranch(null);
  try {
    channel = defaults.getCharPref("app.update.channel");
  } catch (e) {
    
  }

  try {
    var partners = Services.prefs.getChildList("app.partner.");
    if (partners.length) {
      channel += "-cck";
      partners.sort();

      for each (prefName in partners) {
        prefValue = Services.prefs.getCharPref(prefName);
        channel += "-" + prefValue;
      }
    }
  }
  catch (e) {
    Cu.reportError(e);
  }

  return channel;
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

function TelemetryPing() {}

TelemetryPing.prototype = {
  _histograms: {},
  _initialized: false,
  _prevValues: {},
  
  
  _uuid: generateUUID(),
  
  _startupHistogramRegex: /SQLITE|HTTP|SPDY|CACHE|DNS/,
  _slowSQLStartup: {},
  _prevSession: null,
  _hasWindowRestoredObserver: false,
  _hasXulWindowVisibleObserver: false,
  _pendingPings: [],
  _doLoadSaveNotifications: false,
  _startupIO : {},
  _hashID: Ci.nsICryptoHash.SHA256,
  
  
  _pingsLoaded: 0,
  
  _pingLoadsCompleted: 0,

  
















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
    let info = Telemetry.registeredHistograms;
    let ret = {};

    for (let name in hls) {
      if (info[name]) {
        ret[name] = this.packHistogram(hls[name]);
        let startup_name = "STARTUP_" + name;
        if (hls[startup_name])
          ret[startup_name] = this.packHistogram(hls[startup_name]);
      }
    }

    return ret;
  },

  getAddonHistograms: function getAddonHistograms() {
    let ahs = Telemetry.addonHistogramSnapshots;
    let ret = {};

    for (let addonName in ahs) {
      addonHistograms = ahs[addonName];
      packedHistograms = {};
      for (let name in addonHistograms) {
        packedHistograms[name] = this.packHistogram(addonHistograms[name]);
      }
      if (Object.keys(packedHistograms).length != 0)
        ret[addonName] = packedHistograms;
    }

    return ret;
  },

  addValue: function addValue(name, id, val) {
    let h = this._histograms[name];
    if (!h) {
      h = Telemetry.getHistogramById(id);
      this._histograms[name] = h;
    }
    h.add(val);
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
      appUpdateChannel: getUpdateChannel(),
      platformBuildID: ai.platformBuildID,
      locale: getLocale()
    };

    
    let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
    let fields = ["cpucount", "memsize", "arch", "version", "device", "manufacturer", "hardware",
                  "hasMMX", "hasSSE", "hasSSE2", "hasSSE3",
                  "hasSSSE3", "hasSSE4A", "hasSSE4_1", "hasSSE4_2",
                  "hasEDSP", "hasARMv6", "hasARMv7", "hasNEON"];
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
                     "adapterRAM", "adapterDriver", "adapterDriverVersion",
                     "adapterDriverDate", "adapterDescription2",
                     "adapterVendorID2", "adapterDeviceID2", "adapterRAM2",
                     "adapterDriver2", "adapterDriverVersion2",
                     "adapterDriverDate2", "isGPU2Active", "D2DEnabled;",
                     "DWriteEnabled", "DWriteVersion"
                    ];

    if (gfxInfo) {
      for each (let field in gfxfields) {
        try {
          let value = "";
          value = gfxInfo[field];
          if (value != "")
            ret[field] = value;
        } catch (e) {
          continue
        }
      }
    }

    let theme = LightweightThemeManager.currentTheme;
    if (theme)
      ret.persona = theme.id;

    if (this._addons)
      ret.addons = this._addons;

    let flashVersion = this.getFlashVersion();
    if (flashVersion)
      ret.flashVersion = flashVersion;

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

    let e = mgr.enumerateReporters();
    while (e.hasMoreElements()) {
      let mr = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
      let id = MEM_HISTOGRAMS[mr.path];
      if (!id) {
        continue;
      }

      
      
      try {
        this.handleMemoryReport(id, mr.path, mr.units, mr.amount);
      }
      catch (e) {
      }
    }
  },

  handleMemoryReport: function handleMemoryReport(id, path, units, amount) {
    if (amount == -1) {
      return;
    }

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
      
      

      if (!(path in this._prevValues)) {
        
        
        
        this._prevValues[path] = amount;
        return;
      }

      val = amount - this._prevValues[path];
      this._prevValues[path] = amount;
    }
    else {
      NS_ASSERT(false, "Can't handle memory reporter with units " + units);
      return;
    }

    this.addValue(path, id, val);
  },

  



  isInterestingStartupHistogram: function isInterestingStartupHistogram(name) {
    return this._startupHistogramRegex.test(name);
  },
  
  


  gatherStartupInformation: function gatherStartupInformation() {
    let info = Telemetry.registeredHistograms;
    let snapshots = Telemetry.histogramSnapshots;
    for (let name in info) {
      
      if (this.isInterestingStartupHistogram(name) && name in snapshots) {
        Telemetry.histogramFrom("STARTUP_" + name, name);
      }
    }
    
    this._slowSQLStartup = {mainThread:{}, otherThreads:{}};
  },

  getCurrentSessionPayloadAndSlug: function getCurrentSessionPayloadAndSlug(reason) {
    
    let isTestPing = (reason == "test-ping");
    let payloadObj = {
      ver: PAYLOAD_VERSION,
      simpleMeasurements: getSimpleMeasurements(),
      histograms: this.getHistograms(Telemetry.histogramSnapshots),
      
      slowSQL: {mainThread:{}, otherThreads:{}},
      chromeHangs: Telemetry.chromeHangs,
      addonHistograms: this.getAddonHistograms()
    };

    if (Object.keys(this._slowSQLStartup.mainThread).length
	|| Object.keys(this._slowSQLStartup.otherThreads).length) {
      payloadObj.slowSQLStartup = this._slowSQLStartup;
    }
    
    for (let ioCounter in this._startupIO)
      payloadObj.simpleMeasurements[ioCounter] = this._startupIO[ioCounter];

    let slug = (isTestPing ? reason : this._uuid);
    payloadObj.info = this.getMetadata(reason);
    return { slug: slug, payload: JSON.stringify(payloadObj) };
  },

  getPayloads: function getPayloads(reason) {
    function payloadIter() {
      yield this.getCurrentSessionPayloadAndSlug(reason);

      if (this._pendingPings.length > 0) {
        let data = this._pendingPings.pop();
        
        if (reason == "test-ping") {
          data.slug = reason;
        }
        yield data;
      }
    }

    let payloadIterWithThis = payloadIter.bind(this);
    return { __iterator__: payloadIterWithThis };
  },

  hashString: function hashString(s) {
    let digest = Cc["@mozilla.org/security/hash;1"]
                 .createInstance(Ci.nsICryptoHash);
    digest.init(this._hashID);
    let stream = Cc["@mozilla.org/io/string-input-stream;1"]
                 .createInstance(Ci.nsIStringInputStream);
    stream.data = s;
    digest.updateFromStream(stream, stream.available());
    return digest.finish(true);
  },

  


  send: function send(reason, server) {
    
    this.gatherMemory();
    this.sendPingsFromIterator(server, reason,
                               Iterator(this.getPayloads(reason)));
  },

  











  sendPingsFromIterator: function sendPingsFromIterator(server, reason, i) {
    function finishPings(reason) {
      if (reason == "test-ping") {
        Services.obs.notifyObservers(null, "telemetry-test-xhr-complete", null);
      }
    }

    let data = null;
    try {
      data = i.next();
    } catch (e if e instanceof StopIteration) {
      finishPings(reason);
      return;
    }
    function onSuccess() {
      this.sendPingsFromIterator(server, reason, i);
    }
    function onError() {
      
      finishPings(reason);
    }
    this.doPing(server, data,
                onSuccess.bind(this), onError.bind(this));
  },

  finishPingRequest: function finishPingRequest(success, startTime, ping) {
    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    hsuccess.add(success);
    hping.add(new Date() - startTime);

    if (success) {
      let file = this.saveFileForPing(ping);
      try {
        file.remove(true);
      } catch(e) {
      }
    }
  },

  doPing: function doPing(server, ping, onSuccess, onError) {
    let submitPath = "/submit/telemetry/" + ping.slug;
    let url = server + submitPath;
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json; charset=UTF-8");

    let startTime = new Date();

    function handler(success, callback) {
      return function(event) {
        this.finishPingRequest(success, startTime, ping);
        callback();
      };
    }
    request.addEventListener("error", handler(false, onError).bind(this), false);
    request.addEventListener("load", handler(true, onSuccess).bind(this), false);

    request.setRequestHeader("Content-Encoding", "gzip");
    let payloadStream = Cc["@mozilla.org/io/string-input-stream;1"]
                        .createInstance(Ci.nsIStringInputStream);
    payloadStream.data = this.gzipCompressString(ping.payload);
    request.send(payloadStream);
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

  


  setup: function setup() {
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
    Services.obs.addObserver(this, "private-browsing", false);
    Services.obs.addObserver(this, "profile-before-change", false);
    Services.obs.addObserver(this, "sessionstore-windows-restored", false);
    Services.obs.addObserver(this, "quit-application-granted", false);
    Services.obs.addObserver(this, "xul-window-visible", false);
    this._hasWindowRestoredObserver = true;
    this._hasXulWindowVisibleObserver = true;

    
    
    
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    function timerCallback() {
      this._initialized = true;
      this.attachObservers();
      this.gatherMemory();
      delete this._timer;
    }
    this._timer.initWithCallback(timerCallback.bind(this), TELEMETRY_DELAY,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
    this.loadSavedPings(false);
  },

  verifyPingChecksum: function verifyPingChecksum(ping) {
    
    if (!ping.checksum) {
      return true;
    }

    let checksumNow = this.hashString(ping.payload);
    return ping.checksum == checksumNow;
  },

  addToPendingPings: function addToPendingPings(stream) {
    try {
      let string = NetUtil.readInputStreamToString(stream, stream.available(), { charset: "UTF-8" });
      stream.close();
      let ping = JSON.parse(string);
      this._pingLoadsCompleted++;

      if (this.verifyPingChecksum(ping)) {
        this._pendingPings.push(ping);
      }

      if (this._doLoadSaveNotifications &&
          this._pingLoadsCompleted == this._pingsLoaded) {
        Services.obs.notifyObservers(null, "telemetry-test-load-complete", null);
      }
    } catch (e) {
      
    }
  },

  loadHistograms: function loadHistograms(file, sync) {
    this._pingsLoaded++;
    if (sync) {
      let stream = Cc["@mozilla.org/network/file-input-stream;1"]
                   .createInstance(Ci.nsIFileInputStream);
      stream.init(file, -1, -1, 0);
      this.addToPendingPings(stream);
    } else {
      let channel = NetUtil.newChannel(file);
      channel.contentType = "application/json"

      NetUtil.asyncFetch(channel, (function(stream, result) {
        if (!Components.isSuccessCode(result)) {
          return;
        }
        this.addToPendingPings(stream);
      }).bind(this));
    }
  },

  loadSavedPings: function loadSavedPings(sync) {
    let directory = this.ensurePingDirectory();
    let entries = directory.directoryEntries
                           .QueryInterface(Ci.nsIDirectoryEnumerator);
    this._pingsLoaded = 0;
    this._pingLoadsCompleted = 0;
    try {
      while (entries.hasMoreElements()) {
        this.loadHistograms(entries.nextFile, sync);
      }
    }
    finally {
      entries.close();
    }
  },

  finishTelemetrySave: function finishTelemetrySave(ok, stream) {
    stream.close();
    if (this._doLoadSaveNotifications && ok) {
      Services.obs.notifyObservers(null, "telemetry-test-save-complete", null);
    }
  },

  savePingToFile: function savePingToFile(ping, file, sync, overwrite) {
    let pingString = JSON.stringify(ping);

    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let ostream = Cc["@mozilla.org/network/file-output-stream;1"]
                  .createInstance(Ci.nsIFileOutputStream);
    let initFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    if (!overwrite) {
      initFlags |= PR_EXCL;
    }
    try {
      ostream.init(file, initFlags, RW_OWNER, ostream.DEFER_OPEN);
    } catch (e) {
      
      return;
    }

    if (sync) {
      let utf8String = converter.ConvertFromUnicode(pingString);
      utf8String += converter.Finish();
      let amount = ostream.write(utf8String, utf8String.length);
      this.finishTelemetrySave(amount == utf8String.length, ostream);
    } else {
      let istream = converter.convertToInputStream(pingString)
      let self = this;
      NetUtil.asyncCopy(istream, ostream,
                        function(result) {
                          self.finishTelemetrySave(Components.isSuccessCode(result),
                                                   ostream);
                        });
    }
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

  ensurePingDirectory: function ensurePingDirectory() {
    let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    let directory = profileDirectory.clone();
    directory.append("saved-telemetry-pings");
    if (directory.exists()) {
      if (directory.isDirectory()) {
        
        
        directory.permissions = RWX_OWNER;
        return directory;
      } else {
        directory.remove(true);
      }
    }

    directory.create(Ci.nsIFile.DIRECTORY_TYPE, RWX_OWNER);
    return directory;
  },

  saveFileForPing: function saveFileForPing(ping) {
    if (!('checksum' in ping)) {
      ping.checksum = this.hashString(ping.payload);
    }
    let file = this.ensurePingDirectory();
    file.append(ping.slug);
    return file;
  },

  savePing: function savePing(ping, overwrite) {
    this.savePingToFile(ping, this.saveFileForPing(ping), true, overwrite);
  },

  savePendingPings: function savePendingPings() {
    let sessionPing = this.getCurrentSessionPayloadAndSlug("saved-session");
    this.savePing(sessionPing, true);
    this._pendingPings.forEach(function sppcb(e, i, a) {
                                 this.savePing(e, false);
                               }, this);
    this._pendingPings = [];
  },

  saveHistograms: function saveHistograms(file, sync) {
    this.savePingToFile(this.getCurrentSessionPayloadAndSlug("saved-session"),
                        file, sync, true);
  },

  


  uninstall: function uninstall() {
    this.detachObservers()
    if (this._hasWindowRestoredObserver) {
      Services.obs.removeObserver(this, "sessionstore-windows-restored");
      this._hasWindowRestoredObserver = false;
    }
    if (this._hasXulWindowVisibleObserver) {
      Services.obs.removeObserver(this, "xul-window-visible");
      this._hasXulWindowVisibleObserver = false;
    }
    Services.obs.removeObserver(this, "profile-before-change");
    Services.obs.removeObserver(this, "private-browsing");
    Services.obs.removeObserver(this, "quit-application-granted");
  },

  


  observe: function (aSubject, aTopic, aData) {
    
    var server = this._server;

    switch (aTopic) {
    case "Add-ons":
      this._addons = aData;
      break;
    case "profile-after-change":
      this.setup();
      break;
    case "profile-before-change":
      this.uninstall();
      break;
    case "cycle-collector-begin":
      let now = new Date();
      if (!gLastMemoryPoll
          || (TELEMETRY_INTERVAL <= now - gLastMemoryPoll)) {
        gLastMemoryPoll = now;
        this.gatherMemory();
      }
      break;
    case "private-browsing":
      Telemetry.canRecord = aData == "exit";
      if (aData == "enter") {
        this.detachObservers()
      } else {
        this.attachObservers()
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
      
    case "test-gather-startup":
      var counters = processInfo.getCounters();
      if (counters) {  
        [this._startupIO.startupSessionRestoreReadBytes, 
          this._startupIO.startupSessionRestoreWriteBytes] = counters;
      }
      this.gatherStartupInformation();
      break;
    case "idle-daily":
      
      
      Services.tm.mainThread.dispatch((function() {
        
        Services.obs.notifyObservers(null, "gather-telemetry", null);
        
        idleService.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
        this._isIdleObserver = true;
      }).bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      break;
    case "get-payload":
      this.gatherMemory();
      this.gatherStartupInformation();
      let data = this.getCurrentSessionPayloadAndSlug("gather-payload");

      aSubject.QueryInterface(Ci.nsISupportsString).data = data.payload;
      break;
    case "test-save-histograms":
      this.saveHistograms(aSubject.QueryInterface(Ci.nsIFile), aData != "async");
      break;
    case "test-load-histograms":
      this._pingsLoaded = 0;
      this._pingLoadsCompleted = 0;
      this.loadHistograms(aSubject.QueryInterface(Ci.nsIFile), aData != "async");
      break;
    case "test-enable-load-save-notifications":
      this._doLoadSaveNotifications = true;
      break;
    case "test-ping":
      server = aData;
      
    case "idle":
      if (this._isIdleObserver) {
        idleService.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
        this._isIdleObserver = false;
      }
      if (aTopic == "test-ping") {
        this.send("test-ping", server);
      }
      else if (Telemetry.canSend && aTopic == "idle") {
        this.send("idle-daily", server);
      }
      break;
    case "quit-application-granted":
      this.savePendingPings();
      break;
    }
  },

  classID: Components.ID("{55d6a5fa-130e-4ee6-a158-0133af3b86ba}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([TelemetryPing]);
