









const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LightweightThemeManager.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PATH = "/submit/telemetry/test-ping";
const SERVER = "http://localhost:4444";
const IGNORE_HISTOGRAM = "test::ignore_me";
const IGNORE_HISTOGRAM_TO_CLONE = "MEMORY_HEAP_ALLOCATED";
const IGNORE_CLONED_HISTOGRAM = "test::ignore_me_also";
const ADDON_NAME = "Telemetry test addon";
const ADDON_HISTOGRAM = "addon-histogram";
const FLASH_VERSION = "1.1.1.1";
const SHUTDOWN_TIME = 10000;
const FAILED_PROFILE_LOCK_ATTEMPTS = 2;


const PR_WRONLY = 0x2;
const PR_CREATE_FILE = 0x8;
const PR_TRUNCATE = 0x20;
const RW_OWNER = 0600;

const BinaryInputStream = Components.Constructor(
  "@mozilla.org/binaryinputstream;1",
  "nsIBinaryInputStream",
  "setInputStream");
const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);

var httpserver = new HttpServer();
var gFinished = false;

function telemetry_ping () {
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  TelemetryPing.gatherStartup();
  TelemetryPing.enableLoadSaveNotifications();
  TelemetryPing.testPing(SERVER);
}


function dummyHandler(request, response) {
  let p = decodeRequestPayload(request);
  return p;
}

function nonexistentServerObserver(aSubject, aTopic, aData) {
  Services.obs.removeObserver(nonexistentServerObserver, aTopic);

  httpserver.start(4444);

  
  httpserver.registerPathHandler(PATH, dummyHandler);
  Services.obs.addObserver(telemetryObserver, "telemetry-test-xhr-complete", false);
  telemetry_ping();
}

function setupTestData() {
  Telemetry.newHistogram(IGNORE_HISTOGRAM, 1, 2, 3, Telemetry.HISTOGRAM_BOOLEAN);
  Telemetry.histogramFrom(IGNORE_CLONED_HISTOGRAM, IGNORE_HISTOGRAM_TO_CLONE);
  Services.startup.interrupted = true;
  Telemetry.registerAddonHistogram(ADDON_NAME, ADDON_HISTOGRAM, 1, 5, 6,
                                   Telemetry.HISTOGRAM_LINEAR);
  h1 = Telemetry.getAddonHistogram(ADDON_NAME, ADDON_HISTOGRAM);
  h1.add(1);
}

function getSavedHistogramsFile(basename) {
  let tmpDir = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let histogramsFile = tmpDir.clone();
  histogramsFile.append(basename);
  if (histogramsFile.exists()) {
    histogramsFile.remove(true);
  }
  do_register_cleanup(function () {
    try {
      histogramsFile.remove(true);
    } catch (e) {
    }
  });
  return histogramsFile;
}

function telemetryObserver(aSubject, aTopic, aData) {
  Services.obs.removeObserver(telemetryObserver, aTopic);
  httpserver.registerPathHandler(PATH, checkHistogramsSync);
  let histogramsFile = getSavedHistogramsFile("saved-histograms.dat");
  setupTestData();

  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  TelemetryPing.saveHistograms(histogramsFile, true);
  TelemetryPing.testLoadHistograms(histogramsFile, true);
  telemetry_ping();
}

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
    payload = decoder.decode(observer.buffer);
  } else {
    payload = decoder.decodeFromStream(s, s.available());
  }

  return payload;
}

function checkPayloadInfo(payload, reason) {
  
  const expected_info = {
    OS: "XPCShell", 
    appID: "xpcshell@tests.mozilla.org", 
    appVersion: "1", 
    appName: "XPCShell", 
    appBuildID: "2007010101",
    platformBuildID: "2007010101",
    flashVersion: FLASH_VERSION
  };

  for (let f in expected_info) {
    do_check_eq(payload.info[f], expected_info[f]);
  }

  do_check_eq(payload.info.reason, reason);
  do_check_true("appUpdateChannel" in payload.info);
  do_check_true("locale" in payload.info);
  do_check_true("revision" in payload.info);
  do_check_true(payload.info.revision.startsWith("http"));

  try {
    
    
    var gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfoDebug);
    var isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
    var isOSX = ("nsILocalFileMac" in Components.interfaces);

    if (isWindows || isOSX) {
      do_check_true("adapterVendorID" in payload.info);
      do_check_true("adapterDeviceID" in payload.info);
    }
  }
  catch (x) {
  }
}

