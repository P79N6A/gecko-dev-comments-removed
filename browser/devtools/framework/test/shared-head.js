




const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});
const {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
const {ScratchpadManager} = Cu.import("resource:///modules/devtools/scratchpad-manager.jsm", {});
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {require} = devtools;
const {TargetFactory} = devtools;
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const promise = require("promise");

const TEST_DIR = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
const CHROME_URL_ROOT = TEST_DIR + "/";
const URL_ROOT = CHROME_URL_ROOT.replace("chrome://mochitests/content/", "http://example.com/");


waitForExplicitFinish();




function getFrameScript() {
  let mm = gBrowser.selectedBrowser.messageManager;
  let frameURL = "chrome://browser/content/devtools/frame-script-utils.js";
  mm.loadFrameScript(frameURL, false);
  SimpleTest.registerCleanupFunction(() => {
    mm = null;
  });
  return mm;
}

DevToolsUtils.testing = true;
registerCleanupFunction(() => {
  DevToolsUtils.testing = false;
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.toolbox.host");
  Services.prefs.clearUserPref("devtools.toolbox.previousHost");
});

registerCleanupFunction(function* cleanup() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});






function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    def.resolve(tab);
  }, true);

  return def.promise;
}

function synthesizeKeyFromKeyTag(aKeyId, document) {
  let key = document.getElementById(aKeyId);
  isnot(key, null, "Successfully retrieved the <key> node");

  let modifiersAttr = key.getAttribute("modifiers");

  let name = null;

  if (key.getAttribute("keycode"))
    name = key.getAttribute("keycode");
  else if (key.getAttribute("key"))
    name = key.getAttribute("key");

  isnot(name, null, "Successfully retrieved keycode/key");

  let modifiers = {
    shiftKey: modifiersAttr.match("shift"),
    ctrlKey: modifiersAttr.match("ctrl"),
    altKey: modifiersAttr.match("alt"),
    metaKey: modifiersAttr.match("meta"),
    accelKey: modifiersAttr.match("accel")
  };

  EventUtils.synthesizeKey(name, modifiers);
}









function once(target, eventName, useCapture=false) {
  info("Waiting for event: '" + eventName + "' on " + target + ".");

  let deferred = promise.defer();

  for (let [add, remove] of [
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"],
    ["on", "off"]
  ]) {
    if ((add in target) && (remove in target)) {
      target[add](eventName, function onEvent(...aArgs) {
        info("Got event: '" + eventName + "' on " + target + ".");
        target[remove](eventName, onEvent, useCapture);
        deferred.resolve.apply(deferred, aArgs);
      }, useCapture);
      break;
    }
  }

  return deferred.promise;
}












function loadHelperScript(filePath) {
  let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
  Services.scriptloader.loadSubScript(testDir + "/" + filePath, this);
}

function waitForTick() {
  let deferred = promise.defer();
  executeSoon(deferred.resolve);
  return deferred.promise;
}

function loadToolbox (url) {
  let { promise: p, resolve } = promise.defer();
  gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    gDevTools.showToolbox(target).then(resolve);
  }, true);

  content.location = url;
  return p;
}

function unloadToolbox (toolbox) {
  return toolbox.destroy().then(function() {
    gBrowser.removeCurrentTab();
  });
}
