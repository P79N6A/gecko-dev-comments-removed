



"use strict";

const TEST_URL = "http://example.com/browser/toolkit/components/startup/tests/browser/beforeunload.html";

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
  let browser = gBrowser.selectedBrowser;

  whenBrowserLoaded(browser, function () {
    let seenDialog = false;

    
    waitForOnBeforeUnloadDialog(browser, (btnLeave, btnStay) => {
      seenDialog = true;
      btnStay.click();
    });

    let appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                       getService(Ci.nsIAppStartup);
    appStartup.quit(Ci.nsIAppStartup.eAttemptQuit);
    ok(seenDialog, "Should have seen a prompt dialog");
    ok(!window.closed, "Shouldn't have closed the window");

    let win2 = window.openDialog(location, "", "chrome,all,dialog=no", "about:blank");
    ok(win2 != null, "Should have been able to open a new window");
    win2.addEventListener("load", function onLoad() {
      win2.removeEventListener("load", onLoad);
      win2.close();

      
      waitForOnBeforeUnloadDialog(browser, (btnLeave, btnStay) => {
        btnLeave.click();
      });

      gBrowser.removeTab(gBrowser.selectedTab);
      executeSoon(finish);
    });
  });
}
