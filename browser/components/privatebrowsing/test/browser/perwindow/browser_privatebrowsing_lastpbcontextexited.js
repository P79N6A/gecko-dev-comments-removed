



function test() {
  
  
  let newWin = window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no");
  waitForExplicitFinish();
  SimpleTest.waitForFocus(function() {
    let expected = false;
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        is(aTopic, "last-pb-context-exited", "Correct topic should be dispatched");
        is(expected, true, "notification not expected yet");
        Services.obs.removeObserver(observer, "last-pb-context-exited", false);
        gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
        finish();
      }
    };
    Services.obs.addObserver(observer, "last-pb-context-exited", false);
    setPrivateWindow(newWin, true);
    expected = true;
    newWin.close(); 
    newWin = null;
    SpecialPowers.forceGC();
  }, newWin);
}