function checkPayload(request, reason, successfulPings) {
  let payload = decodeRequestPayload(request);

  checkPayloadInfo(payload, reason);
  do_check_eq(request.getHeader("content-type"), "application/json; charset=UTF-8");
  do_check_true(payload.simpleMeasurements.uptime >= 0);
  do_check_true(payload.simpleMeasurements.startupInterrupted === 1);
  do_check_eq(payload.simpleMeasurements.shutdownDuration, SHUTDOWN_TIME);
  do_check_eq(payload.simpleMeasurements.savedPings, 1);

  do_check_eq(payload.simpleMeasurements.failedProfileLockCount,
              FAILED_PROFILE_LOCK_ATTEMPTS);
  let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let failedProfileLocksFile = profileDirectory.clone();
  failedProfileLocksFile.append("Telemetry.FailedProfileLocks.txt");
  do_check_true(!failedProfileLocksFile.exists());


  var isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
  if (isWindows) {
    do_check_true(payload.simpleMeasurements.startupSessionRestoreReadBytes > 0);
    do_check_true(payload.simpleMeasurements.startupSessionRestoreWriteBytes > 0);
  }

  const TELEMETRY_PING = "TELEMETRY_PING";
  const TELEMETRY_SUCCESS = "TELEMETRY_SUCCESS";
  const TELEMETRY_TEST_FLAG = "TELEMETRY_TEST_FLAG";
  const READ_SAVED_PING_SUCCESS = "READ_SAVED_PING_SUCCESS";
  do_check_true(TELEMETRY_PING in payload.histograms);
  do_check_true(READ_SAVED_PING_SUCCESS in payload.histograms);
  let rh = Telemetry.registeredHistograms;
  for (let name in rh) {
    if (/SQLITE/.test(name) && name in payload.histograms) {
      do_check_true(("STARTUP_" + name) in payload.histograms); 
    }
  }
  do_check_false(IGNORE_HISTOGRAM in payload.histograms);
  do_check_false(IGNORE_CLONED_HISTOGRAM in payload.histograms);

  
  const expected_flag = {
    range: [1, 2],
    bucket_count: 3,
    histogram_type: 3,
    values: {0:1, 1:0},
    sum: 0,
    sum_squares_lo: 0,
    sum_squares_hi: 0
  };
  let flag = payload.histograms[TELEMETRY_TEST_FLAG];
  do_check_eq(uneval(flag), uneval(expected_flag));

  
  const expected_tc = {
    range: [1, 2],
    bucket_count: 3,
    histogram_type: 2,
    values: {0:1, 1:successfulPings, 2:0},
    sum: successfulPings,
    sum_squares_lo: successfulPings,
    sum_squares_hi: 0
  };
  let tc = payload.histograms[TELEMETRY_SUCCESS];
  do_check_eq(uneval(tc), uneval(expected_tc));

  let h = payload.histograms[READ_SAVED_PING_SUCCESS];
  do_check_eq(h.values[0], 1);

  
  
  
  
  
  
  
  
  

  do_check_true('MEMORY_JS_GC_HEAP' in payload.histograms); 
  do_check_true('MEMORY_JS_COMPARTMENTS_SYSTEM' in payload.histograms); 

  
  do_check_true("addonHistograms" in payload);
  do_check_true(ADDON_NAME in payload.addonHistograms);
  do_check_true(ADDON_HISTOGRAM in payload.addonHistograms[ADDON_NAME]);

  do_check_true(("mainThread" in payload.slowSQL) &&
                ("otherThreads" in payload.slowSQL));
}

function checkPersistedHistogramsSync(request, response) {
  
  
  
  checkPayload(request, "saved-session", 1);

  Services.obs.addObserver(runAsyncTestObserver, "telemetry-test-xhr-complete", false);
}

function checkHistogramsSync(request, response) {
  httpserver.registerPathHandler(PATH, checkPersistedHistogramsSync);
  checkPayload(request, "test-ping", 1);
}

function runAsyncTestObserver(aSubject, aTopic, aData) {
  Services.obs.removeObserver(runAsyncTestObserver, aTopic);
  httpserver.registerPathHandler(PATH, checkHistogramsAsync);
  let histogramsFile = getSavedHistogramsFile("saved-histograms2.dat");

  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aTopic);

    Services.obs.addObserver(function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(arguments.callee, aTopic);
      telemetry_ping();
    }, "telemetry-test-load-complete", false);

    TelemetryPing.testLoadHistograms(histogramsFile, false);
  }, "telemetry-test-save-complete", false);
  TelemetryPing.saveHistograms(histogramsFile, false);
}

