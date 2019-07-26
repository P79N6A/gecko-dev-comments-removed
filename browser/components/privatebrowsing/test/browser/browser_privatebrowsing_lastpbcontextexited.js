



function test() {
  
  
  function runTest(aCloseWindow, aCallback) {
    let newWin = OpenBrowserWindow({private: true});
    SimpleTest.waitForFocus(function() {
      let expectedExiting = true;
      let expectedExited = false;
      let observerExiting = {
        observe: function(aSubject, aTopic, aData) {
          is(aTopic, "last-pb-context-exiting", "Correct topic should be dispatched (exiting)");
          is(expectedExiting, true, "notification not expected yet (exiting)");
          expectedExited = true;
          Services.obs.removeObserver(observerExiting, "last-pb-context-exiting", false);
        }
      };
      let observerExited = {
        observe: function(aSubject, aTopic, aData) {
          is(aTopic, "last-pb-context-exited", "Correct topic should be dispatched (exited)");
          is(expectedExited, true, "notification not expected yet (exited)");
          Services.obs.removeObserver(observerExited, "last-pb-context-exited", false);
          aCallback();
        }
      };
      Services.obs.addObserver(observerExiting, "last-pb-context-exiting", false);
      Services.obs.addObserver(observerExited, "last-pb-context-exited", false);
      expectedExiting = true;
      aCloseWindow(newWin);
      newWin = null;
      SpecialPowers.forceGC();
    }, newWin);
  }

  waitForExplicitFinish();

  runTest(function(newWin) {
      
      newWin.document.getElementById("cmd_closeWindow").doCommand();
    }, function () {
      runTest(function(newWin) {
          
          newWin.document.getElementById("cmd_close").doCommand();
        }, finish);
    });
}
