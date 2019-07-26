


"use strict";

const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});

let {devtools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let TargetFactory = devtools.TargetFactory;


let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);

gDevTools.testing = true;
SimpleTest.registerCleanupFunction(() => {
  gDevTools.testing = false;
});





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







let openInspectorSideBar = Task.async(function*(id) {
  let {toolbox, inspector} = yield openInspector();

  if (!hasSideBarTab(inspector, id)) {
    info("Waiting for the " + id + " sidebar to be ready");
    yield inspector.sidebar.once(id + "-ready");
  }

  info("Selecting the " + id + " sidebar");
  inspector.sidebar.select(id);

  return {
    toolbox: toolbox,
    inspector: inspector,
    view: inspector.sidebar.getWindowForTab(id)[id].view
  };
});








function hasSideBarTab(inspector, id) {
  return !!inspector.sidebar.getWindowForTab(id);
}







function openComputedView() {
  return openInspectorSideBar("computedview");
}







function openRuleView() {
  return openInspectorSideBar("ruleview");
}
