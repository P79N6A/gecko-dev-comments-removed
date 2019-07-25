









do_load_httpd_js();
Cu.import("resource://gre/modules/Services.jsm");

const PATH = "/submit/telemetry/test-ping";
const SERVER = "http://localhost:4444";
const IGNORE_HISTOGRAM = "test::ignore_me";

const BinaryInputStream = Components.Constructor(
  "@mozilla.org/binaryinputstream;1",
  "nsIBinaryInputStream",
  "setInputStream");

var httpserver = new nsHttpServer();

function telemetry_ping () {
  const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsIObserver);
  TelemetryPing.observe(null, "test-ping", SERVER);
}

function nonexistentServerObserver(aSubject, aTopic, aData) {
  Services.obs.removeObserver(nonexistentServerObserver, aTopic);

  httpserver.start(4444);

  
  httpserver.registerPathHandler(PATH, function () {});
  Services.obs.addObserver(telemetryObserver, "telemetry-test-xhr-complete", false);
  telemetry_ping();
}

function telemetryObserver(aSubject, aTopic, aData) {
  Services.obs.removeObserver(telemetryObserver, aTopic);
  httpserver.registerPathHandler(PATH, checkHistograms);
  const Telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
  Telemetry.newHistogram(IGNORE_HISTOGRAM, 1, 2, 3, Telemetry.HISTOGRAM_BOOLEAN);
  Services.startup.interrupted = true;
  telemetry_ping();
}

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
  Services.obs.addObserver(nonexistentServerObserver, "telemetry-test-xhr-complete", false);
  telemetry_ping();
  
  do_test_pending();
}

function readBytesFromInputStream(inputStream, count) {
  if (!count) {
    count = inputStream.available();
  }
  return new BinaryInputStream(inputStream).readBytes(count);
}

function checkHistograms(request, response) {
  
  httpserver.stop(do_test_finished);
  let s = request.bodyInputStream
  let payload = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON)
                                             .decode(readBytesFromInputStream(s))

  do_check_eq(request.getHeader("content-type"), "application/json; charset=UTF-8");
  do_check_true(payload.simpleMeasurements.uptime >= 0)
  do_check_true(payload.simpleMeasurements.startupInterrupted === 1);
  
  const expected_info = {
    reason: "test-ping",
    OS: "XPCShell", 
    appID: "xpcshell@tests.mozilla.org", 
    appVersion: "1", 
    appName: "XPCShell", 
    appBuildID: "2007010101",
    platformBuildID: "2007010101"
  };

  for (let f in expected_info) {
    do_check_eq(payload.info[f], expected_info[f]);
  }

  const TELEMETRY_PING = "TELEMETRY_PING";
  const TELEMETRY_SUCCESS = "TELEMETRY_SUCCESS";
  do_check_true(TELEMETRY_PING in payload.histograms);
  do_check_false(IGNORE_HISTOGRAM in payload.histograms);

  
  const expected_tc = {
    range: [1, 2],
    bucket_count: 3,
    histogram_type: 2,
    values: {0:1, 1:1, 2:0},
    sum: 1
  }
  let tc = payload.histograms[TELEMETRY_SUCCESS]
  do_check_eq(uneval(tc), 
              uneval(expected_tc));
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
