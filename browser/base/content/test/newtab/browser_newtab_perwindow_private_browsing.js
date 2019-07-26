









function runTests() {
  
  yield testOnWindow(undefined);
  yield setLinks("0,1,2,3,4,5,6,7,8,9");

  yield addNewTabPageTab();
  pinCell(0);
  checkGrid("0p,1,2,3,4,5,6,7,8");

  
  yield testOnWindow({private: true});

  yield addNewTabPageTab();
  checkGrid("0p,1,2,3,4,5,6,7,8");

  
  yield blockCell(1);
  checkGrid("0p,2,3,4,5,6,7,8");

  yield unpinCell(0);
  checkGrid("0,2,3,4,5,6,7,8");

  
  yield testOnWindow(undefined);

  
  yield addNewTabPageTab();
  checkGrid("0,2,3,4,5,6,7,8")
}

var windowsToClose = [];
function testOnWindow(options) {
  var win = OpenBrowserWindow(options);
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    windowsToClose.push(win);
    gWindow = win;
    executeSoon(TestRunner.next);
  }, false);
}

registerCleanupFunction(function () {
  gWindow = window;
  windowsToClose.forEach(function(win) {
    win.close();
  });
});

