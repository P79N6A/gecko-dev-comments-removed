



































function test() {
  

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
    theWin.removeEventListener("load", arguments.callee, false);

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
      finish();
    });
  }, false);
}

