




const URL = "http://example.com/browser/browser/devtools/shared/test/browser_toolbar_basic.html";

function test() {
  addTab(URL, function(browser, tab) {
    info("Starting browser_toolbar_basic.js");
    runTest();
  });
}

function runTest() {
  Services.obs.addObserver(checkOpen, DeveloperToolbar.NOTIFICATIONS.SHOW, false);
  
  
  DeveloperToolbarTest.show();
}

function checkOpen() {
  Services.obs.removeObserver(checkOpen, DeveloperToolbar.NOTIFICATIONS.SHOW, false);
  ok(DeveloperToolbar.visible, "DeveloperToolbar is visible");

  Services.obs.addObserver(checkClosed, DeveloperToolbar.NOTIFICATIONS.HIDE, false);
  
  DeveloperToolbarTest.hide();
}

function checkClosed() {
  Services.obs.removeObserver(checkClosed, DeveloperToolbar.NOTIFICATIONS.HIDE, false);
  ok(!DeveloperToolbar.visible, "DeveloperToolbar is not visible");

  finish();
}
