

"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});



let gEnableLogging = Services.prefs.getBoolPref("devtools.debugger.log");
Services.prefs.setBoolPref("devtools.debugger.log", false);


let gToolEnabled = Services.prefs.getBoolPref("devtools.timeline.enabled");
Services.prefs.setBoolPref("devtools.timeline.enabled", true);

let { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
let { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
let { DevToolsUtils } = Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm", {});
let { gDevTools } = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

let TargetFactory = devtools.TargetFactory;
let Toolbox = devtools.Toolbox;

const EXAMPLE_URL = "http://example.com/browser/browser/devtools/timeline/test/";
const SIMPLE_URL = EXAMPLE_URL + "doc_simple-test.html";


waitForExplicitFinish();

registerCleanupFunction(() => {
  info("finish() was called, cleaning up...");
  Services.prefs.setBoolPref("devtools.debugger.log", gEnableLogging);
  Services.prefs.setBoolPref("devtools.timeline.enabled", gToolEnabled);
});

function addTab(url) {
  info("Adding tab: " + url);

  let deferred = promise.defer();
  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let linkedBrowser = tab.linkedBrowser;

  linkedBrowser.addEventListener("load", function onLoad() {
    linkedBrowser.removeEventListener("load", onLoad, true);
    info("Tab added and finished loading: " + url);
    deferred.resolve(tab);
  }, true);

  return deferred.promise;
}

function removeTab(tab) {
  info("Removing tab.");

  let deferred = promise.defer();
  let tabContainer = gBrowser.tabContainer;

  tabContainer.addEventListener("TabClose", function onClose(aEvent) {
    tabContainer.removeEventListener("TabClose", onClose, false);
    info("Tab removed and finished closing.");
    deferred.resolve();
  }, false);

  gBrowser.removeTab(tab);
  return deferred.promise;
}













function* initTimelinePanel(url) {
  info("Initializing a timeline pane.");

  let tab = yield addTab(url);
  let target = TargetFactory.forTab(tab);
  let debuggee = target.window.wrappedJSObject;

  yield target.makeRemote();

  let toolbox = yield gDevTools.showToolbox(target, "timeline");
  let panel = toolbox.getCurrentPanel();
  return [target, debuggee, panel];
}












function* teardown(panel) {
  info("Destroying the specified timeline.");

  let tab = panel.target.tab;
  yield panel._toolbox.destroy();
  yield removeTab(tab);
}









function waitUntil(predicate, interval = 10) {
  if (predicate()) {
    return promise.resolve(true);
  }
  let deferred = promise.defer();
  setTimeout(function() {
    waitUntil(predicate).then(() => deferred.resolve(true));
  }, interval);
  return deferred.promise;
}
