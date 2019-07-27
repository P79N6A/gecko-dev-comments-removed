


const { classes: Cc, utils: Cu, interfaces: Ci, results: Cr } = Components;

Cu.import("resource://gre/modules/TelemetryController.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

const gIsWindows = ("@mozilla.org/windows-registry-key;1" in Cc);
const gIsMac = ("@mozilla.org/xpcom/mac-utils;1" in Cc);
const gIsAndroid =  ("@mozilla.org/android/bridge;1" in Cc);
const gIsGonk = ("@mozilla.org/cellbroadcast/gonkservice;1" in Cc);

const MILLISECONDS_PER_MINUTE = 60 * 1000;
const MILLISECONDS_PER_HOUR = 60 * MILLISECONDS_PER_MINUTE;
const MILLISECONDS_PER_DAY = 24 * MILLISECONDS_PER_HOUR;

const HAS_DATAREPORTINGSERVICE = "@mozilla.org/datareporting/service;1" in Cc;

const UUID_REGEX = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

let gOldAppInfo = null;
let gGlobalScope = this;






function decodeRequestPayload(request) {
  let s = request.bodyInputStream;
  let payload = null;
  let decoder = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON)

  if (request.getHeader("content-encoding") == "gzip") {
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
    let converter = scs.asyncConvertData("gzip", "uncompressed",
                                         listener, null);
    converter.onStartRequest(null, null);
    converter.onDataAvailable(null, null, s, 0, s.available());
    converter.onStopRequest(null, null, null);
    let unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    unicodeConverter.charset = "UTF-8";
    let utf8string = unicodeConverter.ConvertToUnicode(observer.buffer);
    utf8string += unicodeConverter.Finish();
    payload = decoder.decode(utf8string);
  } else {
    payload = decoder.decodeFromStream(s, s.available());
  }

  return payload;
}

function loadAddonManager(id, name, version, platformVersion) {
  let ns = {};
  Cu.import("resource://gre/modules/Services.jsm", ns);
  let head = "../../../../mozapps/extensions/test/xpcshell/head_addons.js";
  let file = do_get_file(head);
  let uri = ns.Services.io.newFileURI(file);
  ns.Services.scriptloader.loadSubScript(uri.spec, gGlobalScope);
  createAppInfo(id, name, version, platformVersion);
  startupManager();
}

function createAppInfo(id, name, version, platformVersion) {
  const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
  const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
  let gAppInfo;
  if (!gOldAppInfo) {
    gOldAppInfo = Cc[XULAPPINFO_CONTRACTID].getService(Ci.nsIXULRuntime);
  }

  gAppInfo = {
    
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",

    
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",
    invalidateCachesOnRestart: function invalidateCachesOnRestart() {
      
    },

    
    annotations: {},

    annotateCrashReport: function(key, data) {
      this.annotations[key] = data;
    },

    QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULAppInfo,
                                           Ci.nsIXULRuntime,
                                           Ci.nsICrashReporter,
                                           Ci.nsISupports])
  };

  Object.setPrototypeOf(gAppInfo, gOldAppInfo);

  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return gAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}


function fakeSchedulerTimer(set, clear) {
  let session = Cu.import("resource://gre/modules/TelemetrySession.jsm");
  session.Policy.setSchedulerTickTimeout = set;
  session.Policy.clearSchedulerTickTimeout = clear;
}








function fakeNow(...args) {
  const date = new Date(...args);
  const modules = [
    Cu.import("resource://gre/modules/TelemetrySession.jsm"),
    Cu.import("resource://gre/modules/TelemetryEnvironment.jsm"),
    Cu.import("resource://gre/modules/TelemetryController.jsm"),
    Cu.import("resource://gre/modules/TelemetryStorage.jsm"),
    Cu.import("resource://gre/modules/TelemetrySend.jsm"),
  ];

  for (let m of modules) {
    m.Policy.now = () => date;
  }

  return new Date(date);
}


function fakePingSendTimer(set, clear) {
  let module = Cu.import("resource://gre/modules/TelemetrySend.jsm");
  module.Policy.setPingSendTimeout = set;
  module.Policy.clearPingSendTimeout = clear;
}

function fakeMidnightPingFuzzingDelay(delayMs) {
  let module = Cu.import("resource://gre/modules/TelemetrySend.jsm");
  module.Policy.midnightPingFuzzingDelay = () => delayMs;
}


function futureDate(date, offset) {
  return new Date(date.getTime() + offset);
}

function truncateToDays(aMsec) {
  return Math.floor(aMsec / MILLISECONDS_PER_DAY);
}



function promiseRejects(promise) {
  return promise.then(() => false, () => true);
}

if (runningInParent) {
  
  Services.prefs.setCharPref("toolkit.telemetry.log.level", "Trace");
  
  Services.prefs.setBoolPref("toolkit.telemetry.archive.enabled", true);
}

TelemetryController.initLogging();


fakeSchedulerTimer(() => {}, () => {});
fakePingSendTimer(() => {}, () => {});

fakeMidnightPingFuzzingDelay(0);
