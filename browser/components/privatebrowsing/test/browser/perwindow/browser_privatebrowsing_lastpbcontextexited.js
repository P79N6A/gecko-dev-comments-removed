



function test() {
  
  
  let newWin = OpenBrowserWindow({private: true});
  waitForExplicitFinish();
  SimpleTest.waitForFocus(function() {
    let expected = false;
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        is(aTopic, "last-pb-context-exited", "Correct topic should be dispatched");
        is(expected, true, "notification not expected yet");
        Services.obs.removeObserver(observer, "last-pb-context-exited", false);
        finish();
      }
    };
    Services.obs.addObserver(observer, "last-pb-context-exited", false);
    expected = true;
    newWin.close(); 
    newWin = null;
    SpecialPowers.forceGC();
  }, newWin);
}
