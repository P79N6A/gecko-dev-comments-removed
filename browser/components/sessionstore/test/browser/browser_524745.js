




































function test() {
  

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let uniqKey = "bug524745";
  let uniqVal = Date.now();

  waitForExplicitFinish();

  let window_B = openDialog(location, "_blank", "chrome,all,dialog=no");
  window_B.addEventListener("load", function(aEvent) {
    window_B.removeEventListener("load", arguments.callee, false);

      waitForFocus(function() {
        
        ss.setWindowValue(window_B, uniqKey, uniqVal);
        let state = JSON.parse(ss.getBrowserState());
        let selectedWindow = state.windows[state.selectedWindow - 1];
        is(selectedWindow.extData && selectedWindow.extData[uniqKey], uniqVal,
           "selectedWindow is window_B");

        
        window_B.minimize();
        waitForFocus(function() {
          state = JSON.parse(ss.getBrowserState());
          selectedWindow = state.windows[state.selectedWindow - 1];
          ok(!selectedWindow.extData || !selectedWindow.extData[uniqKey],
             "selectedWindow is not window_B after minimizing it");

          
          window.minimize();
          state = JSON.parse(ss.getBrowserState());
          is(state.selectedWindow, 0,
             "selectedWindow should be 0 when all windows are minimized");

          
          window.restore();
          window_B.close();
          finish();
        });
      }, window_B);
  }, false);
}
