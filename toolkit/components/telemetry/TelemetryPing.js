



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/LightweightThemeManager.jsm");
Cu.import("resource://gre/modules/ctypes.jsm");


const PAYLOAD_VERSION = 1;

const PREF_SERVER = "toolkit.telemetry.server";
const PREF_ENABLED = "toolkit.telemetry.enabled";

const TELEMETRY_INTERVAL = 60000;

const TELEMETRY_DELAY = 60000;

const MEM_HISTOGRAMS = {
  "js-gc-heap": "MEMORY_JS_GC_HEAP",
  "js-compartments-system": "MEMORY_JS_COMPARTMENTS_SYSTEM",
  "js-compartments-user": "MEMORY_JS_COMPARTMENTS_USER",
  "explicit": "MEMORY_EXPLICIT",
  "resident": "MEMORY_RESIDENT",
  "storage-sqlite": "MEMORY_STORAGE_SQLITE",
  "explicit/images/content/used/uncompressed":
    "MEMORY_IMAGES_CONTENT_USED_UNCOMPRESSED",
  "heap-allocated": "MEMORY_HEAP_ALLOCATED",
  "page-faults-hard": "PAGE_FAULTS_HARD",
  "low-memory-events-virtual": "LOW_MEMORY_EVENTS_VIRTUAL",
  "low-memory-events-physical": "LOW_MEMORY_EVENTS_PHYSICAL"
};



const IDLE_TIMEOUT_SECONDS = 5 * 60;

var gLastMemoryPoll = null;

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

  if (si.process) {
    for each (let field in ["main", "firstPaint", "sessionRestored"]) {
      if (!(field in si))
        continue;
      ret[field] = si[field] - si.process
    }
  }
  ret.startupInterrupted = new Number(Services.startup.interrupted);

  ret.js = Cc["@mozilla.org/js/xpc/XPConnect;1"]
           .getService(Ci.nsIJSEngineTelemetryStats)
           .telemetryValue;

  return ret;
}

function TelemetryPing() {}

