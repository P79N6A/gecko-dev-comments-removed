




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PREF_SERVER = "toolkit.telemetry.server";
const PREF_ENABLED = "toolkit.telemetry.enabled";

const TELEMETRY_INTERVAL = 60;

const TELEMETRY_DELAY = 60000;

const MEM_HISTOGRAMS = {
  "heap-used/js/gc-heap": [1024, 1024 * 500, 10],
  "mapped/heap/used": [1024, 2 * 1024 * 1024, 10],
  "heap-used/layout/all": [1024, 50 * 1025, 10]
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
      values: {}
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
  return Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator)
         .generateUUID().toString();
}




function getMetadata(reason) {
  let si = Cc["@mozilla.org/toolkit/app-startup;1"].
           getService(Ci.nsIAppStartup).getStartupInfo();
  let ai = Services.appinfo;
  let ret = {
    uptime: (new Date() - si.process),
    reason: reason,
    OS: ai.OS,
    XPCOMABI: ai.XPCOMABI,
    ID: ai.ID,
    vesion: ai.version,
    name: ai.name,
    appBuildID: ai.appBuildID,
    platformBuildID: ai.platformBuildID,
  }
  return ret;
}

function TelemetryPing() {}

TelemetryPing.prototype = {
  _histograms: {},

  


  gatherMemory: function gatherMemory() {
    let mgr;
    try {
      mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
            getService(Ci.nsIMemoryReporterManager);
    } catch (e) {
      
      return
    }

    let e = mgr.enumerateReporters();
    let memReporters = {};
    while (e.hasMoreElements()) {
      let mr = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
      
      let specs = MEM_HISTOGRAMS[mr.path];
      if (!specs) {
        continue;
      }

      let name = "Memory:" + mr.path + " (KB)";
      let h = this._histograms[name];
      if (!h) {
        h = Telemetry.newExponentialHistogram(name, specs[0], specs[1], specs[2]);
        this._histograms[name] = h;
      }
      let v = Math.floor(mr.memoryUsed / 1024);
      h.add(v);
    }
    return memReporters;
  },
  
  


  send: function send(reason, server) {
    
    this.gatherMemory();
    let nativeJSON = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    let payload = {
      info: getMetadata(reason),
      histograms: getHistograms()
    };
    let isTestPing = (reason == "test-ping");
    
    
    if (!this._path)
      this._path = "/submit/telemetry/" + (isTestPing ? reason : generateUUID());
    
    const TELEMETRY_PING = "telemetry.ping (ms)";
    const TELEMETRY_SUCCESS = "telemetry.success (No, Yes)";

    let hping = Telemetry.newExponentialHistogram(TELEMETRY_PING, 1, 3000, 10);
    let hsuccess = Telemetry.newLinearHistogram(TELEMETRY_SUCCESS, 1, 2, 3);

    let url = server + this._path;
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.open("POST", url, true);
    request.overrideMimeType("text/plain");

    let startTime = new Date()

    function finishRequest(success_metric) {
      hsuccess.add(success_metric);
      hping.add(new Date() - startTime);
      if (isTestPing)
        Services.obs.notifyObservers(null, "telemetry-test-xhr-complete", null);
    }
    request.onerror = function(aEvent) finishRequest(1);
    request.onload = function(aEvent) finishRequest(2);
    request.send(nativeJSON.encode(payload));
  },
  
  


  setup: function setup() {
    let enabled = false; 
    try {
      enabled = Services.prefs.getBoolPref(PREF_ENABLED);
      this._server = Services.prefs.getCharPref(PREF_SERVER);
    } catch (e) {
      
    }
    if (!enabled) 
      return;
  
    let self = this;
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    let timerCallback = function() {
      let idleService = Cc["@mozilla.org/widget/idleservice;1"].
                        getService(Ci.nsIIdleService);
      idleService.addIdleObserver(self, TELEMETRY_INTERVAL); 
      self.gatherMemory();
    }
    this._timer.initWithCallback(timerCallback, TELEMETRY_DELAY, Ci.nsITimer.TYPE_ONE_SHOT);
  },

  


  observe: function (aSubject, aTopic, aData) {
    
    var server = this._server;

    switch (aTopic) {
    case "profile-after-change":
      this.setup();
      break;
    case "idle":
      this.gatherMemory();
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
