



































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
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();

  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no,toolbar=yes");
  newWin.addEventListener("load", function() {
    newWin.removeEventListener("load", arguments.callee, false);

    executeSoon(function() {
      let state1 = ss.getWindowState(newWin);
      newWin.close();

      newWin = openDialog(location, "_blank",
        "chrome,extrachrome,menubar,resizable,scrollbars,status,toolbar=no,location,personal,directories,dialog=no");
      newWin.addEventListener("load", function() {
        newWin.removeEventListener("load", arguments.callee, false);

        executeSoon(function() {
          let state2 = ss.getWindowState(newWin);
          newWin.close();

          function testState(state, expected, callback) {
            let win = openDialog(location, "_blank", "chrome,all,dialog=no");
            win.addEventListener("load", function() {
              win.removeEventListener("load", arguments.callee, false);

              is(win.gURLBar.readOnly, false,
                 "URL bar should not be read-only before setting the state");
              is(win.gURLBar.getAttribute("enablehistory"), "true",
                 "URL bar autocomplete should be enabled before setting the state");
              ss.setWindowState(win, state, true);
              is(win.gURLBar.readOnly, expected.readOnly,
                 "URL bar read-only state should be restored correctly");
              is(win.gURLBar.getAttribute("enablehistory"), expected.enablehistory,
                 "URL bar autocomplete state should be restored correctly");

              win.close();
              executeSoon(callback);
            }, false);
          }

          testState(state1, {readOnly: false, enablehistory: "true"}, function() {
            testState(state2, {readOnly: true, enablehistory: "false"}, function() {
              executeSoon(function () {
                is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
                finish();
              });
            });
          });
        });
      }, false);
    });
  }, false);
}
