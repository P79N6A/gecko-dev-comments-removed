



let gTestRoot = getRootDirectory(gTestPath);
let gBugWindow = null;
let gIterations = 5;

function onLoad() {
  gBugWindow.close();
}

function onUnload() {
  if (!gIterations) {
    finish();
  } else {
    gBugWindow = window.openDialog(gTestRoot + "bug839193.xul");
    gIterations--;
  }
}




function test() {
  waitForExplicitFinish();
  Components.classes["@mozilla.org/observer-service;1"]
            .getService(Components.interfaces.nsIObserverService)
            .addObserver(onLoad, "bug839193-loaded", false);
  Components.classes["@mozilla.org/observer-service;1"]
            .getService(Components.interfaces.nsIObserverService)
            .addObserver(onUnload, "bug839193-unloaded", false);

  gBugWindow = window.openDialog(gTestRoot + "bug839193.xul");
}
