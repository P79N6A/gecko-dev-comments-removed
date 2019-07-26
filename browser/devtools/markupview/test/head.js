



const Cu = Components.utils;

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let TargetFactory = devtools.TargetFactory;
let {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let promise = devtools.require("sdk/core/promise");


function clearUserPrefs() {
  Services.prefs.clearUserPref("devtools.inspector.htmlPanelOpen");
  Services.prefs.clearUserPref("devtools.inspector.sidebarOpen");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
}

registerCleanupFunction(clearUserPrefs);

Services.prefs.setBoolPref("devtools.debugger.log", true);
SimpleTest.registerCleanupFunction(() => {
  Services.prefs.clearUserPref("devtools.debugger.log");
});

function getContainerForRawNode(markupView, rawNode) {
  let front = markupView.walker.frontForRawNode(rawNode);
  let container = markupView.getContainer(front);
  return container;
}





function openInspector() {
  let deferred = promise.defer();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    let inspector = toolbox.getCurrentPanel();
    inspector.once("inspector-updated", () => {
      deferred.resolve({toolbox: toolbox, inspector: inspector});
    });
  }).then(null, console.error);

  return deferred.promise;
}

function getNode(nodeOrSelector) {
  let node = nodeOrSelector;

  if (typeof nodeOrSelector === "string") {
    node = content.document.querySelector(nodeOrSelector);
    ok(node, "A node was found for selector " + nodeOrSelector);
  }

  return node;
}







function selectNode(nodeOrSelector, inspector) {
  let node = getNode(nodeOrSelector);
  let updated = inspector.once("inspector-updated");
  inspector.selection.setNode(node, "test");
  return updated;
}







function hoverContainer(nodeOrSelector, inspector) {
  let highlit = inspector.toolbox.once("node-highlight");
  let container = getContainerForRawNode(inspector.markup, getNode(nodeOrSelector));
  EventUtils.synthesizeMouse(container.tagLine, 2, 2, {type: "mousemove"},
    inspector.markup.doc.defaultView);
  return highlit;
}






function clickContainer(nodeOrSelector, inspector) {
  let updated = inspector.once("inspector-updated");
  let container = getContainerForRawNode(inspector.markup, getNode(nodeOrSelector));
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousedown"},
    inspector.markup.doc.defaultView);
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mouseup"},
    inspector.markup.doc.defaultView);
  return updated;
}




function isHighlighterVisible() {
  let outline = gBrowser.selectedBrowser.parentNode.querySelector(".highlighter-container .highlighter-outline");
  return outline && !outline.hasAttribute("hidden");
}





function mouseLeaveMarkupView(inspector) {
  let deferred = promise.defer();

  
  let btn = inspector.toolbox.doc.querySelector(".toolbox-dock-button");

  EventUtils.synthesizeMouse(btn, 2, 2, {type: "mousemove"},
    inspector.toolbox.doc.defaultView);
  executeSoon(deferred.resolve);

  return deferred.promise;
}

