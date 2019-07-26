


"use strict";

let {devtools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
let TargetFactory = devtools.TargetFactory;


let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
Services.scriptloader.loadSubScript(testDir + "/helpers.js", this);

function openInspector(callback)
{
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    callback(toolbox.getCurrentPanel());
  });
}

