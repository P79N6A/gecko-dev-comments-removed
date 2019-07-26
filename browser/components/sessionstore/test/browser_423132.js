



function test() {
  
  

  waitForExplicitFinish();

  let cs = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  cs.removeAll();

  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);

  const testURL = "http://mochi.test:8888/browser/" +
    "browser/components/sessionstore/test/browser_423132_sample.html";

  
  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", "about:blank");

  
  newWin.addEventListener("load", function (aEvent) {
    newWin.removeEventListener("load", arguments.callee, false);

    newWin.gBrowser.loadURI(testURL, null, null);

    whenBrowserLoaded(newWin.gBrowser.selectedBrowser, function() {
      
      let state = ss.getWindowState(newWin);

      
      let e = cs.enumerator;
      let cookie;
      let i = 0;
      while (e.hasMoreElements()) {
        cookie = e.getNext().QueryInterface(Ci.nsICookie);
        i++;
      }
      is(i, 1, "expected one cookie");

      
      cs.removeAll();

      
      ss.setWindowState(newWin, state, true);

      
      e = cs.enumerator;
      let cookie2;
      while (e.hasMoreElements()) {
        cookie2 = e.getNext().QueryInterface(Ci.nsICookie);
        if (cookie.name == cookie2.name)
          break;
      }
      is(cookie.name, cookie2.name, "cookie name successfully restored");
      is(cookie.value, cookie2.value, "cookie value successfully restored");
      is(cookie.path, cookie2.path, "cookie path successfully restored");

      
      if (gPrefService.prefHasUserValue("browser.sessionstore.interval"))
        gPrefService.clearUserPref("browser.sessionstore.interval");
      cs.removeAll();
      newWin.close();
      finish();
    });
  }, false);
}

