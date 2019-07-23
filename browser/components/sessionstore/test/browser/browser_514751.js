



































function browserWindowsCount() {
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  waitForExplicitFinish();

  let state = {
    windows: [{
      tabs: [{
        entries: [
          { url: "http://www.mozilla.org/projects/minefield/", title: "Minefield Start Page" },
          {}
        ]
      }]
    }]
  };

  var theWin = openDialog(location, "", "chrome,all,dialog=no");
  theWin.addEventListener("load", function () {
    executeSoon(function () {
      var gotError = false;
      try {
        ss.setWindowState(theWin, JSON.stringify(state), true);
      } catch (e) {
        if (/NS_ERROR_MALFORMED_URI/.test(e))
          gotError = true;
      }
      ok(!gotError, "Didn't get a malformed URI error.");
      theWin.close();
      is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
      finish();
    });
  }, false);
}