TelemetryPing.prototype = {
  _histograms: {},
  _initialized: false,
  _prevValues: {},
  
  
  _uuid: generateUUID(),
  _prevSession: null,

  






  getHistograms: function getHistograms(hls) {
    let info = Telemetry.registeredHistograms;
    let ret = {};

    function processHistogram(name, hgram) {
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
          first = false;
          retgram.values[r[i - 1]] = 0;
        }
        first = false;
        last = i + 1;
        retgram.values[r[i]] = value;
      }

      
      if (last && last < c.length)
        retgram.values[r[last]] = 0;
      ret[name] = retgram;
    };

    for (let name in hls) {
      if (info[name]) {
	processHistogram(name, hls[name]);
	let startup_name = "STARTUP_" + name;
	if (hls[startup_name])
	  processHistogram(startup_name, hls[startup_name]);
      }
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
      platformBuildID: ai.platformBuildID,
    };

    
    let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
    let fields = ["cpucount", "memsize", "arch", "version", "device", "manufacturer", "hardware",
                  "hasMMX", "hasSSE", "hasSSE2", "hasSSE3",
                  "hasSSSE3", "hasSSE4A", "hasSSE4_1", "hasSSE4_2",
                  "hasEDSP", "hasARMv6", "hasNEON"];
    for each (let field in fields) {
      let value;
      try {
        value = sysInfo.getProperty(field);
      } catch (e) {
        continue
      }
      if (field == "memsize") {
        
        
        value = Math.round(value / 1024 / 1024)
      }
      ret[field] = value
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
      
      let amount = mr.amount;
      if (amount == -1) {
        continue;
      }

      let val;
      if (mr.units == Ci.nsIMemoryReporter.UNITS_BYTES) {
        val = Math.floor(amount / 1024);
      }
      else if (mr.units == Ci.nsIMemoryReporter.UNITS_COUNT) {
        val = amount;
      }
      else if (mr.units == Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE) {
        
        

        if (!(mr.path in this._prevValues)) {
          
          
          
          this._prevValues[mr.path] = amount;
          continue;
        }

        val = amount - this._prevValues[mr.path];
        this._prevValues[mr.path] = amount;
      }
      else {
        NS_ASSERT(false, "Can't handle memory reporter with units " + mr.units);
        continue;
      }
      this.addValue(mr.path, id, val);
    }
  },
  
  


  gatherStartupSqlite: function gatherStartupSqlite() {
    let info = Telemetry.registeredHistograms;
    let sqlite_re = /SQLITE/;
    for (let name in info) {
      if (sqlite_re.test(name))
        Telemetry.histogramFrom("STARTUP_" + name, name);
    }
  },

  


  send: function send(reason, server) {
    
    this.gatherMemory();
    let data = this.getSessionPayloadAndSlug(reason);

    
    this.doPing(server, data.slug, data.payload, !data.previous);
    this._prevSession = null;

    
    
    if (data.previous) {
      data = this.getSessionPayloadAndSlug(reason);
      this.doPing(server, data.slug, data.payload, true);
    }
  },

  getSessionPayloadAndSlug: function getSessionPayloadAndSlug(reason) {
    
    let isTestPing = (reason == "test-ping");
    let havePreviousSession = !!this._prevSession;
    let slug = (isTestPing
                ? reason
                : (havePreviousSession
                   ? this._prevSession.uuid
                   : this._uuid));
    let payload = {
      ver: PAYLOAD_VERSION,
      
      info: this.getMetadata(havePreviousSession ? "saved-session" : reason),
    };
    if (havePreviousSession) {
      payload.histograms = this.getHistograms(this._prevSession.snapshots);
    }
    else {
      payload.simpleMeasurements = getSimpleMeasurements();
      payload.histograms = this.getHistograms(Telemetry.histogramSnapshots);
      payload.slowSQL = Telemetry.slowSQL;
    }
    return { previous: !!havePreviousSession, slug: slug, payload: payload };
  },

  doPing: function doPing(server, slug, payload, recordSuccess) {
    let submitPath = "/submit/telemetry/" + slug;
    let url = server + submitPath;
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json");

    let startTime = new Date();
    let file = this.savedHistogramsFile();

    function finishRequest(channel) {
      let success = false;
      try {
        success = channel.QueryInterface(Ci.nsIHttpChannel).requestSucceeded;
      } catch(e) {
      }
      if (recordSuccess) {
        let hping = Telemetry.getHistogramById("TELEMETRY_PING");
        let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

        hsuccess.add(success);
        hping.add(new Date() - startTime);
      }
      if (success && file.exists()) {
        file.remove(true);
      }
      if (slug == "test-ping")
        Services.obs.notifyObservers(null, "telemetry-test-xhr-complete", null);
    }
    request.addEventListener("error", function(aEvent) finishRequest(request.channel), false);
    request.addEventListener("load", function(aEvent) finishRequest(request.channel), false);

    request.send(JSON.stringify(payload));
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

  savedHistogramsFile: function savedHistogramsFile() {
    let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    let profileFile = profileDirectory.clone();

    
    
    
    let size = ctypes.voidptr_t.size;
    
    let uint32_array_t = ctypes.uint32_t.array(1);
    let array = uint32_array_t([0xdeadbeef]);
    let uint8_array_t = ctypes.uint8_t.array(4);
    let array_as_bytes = ctypes.cast(array, uint8_array_t);
    let endian = (array_as_bytes[0] === 0xde) ? "big" : "little"
    let name = "sessionHistograms.dat." + size + endian;
    profileFile.append(name);
    return profileFile;
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

    
    
    
    let self = this;
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let timerCallback = function() {
      self._initialized = true;
      self.attachObservers();
      self.gatherMemory();
      delete self._timer
    }
    this._timer.initWithCallback(timerCallback, TELEMETRY_DELAY, Ci.nsITimer.TYPE_ONE_SHOT);

    
    let loadCallback = function(data) {
      self._prevSession = data;
    }
    Telemetry.loadHistograms(this.savedHistogramsFile(), loadCallback);
  },

  


  uninstall: function uninstall() {
    this.detachObservers()
    Services.obs.removeObserver(this, "sessionstore-windows-restored");
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
    case "sessionstore-windows-restored":
      this.gatherStartupSqlite();
      break;
    case "idle-daily":
      
      
      Services.tm.mainThread.dispatch((function() {
        
        Services.obs.notifyObservers(null, "gather-telemetry", null);
        
        idleService.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
        this._isIdleObserver = true;
      }).bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      break;
    case "test-ping":
      server = aData;
      
    case "idle":
      if (this._isIdleObserver) {
        idleService.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
        this._isIdleObserver = false;
      }
      this.send(aTopic == "idle" ? "idle-daily" : aTopic, server);
      break;
    case "quit-application-granted":
      Telemetry.saveHistograms(this.savedHistogramsFile(),
                               this._uuid, function (success) success,
                               true);
      break;
    }
  },

  classID: Components.ID("{55d6a5fa-130e-4ee6-a158-0133af3b86ba}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([TelemetryPing]);
