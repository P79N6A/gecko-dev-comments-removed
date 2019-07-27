 



"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;

const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
const DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils");

const BASE_URI = "http://mochi.test:8888/browser/browser/devtools/fontinspector/test/"


waitForExplicitFinish();

DevToolsUtils.testing = true;
SimpleTest.registerCleanupFunction(() => {
  DevToolsUtils.testing = false;
});

registerCleanupFunction(function*() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});






function loadTab(url) {
  let deferred = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  let browser = gBrowser.getBrowserForTab(tab);

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    deferred.resolve({tab: tab, browser: browser});
  }, true);

  return deferred.promise;
}







let openInspector = Task.async(function*(cb) {
  info("Opening the inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  let inspector, toolbox;

  
  
  
  toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    inspector = toolbox.getPanel("inspector");
    if (inspector) {
      info("Toolbox and inspector already open");
      if (cb) {
        return cb(inspector, toolbox);
      } else {
        return {
          toolbox: toolbox,
          inspector: inspector
        };
      }
    }
  }

  info("Opening the toolbox");
  toolbox = yield gDevTools.showToolbox(target, "inspector");
  yield waitForToolboxFrameFocus(toolbox);
  inspector = toolbox.getPanel("inspector");

  info("Waiting for the inspector to update");
  yield inspector.once("inspector-updated");

  if (cb) {
    return cb(inspector, toolbox);
  } else {
    return {
      toolbox: toolbox,
      inspector: inspector
    };
  }
});












let openFontInspectorForURL = Task.async(function* (url) {
  info("Opening tab " + url);
  yield loadTab(url);

  let { toolbox, inspector } = yield openInspector();

  










  let onUpdated = inspector.once("fontinspector-updated");

  yield selectNode("body", inspector);
  inspector.sidebar.select("fontinspector");

  info("Waiting for font-inspector to update.");
  yield onUpdated;

  info("Font Inspector ready.");

  let { fontInspector } = inspector.sidebar.getWindowForTab("fontinspector");
  return {
    fontInspector,
    inspector,
    toolbox
  };
});




let selectNode = Task.async(function*(selector, inspector, reason="test") {
  info("Selecting the node for '" + selector + "'");
  let nodeFront = yield getNodeFront(selector, inspector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNodeFront(nodeFront, reason);
  yield updated;
});








function getNodeFront(selector, {walker}) {
  if (selector._form) {
    return selector;
  }
  return walker.querySelector(walker.rootNode, selector);
}






function waitForToolboxFrameFocus(toolbox) {
  info("Making sure that the toolbox's frame is focused");
  let def = promise.defer();
  let win = toolbox.frame.contentWindow;
  waitForFocus(def.resolve, win);
  return def.promise;
}








function* updatePreviewText(fontInspector, text) {
  info(`Changing the preview text to '${text}'`);

  let doc = fontInspector.chromeDoc;
  let input = doc.getElementById("preview-text-input");
  let update = fontInspector.inspector.once("fontinspector-updated");

  info("Focusing the input field.");
  input.focus();

  is(doc.activeElement, input, "The input was focused.");

  info("Blanking the input field.");
  for (let i = input.value.length; i >= 0; i--) {
    EventUtils.sendKey("BACK_SPACE", doc.defaultView);
  }

  is(input.value, "", "The input is now blank.");

  info("Typing the specified text to the input field.");
  EventUtils.sendString(text, doc.defaultView);
  is(input.value, text, "The input now contains the correct text.");

  info("Waiting for the font-inspector to update.");
  yield update;
}
