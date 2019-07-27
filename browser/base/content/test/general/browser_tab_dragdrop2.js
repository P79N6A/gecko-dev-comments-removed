"use strict";

const ROOT = getRootDirectory(gTestPath);
const URI = ROOT + "browser_tab_dragdrop2_frame1.xul";





add_task(function* () {
  
  let args = "chrome,all,dialog=no";
  let win = window.openDialog(getBrowserURL(), "_blank", args, URI);

  
  yield promiseTestsDone(win);
  ok(true, "tests succeeded");

  
  win.gBrowser.addTab("about:blank", {skipAnimation: true});

  
  let browser = win.gBrowser.selectedBrowser;
  let tabClosed = promiseWaitForEvent(browser, "pagehide", true);
  let win2 = win.gBrowser.replaceTabWithWindow(win.gBrowser.tabs[0]);

  
  
  
  let onTestsDone = () => ok(false, "shouldn't run tests when tearing off");
  win2.addEventListener("TestsDone", onTestsDone);

  
  yield Promise.all([tabClosed, promiseDelayedStartupFinished(win2)]);

  
  
  win2.removeEventListener("TestsDone", onTestsDone);

  
  let promise = promiseTestsDone(win2);
  win2.content.test_panels();
  yield promise;
  ok(true, "tests succeeded a second time");

  
  yield promiseWindowClosed(win2);
  yield promiseWindowClosed(win);
});

function promiseTestsDone(win) {
  return promiseWaitForEvent(win, "TestsDone");
}

function promiseDelayedStartupFinished(win) {
  return new Promise(resolve => whenDelayedStartupFinished(win, resolve));
}
