









const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LightweightThemeManager.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const IGNORE_HISTOGRAM = "test::ignore_me";
const IGNORE_HISTOGRAM_TO_CLONE = "MEMORY_HEAP_ALLOCATED";
const IGNORE_CLONED_HISTOGRAM = "test::ignore_me_also";
const ADDON_NAME = "Telemetry test addon";
const ADDON_HISTOGRAM = "addon-histogram";

const FLASH_VERSION = "\u201c1.1.1.1\u201d";
const SHUTDOWN_TIME = 10000;
const FAILED_PROFILE_LOCK_ATTEMPTS = 2;


const PR_WRONLY = 0x2;
const PR_CREATE_FILE = 0x8;
const PR_TRUNCATE = 0x20;
const RW_OWNER = 0600;

const NUMBER_OF_THREADS_TO_LAUNCH = 30;
var gNumberOfThreadsLaunched = 0;

const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);

var httpserver = new HttpServer();
var serverStarted = false;
var gFinished = false;

function telemetry_ping () {
  TelemetryPing.gatherStartup();
  TelemetryPing.enableLoadSaveNotifications();
  TelemetryPing.cacheProfileDirectory();
  if (serverStarted) {
    TelemetryPing.testPing("http://localhost:" + httpserver.identity.primaryPort);
  } else {
    TelemetryPing.testPing("http://doesnotexist");
  }
}


function dummyHandler(request, response) {
  let p = decodeRequestPayload(request);
  return p;
}

function wrapWithExceptionHandler(f) {
  function wrapper() {
    try {
      f.apply(null, arguments);
    } catch (ex if typeof(ex) == 'object') {
      dump("Caught exception: " + ex.message + "\n");
      dump(ex.stack);
      do_test_finished();
    }
  }
  return wrapper;
}

function addWrappedObserver(f, topic) {
  let wrappedObserver = wrapWithExceptionHandler(f);
  Services.obs.addObserver(function g(aSubject, aTopic, aData) {
    Services.obs.removeObserver(g, aTopic);
    wrappedObserver(aSubject, aTopic, aData);
  }, topic, false);
}

function registerPingHandler(handler) {
  httpserver.registerPrefixHandler("/submit/telemetry/",
				   wrapWithExceptionHandler(handler));
}

function nonexistentServerObserver(aSubject, aTopic, aData) {
  httpserver.start(-1);
  serverStarted = true;

  
  registerPingHandler(dummyHandler);
  addWrappedObserver(telemetryObserver, "telemetry-test-xhr-complete");
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
  registerPingHandler(checkHistogramsSync);
  let histogramsFile = getSavedHistogramsFile("saved-histograms.dat");
  setupTestData();

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
  
  let pathComponents = request.path.split("/").slice(3);

  checkPayloadInfo(payload, reason);
  do_check_eq(reason, pathComponents[1]);
  do_check_eq(request.getHeader("content-type"), "application/json; charset=UTF-8");
  do_check_true(payload.simpleMeasurements.uptime >= 0);
  do_check_true(payload.simpleMeasurements.startupInterrupted === 1);
  do_check_eq(payload.simpleMeasurements.shutdownDuration, SHUTDOWN_TIME);
  do_check_eq(payload.simpleMeasurements.savedPings, 1);
  do_check_true("maximalNumberOfConcurrentThreads" in payload.simpleMeasurements);
  do_check_true(payload.simpleMeasurements.maximalNumberOfConcurrentThreads >= gNumberOfThreadsLaunched);

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

  addWrappedObserver(runAsyncTestObserver, "telemetry-test-xhr-complete");
}

function checkHistogramsSync(request, response) {
  registerPingHandler(checkPersistedHistogramsSync);
  checkPayload(request, "test-ping", 1);
}

function runAsyncTestObserver(aSubject, aTopic, aData) {
  registerPingHandler(checkHistogramsAsync);
  let histogramsFile = getSavedHistogramsFile("saved-histograms2.dat");

  addWrappedObserver(function(aSubject, aTopic, aData) {
    addWrappedObserver(function(aSubject, aTopic, aData) {
      telemetry_ping();
    }, "telemetry-test-load-complete");

    TelemetryPing.testLoadHistograms(histogramsFile, false);
  }, "telemetry-test-save-complete");
  TelemetryPing.saveHistograms(histogramsFile, false);
}

function checkPersistedHistogramsAsync(request, response) {
  
  httpserver.stop(do_test_finished);
  
  
  
  checkPayload(request, "saved-session", 3);

  runOldPingFileTest();

  gFinished = true;
}

function checkHistogramsAsync(request, response) {
  registerPingHandler(checkPersistedHistogramsAsync);
  checkPayload(request, "test-ping", 3);
}

function runInvalidJSONTest() {
  let histogramsFile = getSavedHistogramsFile("invalid-histograms.dat");
  writeStringToFile(histogramsFile, "this.is.invalid.JSON");
  do_check_true(histogramsFile.exists());
  
  TelemetryPing.testLoadHistograms(histogramsFile, true);
  do_check_false(histogramsFile.exists());
}

function runOldPingFileTest() {
  let histogramsFile = getSavedHistogramsFile("old-histograms.dat");
  TelemetryPing.saveHistograms(histogramsFile, true);
  do_check_true(histogramsFile.exists());

  let mtime = histogramsFile.lastModifiedTime;
  histogramsFile.lastModifiedTime = mtime - 8 * 24 * 60 * 60 * 1000; 
  TelemetryPing.testLoadHistograms(histogramsFile, true);
  do_check_false(histogramsFile.exists());
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

  let currentMaxNumberOfThreads = Telemetry.maximalNumberOfConcurrentThreads;
  do_check_true(currentMaxNumberOfThreads > 0);

  
  let threads = [];
  try {
    for (let i = 0; i < currentMaxNumberOfThreads + 10; ++i) {
      threads.push(Services.tm.newThread(0));
    }
  } catch (ex) {
    
  }
  gNumberOfThreadsLaunched = threads.length;

  do_check_true(Telemetry.maximalNumberOfConcurrentThreads >= gNumberOfThreadsLaunched);

  do_register_cleanup(function() {
    threads.forEach(function(thread) {
      thread.shutdown();
    });
  });

  Telemetry.asyncFetchTelemetryData(wrapWithExceptionHandler(actualTest));
}

function actualTest() {
  
  do_register_cleanup(function () do_check_true(gFinished));
  
  let gInternalManager = Cc["@mozilla.org/addons/integration;1"]
                         .getService(Ci.nsIObserver)
                         .QueryInterface(Ci.nsITimerCallback);

  gInternalManager.observe(null, "addons-startup", null);
  LightweightThemeManager.currentTheme = dummyTheme("1234");

  
  registerFakePluginHost();

  runInvalidJSONTest();

  addWrappedObserver(nonexistentServerObserver, "telemetry-test-xhr-complete");
  telemetry_ping();
  
  do_test_pending();
  do_test_finished();
}
