



function test() {
  waitForExplicitFinish();

  let windowsToClose = [];
  registerCleanupFunction(function() {
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  let win = OpenBrowserWindow({private: true});
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    let consoleWin = win.open("chrome://global/content/console.xul", "_blank", "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar");
    consoleWin.addEventListener("load", function consoleLoad() {
      consoleWin.removeEventListener("load", consoleLoad, false);
      win.close();
    }, false);
    windowsToClose.push(consoleWin);
  }, false);

  let observer = function() {
    is(true, true, "observer fired");
    Services.obs.removeObserver(observer, "last-pb-context-exited");
    executeSoon(finish);
  };
  Services.obs.addObserver(observer, "last-pb-context-exited", false);
  windowsToClose.push(win);
}