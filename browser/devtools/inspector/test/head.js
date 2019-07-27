



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const CC = Components.Constructor;









let TEST_URL_ROOT = "http://example.com/browser/browser/devtools/inspector/test/";
let ROOT_TEST_DIR = getRootDirectory(gTestPath);
let { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});


waitForExplicitFinish();

let {TargetFactory, require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});


let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);


Services.scriptloader.loadSubScript(testDir + "../../../shared/test/test-actor-registry.js", this);

gDevTools.testing = true;
registerCleanupFunction(() => {
  gDevTools.testing = false;
});

registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
});

registerCleanupFunction(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  
  
  
  
  EventUtils.synthesizeMouseAtPoint(1, 1, {type: "mousemove"}, window);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});






let addTab = Task.async(function* (url) {
  info("Adding a new tab with URL: '" + url + "'");

  window.focus();

  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let browser = tab.linkedBrowser;

  yield once(browser, "load", true);
  info("URL '" + url + "' loading complete");

  return tab;
});

let navigateTo = function (toolbox, url) {
  let activeTab = toolbox.target.activeTab;
  return activeTab.navigateTo(url);
};
















function getNode(nodeOrSelector, options = {}) {
  let document = options.document || content.document;
  let noMatches = !!options.expectNoMatch;

  if (typeof nodeOrSelector === "string") {
    info("Looking for a node that matches selector " + nodeOrSelector);
    let node = document.querySelector(nodeOrSelector);
    if (noMatches) {
      ok(!node, "Selector " + nodeOrSelector + " didn't match any nodes.");
    }
    else {
      ok(node, "Selector " + nodeOrSelector + " matched a node.");
    }

    return node;
  }

  info("Looking for a node but selector was not a string.");
  return nodeOrSelector;
}










function selectAndHighlightNode(selector, inspector) {
  info("Highlighting and selecting the node " + selector);
  return selectNode(selector, inspector, "test-highlight");
}











let selectNode = Task.async(function*(selector, inspector, reason="test") {
  info("Selecting the node for '" + selector + "'");
  let nodeFront = yield getNodeFront(selector, inspector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(nodeFront, reason);
  yield updated;
});








let openInspectorForURL = Task.async(function*(url, hostType) {
  let tab = yield addTab(url);
  let { inspector, toolbox, testActor } = yield openInspector(null, hostType);
  return { tab, inspector, toolbox, testActor };
});









let openInspector = Task.async(function*(cb, hostType) {
  info("Opening the inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  let toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    if (toolbox.getPanel("inspector")) {
      info("Toolbox and inspector already open");
      throw new Error("Inspector is already opened, please use getActiveInspector");
    }
  }

  info("Opening the toolbox");
  toolbox = yield gDevTools.showToolbox(target, "inspector", hostType);
  yield waitForToolboxFrameFocus(toolbox);
  let inspector = toolbox.getPanel("inspector");

  info("Waiting for the inspector to update");
  yield inspector.once("inspector-updated");

  yield registerTestActor(toolbox.target.client);
  let testActor = yield getTestActor(toolbox);

  return {
    toolbox: toolbox,
    inspector: inspector,
    testActor: testActor
  };
});






function waitForToolboxFrameFocus(toolbox) {
  info("Making sure that the toolbox's frame is focused");
  let def = promise.defer();
  let win = toolbox.frame.contentWindow;
  waitForFocus(def.resolve, win);
  return def.promise;
}

function getActiveInspector() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  return gDevTools.getToolbox(target).getPanel("inspector");
}









function getNodeFront(selector, {walker}) {
  if (selector._form) {
    return selector;
  }
  return walker.querySelector(walker.rootNode, selector);
}













let getNodeFrontInFrame = Task.async(function*(selector, frameSelector,
                                               inspector, reason="test") {
  let iframe = yield getNodeFront(frameSelector, inspector);
  let {nodes} = yield inspector.walker.children(iframe);
  return inspector.walker.querySelector(nodes[0], selector);
});

function synthesizeKeyFromKeyTag(aKeyId, aDocument = null) {
  let document = aDocument || document;
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
  }

  EventUtils.synthesizeKey(name, modifiers);
}

let focusSearchBoxUsingShortcut = Task.async(function* (panelWin, callback) {
  info("Focusing search box");
  let searchBox = panelWin.document.getElementById("inspector-searchbox");
  let focused = once(searchBox, "focus");

  panelWin.focus();
  synthesizeKeyFromKeyTag("nodeSearchKey", panelWin.document);

  yield focused;

  if (callback) {
    callback();
  }
});









function getContainerForNodeFront(nodeFront, {markup}) {
  return markup.getContainer(nodeFront);
}









let getContainerForSelector = Task.async(function*(selector, inspector) {
  info("Getting the markup-container for node " + selector);
  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);
  info("Found markup-container " + container);
  return container;
});










let hoverContainer = Task.async(function*(selector, inspector) {
  info("Hovering over the markup-container for node " + selector);

  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);

  let highlit = inspector.toolbox.once("node-highlight");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousemove"},
    inspector.markup.doc.defaultView);
  return highlit;
});









let clickContainer = Task.async(function*(selector, inspector) {
  info("Clicking on the markup-container for node " + selector);

  let nodeFront = yield getNodeFront(selector, inspector);
  let container = getContainerForNodeFront(nodeFront, inspector);

  let updated = inspector.once("inspector-updated");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousedown"},
    inspector.markup.doc.defaultView);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mouseup"},
    inspector.markup.doc.defaultView);
  return updated;
});






function mouseLeaveMarkupView(inspector) {
  info("Leaving the markup-view area");
  let def = promise.defer();

  
  let btn = inspector.toolbox.doc.querySelector("#toolbox-controls");

  EventUtils.synthesizeMouseAtCenter(btn, {type: "mousemove"},
    inspector.toolbox.doc.defaultView);
  executeSoon(def.resolve);

  return def.promise;
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









function undoChange(inspector) {
  let canUndo = inspector.markup.undo.canUndo();
  ok(canUndo, "The last change in the markup-view can be undone");
  if (!canUndo) {
    return promise.reject();
  }

  let mutated = inspector.once("markupmutation");
  inspector.markup.undo.undo();
  return mutated;
}









function redoChange(inspector) {
  let canRedo = inspector.markup.undo.canRedo();
  ok(canRedo, "The last change in the markup-view can be redone");
  if (!canRedo) {
    return promise.reject();
  }

  let mutated = inspector.once("markupmutation");
  inspector.markup.undo.redo();
  return mutated;
}