function checkPersistedHistogramsAsync(request, response) {
  
  httpserver.stop(do_test_finished);
  
  
  
  checkPayload(request, "saved-session", 3);

  gFinished = true;

  runOldPingFileTest();
}

function checkHistogramsAsync(request, response) {
  httpserver.registerPathHandler(PATH, checkPersistedHistogramsAsync);
  checkPayload(request, "test-ping", 3);
}

function runInvalidJSONTest() {
  let histogramsFile = getSavedHistogramsFile("invalid-histograms.dat");
  writeStringToFile(histogramsFile, "this.is.invalid.JSON");
  do_check_true(histogramsFile.exists());
  
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  TelemetryPing.testLoadHistograms(histogramsFile, true);
  do_check_false(histogramsFile.exists());
}

function runOldPingFileTest() {
  let histogramsFile = getSavedHistogramsFile("old-histograms.dat");
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
  TelemetryPing.saveHistograms(histogramsFile, true);
  do_check_true(histogramsFile.exists());

  let mtime = histogramsFile.lastModifiedTime;
  histogramsFile.lastModifiedTime = mtime - 8 * 24 * 60 * 60 * 1000; 
  TelemetryPing.testLoadHistograms(histogramsFile, true);
  do_check_false(histogramsFile.exists());
}


const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");

function createAppInfo(id, name, version, platformVersion) {
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

  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return gAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}

function dummyTheme(id) {
  return {
    id: id,
    name: Math.random().toString(),
    headerURL: "http://lwttest.invalid/a.png",
    footerURL: "http://lwttest.invalid/b.png",
    textcolor: Math.random().toString(),
    accentcolor: Math.random().toString()
  };
}


var PluginHost = {
  getPluginTags: function(countRef) {
    let plugins = [{name: "Shockwave Flash", version: FLASH_VERSION}];
    countRef.value = plugins.length;
    return plugins;
  },

  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIPluginHost)
     || iid.equals(Ci.nsISupports))
      return this;
  
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

var PluginHostFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return PluginHost.QueryInterface(iid);
  }
};

const PLUGINHOST_CONTRACTID = "@mozilla.org/plugin/host;1";
const PLUGINHOST_CID = Components.ID("{2329e6ea-1f15-4cbe-9ded-6e98e842de0e}");

function registerFakePluginHost() {
  var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(PLUGINHOST_CID, "Fake Plugin Host",
                            PLUGINHOST_CONTRACTID, PluginHostFactory);
}

function writeStringToFile(file, contents) {
  let ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                .createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
	       RW_OWNER, ostream.DEFER_OPEN);
  ostream.write(contents, contents.length);
  ostream.QueryInterface(Ci.nsISafeOutputStream).finish();
  ostream.close();
}

function write_fake_shutdown_file() {
  let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let file = profileDirectory.clone();
  file.append("Telemetry.ShutdownTime.txt");
  let contents = "" + SHUTDOWN_TIME;
  writeStringToFile(file, contents);
}

function write_fake_failedprofilelocks_file() {
  let profileDirectory = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let file = profileDirectory.clone();
  file.append("Telemetry.FailedProfileLocks.txt");
  let contents = "" + FAILED_PROFILE_LOCK_ATTEMPTS;
  writeStringToFile(file, contents);
}

function run_test() {
  do_test_pending();
  try {
    var gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfoDebug);
    gfxInfo.spoofVendorID("0xabcd");
    gfxInfo.spoofDeviceID("0x1234");
  } catch (x) {
    
  }

  
  do_get_profile();
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");

  
  write_fake_failedprofilelocks_file();

  
  write_fake_shutdown_file();

  Telemetry.asyncFetchTelemetryData(function () {
    actualTest();
  });
}

function actualTest() {
  
  let gInternalManager = Cc["@mozilla.org/addons/integration;1"]
                         .getService(Ci.nsIObserver)
                         .QueryInterface(Ci.nsITimerCallback);

  gInternalManager.observe(null, "addons-startup", null);
  LightweightThemeManager.currentTheme = dummyTheme("1234");

  
  registerFakePluginHost();

  runInvalidJSONTest();

  Services.obs.addObserver(nonexistentServerObserver, "telemetry-test-xhr-complete", false);
  telemetry_ping();
  
  do_test_pending();
  
  do_register_cleanup(function () do_check_true(gFinished));
  do_test_finished();
}
