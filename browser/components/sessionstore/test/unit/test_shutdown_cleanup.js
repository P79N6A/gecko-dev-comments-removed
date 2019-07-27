"use strict";








const {XPCOMUtils} = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const {SessionWorker} = Cu.import("resource:///modules/sessionstore/SessionWorker.jsm", {});

const profd = do_get_profile();
const {SessionFile} = Cu.import("resource:///modules/sessionstore/SessionFile.jsm", {});
const {Paths} = SessionFile;

const {OS} = Cu.import("resource://gre/modules/osfile.jsm", {});
const {File} = OS;

const MAX_ENTRIES = 9;
const URL = "http://example.com/#";


let XULAppInfo = {
  vendor: "Mozilla",
  name: "SessionRestoreTest",
  ID: "{230de50e-4cd1-11dc-8314-0800200c9a66}",
  version: "1",
  appBuildID: "2007010101",
  platformVersion: "",
  platformBuildID: "2007010101",
  inSafeMode: false,
  logConsoleErrors: true,
  OS: "XPCShell",
  XPCOMABI: "noarch-spidermonkey",

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIXULAppInfo,
    Ci.nsIXULRuntime,
  ])
};

let XULAppInfoFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return XULAppInfo.QueryInterface(iid);
  }
};

let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{fbfae60b-64a4-44ef-a911-08ceb70b9f31}"),
                          "XULAppInfo", "@mozilla.org/xre/app-info;1",
                          XULAppInfoFactory);

add_task(function* setup() {
  let source = do_get_file("data/sessionstore_valid.js");
  source.copyTo(profd, "sessionstore.js");

  
  yield SessionFile.read();

  
  do_register_cleanup(() => {
    Services.prefs.clearUserPref("browser.sessionstore.max_serialize_back");
    Services.prefs.clearUserPref("browser.sessionstore.max_serialize_forward");
  });
});

function createSessionState(index) {
  
  
  let tabState = {entries: [], index};
  for (let i = 0; i < MAX_ENTRIES; i++) {
    tabState.entries.push({url: URL + i});
  }

  return {windows: [{tabs: [tabState]}]};
}

function* setMaxBackForward(back, fwd) {
  Services.prefs.setIntPref("browser.sessionstore.max_serialize_back", back);
  Services.prefs.setIntPref("browser.sessionstore.max_serialize_forward", fwd);
  yield SessionFile.read();
}

function* writeAndParse(state, path, options = {}) {
  yield SessionWorker.post("write", [state, options]);
  return JSON.parse(yield File.read(path, {encoding: "utf-8"}));
}

add_task(function* test_shistory_cap_none() {
  let state = createSessionState(5);

  
  yield setMaxBackForward(-1, -1);

  
  let diskState = yield writeAndParse(state, Paths.clean, {isFinalWrite: true});
  Assert.deepEqual(state, diskState, "no cap applied");
});

add_task(function* test_shistory_cap_middle() {
  let state = createSessionState(5);
  yield setMaxBackForward(2, 3);

  
  let diskState = yield writeAndParse(state, Paths.recovery);
  Assert.deepEqual(state, diskState, "no cap applied");

  
  
  diskState = yield writeAndParse(state, Paths.clean, {isFinalWrite: true});
  let tabState = state.windows[0].tabs[0];
  tabState.entries = tabState.entries.slice(2, 8);
  tabState.index = 3;
  Assert.deepEqual(state, diskState, "cap applied");
});

add_task(function* test_shistory_cap_lower_bound() {
  let state = createSessionState(1);
  yield setMaxBackForward(5, 5);

  
  let diskState = yield writeAndParse(state, Paths.recovery);
  Assert.deepEqual(state, diskState, "no cap applied");

  
  diskState = yield writeAndParse(state, Paths.clean, {isFinalWrite: true});
  let tabState = state.windows[0].tabs[0];
  tabState.entries = tabState.entries.slice(0, 6);
  Assert.deepEqual(state, diskState, "cap applied");
});

add_task(function* test_shistory_cap_upper_bound() {
  let state = createSessionState(MAX_ENTRIES);
  yield setMaxBackForward(5, 5);

  
  let diskState = yield writeAndParse(state, Paths.recovery);
  Assert.deepEqual(state, diskState, "no cap applied");

  
  
  diskState = yield writeAndParse(state, Paths.clean, {isFinalWrite: true});
  let tabState = state.windows[0].tabs[0];
  tabState.entries = tabState.entries.slice(3);
  tabState.index = 6;
  Assert.deepEqual(state, diskState, "cap applied");
});
