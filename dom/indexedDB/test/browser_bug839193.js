



let gTestRoot = getRootDirectory(gTestPath);
let gBugWindow = null;
let gIterations = 5;

function onLoad() {
  gBugWindow.close();
}

function onUnload() {
  if (!gIterations) {
    gBugWindow = null;
    Services.obs.removeObserver(onLoad, "bug839193-loaded");
    Services.obs.removeObserver(onUnload, "bug839193-unloaded");

    window.focus();
    finish();
  } else {
    gBugWindow = window.openDialog(gTestRoot + "bug839193.xul");
    gIterations--;
  }
}




function test() {
  waitForExplicitFinish();
  Services.obs.addObserver(onLoad, "bug839193-loaded", false);
  Services.obs.addObserver(onUnload, "bug839193-unloaded", false);

  gBugWindow = window.openDialog(gTestRoot + "bug839193.xul");
}
