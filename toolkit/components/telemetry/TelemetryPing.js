




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");


const PAYLOAD_VERSION = 1;

const PREF_SERVER = "toolkit.telemetry.server";
const PREF_ENABLED = "toolkit.telemetry.enabled";

const TELEMETRY_INTERVAL = 60;

const TELEMETRY_DELAY = 60000;

const MEM_HISTOGRAMS = {
  "explicit/js/gc-heap": "MEMORY_JS_GC_HEAP",
  "resident": "MEMORY_RESIDENT",
  "explicit/layout/all": "MEMORY_LAYOUT_ALL",
  "hard-page-faults": "HARD_PAGE_FAULTS"
};

XPCOMUtils.defineLazyGetter(this, "Telemetry", function () {
  return Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
});








function getHistograms() {
  let hls = Telemetry.histogramSnapshots;
  let ret = {};

  for (let key in hls) {
    let hgram = hls[key];
    let r = hgram.ranges;
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
    ret[key] = retgram;
  }
  return ret;
}

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}










function getMetadata(reason) {
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
  let fields = ["cpucount", "memsize", "arch", "version", "device", "manufacturer", "hardware"];
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
  return ret;
}







function getSimpleMeasurements() {
  let si = Cc["@mozilla.org/toolkit/app-startup;1"].
           getService(Ci.nsIAppStartup).getStartupInfo();

  var ret = {
    
    uptime: Math.round((new Date() - si.process) / 60000)
  }
  for each (let field in ["main", "firstPaint", "sessionRestored"]) {
    if (!(field in si))
      continue;
    ret[field] = si[field] - si.process
  }
  return ret;
}

function TelemetryPing() {}

TelemetryPing.prototype = {
  _histograms: {},
  _prevValues: {},

  


  gatherMemory: function gatherMemory() {
    let mgr;
    try {
      mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
            getService(Ci.nsIMemoryReporterManager);
    } catch (e) {
      
      return;
    }

    let e = mgr.enumerateReporters();
    let memReporters = {};
    while (e.hasMoreElements()) {
      let mr = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
      
      let id = MEM_HISTOGRAMS[mr.path];
      if (!id) {
        continue;
      }

      let val;
      if (mr.units == Ci.nsIMemoryReporter.UNITS_BYTES) {
        val = Math.floor(mr.amount / 1024);
      }
      else if (mr.units == Ci.nsIMemoryReporter.UNITS_COUNT) {
        
        

        
        let curVal = mr.amount;
        let prevVal = this._prevValues[mr.path];
        if (!prevVal) {
          
          
          
          this._prevValues[mr.path] = curVal;
          continue;
        }
        val = curVal - prevVal;
        this._prevValues[mr.path] = curVal;
      }
      else {
        NS_ASSERT(false, "Can't handle memory reporter with units " + mr.units);
        continue;
      }

      let h = this._histograms[mr.name];
      if (!h) {
        h = Telemetry.getHistogramById(id);
        this._histograms[mr.name] = h;
      }
      h.add(val);
    }
    return memReporters;
  },
  
  


  send: function send(reason, server) {
    
    this.gatherMemory();
    let nativeJSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    let payload = {
      ver: PAYLOAD_VERSION,
      info: getMetadata(reason),
      simpleMeasurements: getSimpleMeasurements(),
      histograms: getHistograms()
    };
    let isTestPing = (reason == "test-ping");
    
    
    if (!this._path)
      this._path = "/submit/telemetry/" + (isTestPing ? reason : generateUUID());
    
    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    let url = server + this._path;
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json");

    let startTime = new Date();

    function finishRequest(channel) {
      let success = false;
      try {
        success = channel.QueryInterface(Ci.nsIHttpChannel).requestSucceeded;
      } catch(e) {
      }
      hsuccess.add(success);
      hping.add(new Date() - startTime);
      if (isTestPing)
        Services.obs.notifyObservers(null, "telemetry-test-xhr-complete", null);
    }
    request.onerror = function(aEvent) finishRequest(request.channel);
    request.onload = function(aEvent) finishRequest(request.channel);

    request.send(nativeJSON.encode(payload));
  },
  
  attachObservers: function attachObservers() {
    if (!this._initialized)
      return;
    let idleService = Cc["@mozilla.org/widget/idleservice;1"].
                      getService(Ci.nsIIdleService);
    idleService.addIdleObserver(this, TELEMETRY_INTERVAL);
    Services.obs.addObserver(this, "idle-daily", false);
  },

  detachObservers: function detachObservers() {
    if (!this._initialized)
      return;
    let idleService = Cc["@mozilla.org/widget/idleservice;1"].
                      getService(Ci.nsIIdleService);
    idleService.removeIdleObserver(this, TELEMETRY_INTERVAL);
    Services.obs.removeObserver(this, "idle-daily");
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

    
    
    
    let self = this;
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let timerCallback = function() {
      self._initialized = true;
      self.attachObservers();
      self.gatherMemory();
      delete self._timer
    }
    this._timer.initWithCallback(timerCallback, TELEMETRY_DELAY, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  


  uninstall: function uninstall() {
    this.detachObservers()
    Services.obs.removeObserver(this, "profile-before-change");
    Services.obs.removeObserver(this, "private-browsing");
  },

  


  observe: function (aSubject, aTopic, aData) {
    
    var server = this._server;

    switch (aTopic) {
    case "profile-after-change":
      this.setup();
      break;
    case "profile-before-change":
      this.uninstall();
      break;
    case "idle":
      this.gatherMemory();
      break;
    case "private-browsing":
      Telemetry.canRecord = aData == "exit";
      if (aData == "enter") {
        this.detachObservers()
      } else {
        this.attachObservers()
      }
      break;
    case "test-ping":
      server = aData;
      
    case "idle-daily":
      this.send(aTopic, server);
      break;
    }
  },

  classID: Components.ID("{55d6a5fa-130e-4ee6-a158-0133af3b86ba}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([TelemetryPing]);
