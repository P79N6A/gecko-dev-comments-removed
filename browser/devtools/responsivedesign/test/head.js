


"use strict";

let {devtools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let TargetFactory = devtools.TargetFactory;


let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "../../../commandline/test/helpers.js", this);

function openInspector(callback)
{
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    callback(toolbox.getCurrentPanel());
  });
}

function openComputedView(callback)
{
  openInspector(inspector => {
    inspector.sidebar.once("computedview-ready", () => {
      inspector.sidebar.select("computedview");
      let ruleView = inspector.sidebar.getWindowForTab("computedview").computedview.view;
      callback(inspector, ruleView);
    })
  });
}
function openRuleView(callback)
{
  openInspector(inspector => {
    inspector.sidebar.once("ruleview-ready", () => {
      inspector.sidebar.select("ruleview");
      let ruleView = inspector.sidebar.getWindowForTab("ruleview").ruleview.view;
      callback(inspector, ruleView);
    })
  });
}
