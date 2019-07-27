


Components.utils.import("resource://gre/modules/TelemetryPing.jsm", this);
Components.utils.import("resource://gre/modules/Services.jsm", this);

let gOldAppInfo = null;
let gGlobalScope = this;

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
    gOldAppInfo = Components.classes[XULAPPINFO_CONTRACTID]
                            .getService(Components.interfaces.nsIXULRuntime);
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
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return gAppInfo.QueryInterface(iid);
    }
  };
  var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}


function fakeDailyTimers(set, clear) {
  let session = Components.utils.import("resource://gre/modules/TelemetrySession.jsm");
  session.Policy.setDailyTimeout = set;
  session.Policy.clearDailyTimeout = clear;
}


Services.prefs.setCharPref("toolkit.telemetry.log.level", "Trace");
Services.prefs.setBoolPref("toolkit.telemetry.log.dump", true);
TelemetryPing.initLogging();


fakeDailyTimers(() => {}, () => {});
