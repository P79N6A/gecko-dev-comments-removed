





function test() {
  waitForExplicitFinish();

  function testNoSessionRestoreMenuItem() { 
    let win = OpenBrowserWindow({private: true});
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      ok(true, "The second private window got loaded");
      let srCommand = win.document.getElementById("Browser:RestoreLastSession");
      ok(srCommand, "The Session Restore command should exist");
      is(PrivateBrowsingUtils.isWindowPrivate(win), true,
         "PrivateBrowsingUtils should report the correct per-window private browsing status");
      is(srCommand.hasAttribute("disabled"), true,
         "The Session Restore command should be disabled in private browsing mode");
      win.close();
      finish();
    }, false);
  }

  let win = OpenBrowserWindow({private: true});
  win.addEventListener("load", function onload() {
    win.removeEventListener("load", onload, false);
    ok(true, "The first private window got loaded");
    win.gBrowser.addTab("about:mozilla");
    win.close();
    testNoSessionRestoreMenuItem();
  }, false);
}
