



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
      deferred.resolve(inspector, toolbox);
    });
  }).then(null, console.error);

  return deferred.promise;
}







function selectNode(selector, inspector) {
  let deferred = promise.defer();
  let node = content.document.querySelector(selector);
  ok(node, "A node was found for selector " + selector + ". Selecting it now");
  inspector.selection.setNode(node, "test");
  inspector.once("inspector-updated", () => {
    deferred.resolve(node);
  });
  return deferred.promise;
}
