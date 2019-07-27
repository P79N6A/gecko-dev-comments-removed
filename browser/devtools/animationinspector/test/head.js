



"use strict";

const Cu = Components.utils;
const {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const TargetFactory = devtools.TargetFactory;
const {console} = Components.utils.import("resource://gre/modules/devtools/Console.jsm", {});
const {ViewHelpers} = Cu.import("resource:///modules/devtools/ViewHelpers.jsm", {});


waitForExplicitFinish();

const TEST_URL_ROOT = "http://example.com/browser/browser/devtools/animationinspector/test/";
const ROOT_TEST_DIR = getRootDirectory(gTestPath);
const FRAME_SCRIPT_URL = ROOT_TEST_DIR + "doc_frame_script.js";
const COMMON_FRAME_SCRIPT_URL = "chrome://browser/content/devtools/frame-script-utils.js";


registerCleanupFunction(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});








gDevTools.testing = true;
registerCleanupFunction(() => gDevTools.testing = false);



registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.debugger.log");
});






function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  window.focus();

  let tab = window.gBrowser.selectedTab = window.gBrowser.addTab(url);
  let browser = tab.linkedBrowser;

  info("Loading the helper frame script " + FRAME_SCRIPT_URL);
  browser.messageManager.loadFrameScript(FRAME_SCRIPT_URL, false);

  info("Loading the helper frame script " + COMMON_FRAME_SCRIPT_URL);
  browser.messageManager.loadFrameScript(COMMON_FRAME_SCRIPT_URL, false);

  browser.addEventListener("load", function onload() {
    browser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");

    def.resolve(tab);
  }, true);

  return def.promise;
}




function reloadTab() {
  return executeInContent("devtools:test:reload", {}, {}, false);
}








function getNodeFront(selector, {walker}) {
  return walker.querySelector(walker.rootNode, selector);
}














let selectNode = Task.async(function*(data, inspector, reason="test") {
  info("Selecting the node for '" + data + "'");
  let nodeFront = data;
  if (!data._form) {
    nodeFront = yield getNodeFront(data, inspector);
  }
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(nodeFront, reason);
  yield updated;
});






let openAnimationInspector = Task.async(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  info("Opening the toolbox with the inspector selected");
  let toolbox = yield gDevTools.showToolbox(target, "inspector");
  yield waitForToolboxFrameFocus(toolbox);

  info("Switching to the animationinspector");
  let inspector = toolbox.getPanel("inspector");
  let initPromises = [
    inspector.once("inspector-updated"),
    inspector.sidebar.once("animationinspector-ready")
  ];
  inspector.sidebar.select("animationinspector");

  info("Waiting for the inspector and sidebar to be ready");
  yield promise.all(initPromises);

  let win = inspector.sidebar.getWindowForTab("animationinspector");
  let {AnimationsController, AnimationsPanel} = win;

  info("Waiting for the animation controller and panel to be ready");
  if (AnimationsPanel.initialized) {
    yield AnimationsPanel.initialized;
  } else {
    yield AnimationsPanel.once(AnimationsPanel.PANEL_INITIALIZED);
  }

  return {
    toolbox: toolbox,
    inspector: inspector,
    controller: AnimationsController,
    panel: AnimationsPanel,
    window: win
  };
});






function waitForToolboxFrameFocus(toolbox) {
  info("Making sure that the toolbox's frame is focused");
  let def = promise.defer();
  let win = toolbox.frame.contentWindow;
  waitForFocus(def.resolve, win);
  return def.promise;
}








function hasSideBarTab(inspector, id) {
  return !!inspector.sidebar.getWindowForTab(id);
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
        target[remove](eventName, onEvent, useCapture);
        deferred.resolve.apply(deferred, aArgs);
      }, useCapture);
      break;
    }
  }

  return deferred.promise;
}








function waitForContentMessage(name) {
  info("Expecting message " + name + " from content");

  let mm = gBrowser.selectedBrowser.messageManager;

  let def = promise.defer();
  mm.addMessageListener(name, function onMessage(msg) {
    mm.removeMessageListener(name, onMessage);
    def.resolve(msg.data);
  });
  return def.promise;
}













function executeInContent(name, data={}, objects={}, expectResponse=true) {
  info("Sending message " + name + " to content");
  let mm = gBrowser.selectedBrowser.messageManager;

  mm.sendAsyncMessage(name, data, objects);
  if (expectResponse) {
    return waitForContentMessage(name);
  } else {
    return promise.resolve();
  }
}

function onceNextPlayerRefresh(player) {
  let onRefresh = promise.defer();
  player.once(player.AUTO_REFRESH_EVENT, onRefresh.resolve);
  return onRefresh.promise;
}




let togglePlayPauseButton = Task.async(function*(widget) {
  let nextState = widget.player.state.playState === "running" ? "paused" : "running";

  
  
  
  
  let onClicked = widget.onPlayPauseBtnClick();

  
  
  ok(widget.el.classList.contains(nextState),
    "The button's state was changed in the UI before the request was sent");

  yield onClicked;

  
  yield waitForStateCondition(widget.player, state => {
    return state.playState === nextState;
  }, "after clicking the toggle button");
});












let waitForStateCondition = Task.async(function*(player, conditionCheck, desc="") {
  if (desc) {
    desc = "(" + desc + ")";
  }
  info("Waiting for a player's auto-refresh event " + desc);
  let def = promise.defer();
  player.on(player.AUTO_REFRESH_EVENT, function onNewState() {
    info("State refreshed, checking condition ... " + desc);
    if (conditionCheck(player.state)) {
      player.off(player.AUTO_REFRESH_EVENT, onNewState);
      def.resolve();
    }
  });
  return def.promise;
});




let getAnimationPlayerState = Task.async(function*(selector, animationIndex=0) {
  let playState = yield executeInContent("Test:GetAnimationPlayerState",
                                         {selector, animationIndex});
  return playState;
});








let checkPausedAt = Task.async(function*(widget, time) {
  info("Wait for the next auto-refresh");

  yield waitForStateCondition(widget.player, state => {
    return state.playState === "paused";
  }, "waiting for the player to pause");

  ok(widget.el.classList.contains("paused"), "The widget is in paused mode");
  is(widget.player.state.currentTime, time,
    "The player front's currentTime was set to " + time);
  is(widget.currentTimeEl.value, time, "The input's value was set to " + time);
});






function isNodeVisible(node) {
  return !!node.getClientRects().length;
}
