



let gBugWindow;

function onLoad() {
  gBugWindow.removeEventListener("load", onLoad);
  gBugWindow.addEventListener("unload", onUnload);
  gBugWindow.close();
}

function onUnload() {
  gBugWindow.removeEventListener("unload", onUnload);
  window.focus();
  finish();
}




function test() {
  waitForExplicitFinish();
  gBugWindow = window.openDialog("chrome://pippki/content/certManager.xul");
  gBugWindow.addEventListener("load", onLoad);
}
