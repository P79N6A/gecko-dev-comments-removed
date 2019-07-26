



"use strict";

const Cu = Components.utils;
let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
let {console} = Components.utils.import("resource://gre/modules/devtools/Console.jsm", {});


waitForExplicitFinish();

const TEST_URL_ROOT = "http://example.com/browser/browser/devtools/layoutview/test/";






gDevTools.testing = true;
registerCleanupFunction(() => gDevTools.testing = false);



Services.prefs.setIntPref("devtools.toolbox.footer.height", 350);
registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.debugger.log");
  Services.prefs.clearUserPref("devtools.toolbox.footer.height");
  Services.prefs.setCharPref("devtools.inspector.activeSidebar", "ruleview");
});


registerCleanupFunction(() => {
  
  
  
  
  
  
  
  
  
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});




function asyncTest(generator) {
  return () => Task.spawn(generator).then(null, ok.bind(null, false)).then(finish);
}






function addTab(url) {
  let def = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL " + url + " loading complete into new test tab");
    waitForFocus(() => {
      def.resolve(tab);
    }, content);
  }, true);
  content.location = url;

  return def.promise;
}






let destroyToolbox = Task.async(function*(inspector) {
  let onDestroyed = gDevTools.once("toolbox-destroyed");
  inspector._toolbox.destroy();
  yield onDestroyed;
});







function getNode(nodeOrSelector) {
  return typeof nodeOrSelector === "string" ?
    content.document.querySelector(nodeOrSelector) :
    nodeOrSelector;
}













function selectNode(nodeOrSelector, inspector, reason="test") {
  info("Selecting the node " + nodeOrSelector);
  let node = getNode(nodeOrSelector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNode(node, reason);
  return updated;
}





let openInspector = Task.async(function*() {
  info("Opening the inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  let inspector, toolbox;

  
  
  
  toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    inspector = toolbox.getPanel("inspector");
    if (inspector) {
      info("Toolbox and inspector already open");
      return {
        toolbox: toolbox,
        inspector: inspector
      };
    }
  }

  info("Opening the toolbox");
  toolbox = yield gDevTools.showToolbox(target, "inspector");
  yield waitForToolboxFrameFocus(toolbox);
  inspector = toolbox.getPanel("inspector");

  info("Waiting for the inspector to update");
  yield inspector.once("inspector-updated");

  return {
    toolbox: toolbox,
    inspector: inspector
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







let openLayoutView = Task.async(function*() {
  let {toolbox, inspector} = yield openInspector();

  if (!hasSideBarTab(inspector, "layoutview")) {
    info("Waiting for the layoutview sidebar to be ready");
    yield inspector.sidebar.once("layoutview-ready");
  }

  info("Selecting the layoutview sidebar");
  inspector.sidebar.select("layoutview");

  return {
    toolbox: toolbox,
    inspector: inspector,
    view: inspector.sidebar.getWindowForTab("layoutview")["layoutview"]
  };
});





function waitForUpdate(inspector) {
  return inspector.once("layoutview-updated");
}

function getHighlighter() {
  return gBrowser.selectedBrowser.parentNode.querySelector(".highlighter-container");
}

function getBoxModelRoot() {
  let highlighter = getHighlighter();
  return highlighter.querySelector(".box-model-root");
}

function getGuideStatus(location) {
  let root = getBoxModelRoot();
  let guide = root.querySelector(".box-model-guide-" + location);

  return {
    visible: !guide.hasAttribute("hidden"),
    x1: guide.getAttribute("x1"),
    y1: guide.getAttribute("y1"),
    x2: guide.getAttribute("x2"),
    y2: guide.getAttribute("y2")
  };
}













var TESTS = [];

function addTest(message, func) {
  TESTS.push([message, Task.async(func)])
}

let runTests = Task.async(function*(...args) {
  for (let [message, test] of TESTS) {
    info("Running new test case: " + message);
    yield test.apply(null, args);
  }
});
