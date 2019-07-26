




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

function initMockAppInfo() {
  var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
  gAppInfoClassID = registrar.contractIDToCID(CONTRACT_ID);
  gIncOldFactory = Cm.getClassObject(Cc[CONTRACT_ID], Ci.nsIFactory);
  registrar.unregisterFactory(gAppInfoClassID, gIncOldFactory);
  var components = [MockAppInfo];
  registrar.registerFactory(gAppInfoClassID, "", CONTRACT_ID, newFactory);
  gIncOldFactory = Cm.getClassObject(Cc[CONTRACT_ID], Ci.nsIFactory);
}

function cleanupMockAppInfo() {
  if (gIncOldFactory) {
    var registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.unregisterFactory(gAppInfoClassID, newFactory);
    registrar.registerFactory(gAppInfoClassID, "", CONTRACT_ID, gIncOldFactory);
  }
  gIncOldFactory = null;
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
  get dialog() {
    return document.getElementById("crash-prompt-dialog");
  },

  run: function() {
    MockAppInfo.crashid = "testid";

    
    this.autosubmitPref = false;
    this.promptedPref = false;

    BrowserUI.startupCrashCheck();

    yield waitForMs(100);

    ok(this.dialog, "prompt dialog exists");
    ok(!this.dialog.hidden, "prompt dialog is visible");

    
    let refuseButton = document.getElementById("crash-button-refuse");
    sendElementTap(window, refuseButton, 20, 20);

    yield waitForCondition(() => this.dialog == null);

    ok(!this.autosubmitPref, "auto submit disabled?");
    ok(this.promptedPref, "prompted should be true");

    
    this.autosubmitPref = false;
    this.promptedPref = false;

    
    gMockAppInfoQueried = false;

    BrowserUI.startupCrashCheck();

    yield waitForMs(100);

    ok(this.dialog, "prompt dialog exists");
    ok(!this.dialog.hidden, "prompt dialog is visible");
    ok(gMockAppInfoQueried, "id queried");

    
    gMockAppInfoQueried = false;

    
    let submitButton = document.getElementById("crash-button-accept");
    sendElementTap(window, submitButton, 20, 20);

    yield waitForCondition(() => this.dialog == null);

    ok(this.autosubmitPref, "auto submit enabled?");
    ok(this.promptedPref, "prompted should be true");
    ok(gMockAppInfoQueried, "id queried");

    
    this.autosubmitPref = false;
    this.promptedPref = true;

    BrowserUI.startupCrashCheck();

    yield waitForMs(100);

    ok(!this.dialog, "prompt dialog does not exists");

    
    this.autosubmitPref = true;
    this.promptedPref = false;

    BrowserUI.startupCrashCheck();

    yield waitForMs(100);

    ok(!this.dialog, "prompt dialog does not exists");
  }
});

function test() {
  if (!CrashReporter.enabled) {
    info("crash reporter prompt tests didn't run, CrashReporter.enabled is false.");
    return;
  }
  runTests();
}
