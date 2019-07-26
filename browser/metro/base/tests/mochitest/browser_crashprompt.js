




"use strict";

const Ci = Components.interfaces;
const Cm = Components.manager;
const Cc = Components.classes;

const CONTRACT_ID = "@mozilla.org/xre/runtime;1";

var gAppInfoClassID, gIncOldFactory;

var gMockAppInfoQueried = false;

function MockAppInfo() {
}

var newFactory = {
  createInstance: function(aOuter, aIID) {
    if (aOuter)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return new MockAppInfo().QueryInterface(aIID);
  },
  lockFactory: function(aLock) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory])
};

let oldPrompt;
function initMockAppInfo() {
  var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
  gAppInfoClassID = registrar.contractIDToCID(CONTRACT_ID);
  gIncOldFactory = Cm.getClassObject(Cc[CONTRACT_ID], Ci.nsIFactory);
  registrar.unregisterFactory(gAppInfoClassID, gIncOldFactory);
  var components = [MockAppInfo];
  registrar.registerFactory(gAppInfoClassID, "", CONTRACT_ID, newFactory);
  gIncOldFactory = Cm.getClassObject(Cc[CONTRACT_ID], Ci.nsIFactory);

  oldPrompt = Services.prompt;
}

function cleanupMockAppInfo() {
  if (gIncOldFactory) {
    var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.unregisterFactory(gAppInfoClassID, newFactory);
    registrar.registerFactory(gAppInfoClassID, "", CONTRACT_ID, gIncOldFactory);
  }
  gIncOldFactory = null;

  Services.prompt = oldPrompt;
}

MockAppInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULRuntime]),
  get lastRunCrashID() {
    gMockAppInfoQueried = true;
    return this.crashid;
  },
}

gTests.push({
  desc: "Test Crash Prompt",
  setUp: initMockAppInfo,
  tearDown: cleanupMockAppInfo,

  get autosubmitPref() {
    return Services.prefs.getBoolPref("app.crashreporter.autosubmit");
  },
  set autosubmitPref(aValue) {
    Services.prefs.setBoolPref('app.crashreporter.autosubmit', aValue);
  },
  get promptedPref() {
    return Services.prefs.getBoolPref("app.crashreporter.prompted");
  },
  set promptedPref(aValue) {
    Services.prefs.setBoolPref('app.crashreporter.prompted', aValue);
  },

  run: function() {
    MockAppInfo.crashid = "testid";

    
    
    Services.prompt = {
      confirmEx: function() {
        return this.retVal;
      }
    };

    
    

    
    this.autosubmitPref = false;
    this.promptedPref = false;

    
    
    Services.prompt.retVal = 1;

    BrowserUI.startupCrashCheck();
    ok(!this.autosubmitPref, "auto submit disabled?");
    ok(this.promptedPref, "prompted should be true");


    
    

    
    this.autosubmitPref = false;
    this.promptedPref = false;

    
    gMockAppInfoQueried = false;

    
    
    Services.prompt.retVal = 0;

    BrowserUI.startupCrashCheck();
    ok(gMockAppInfoQueried, "id queried");
    ok(this.autosubmitPref, "auto submit enabled?");
    ok(this.promptedPref, "prompted should be true");


    
    
    Services.prompt.confirmEx = function() {
      ok(false, "Should not attempt to launch crash reporter prompt");
    };

    
    
    
    this.autosubmitPref = false;
    this.promptedPref = true;
    BrowserUI.startupCrashCheck();

    
    
    this.autosubmitPref = true;
    this.promptedPref = false;
    BrowserUI.startupCrashCheck();
  }
});

function test() {
  if (!CrashReporter.enabled) {
    info("crash reporter prompt tests didn't run, CrashReporter.enabled is false.");
    return;
  }
  runTests();
}
