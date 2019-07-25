



































function test() {
  
  
  let newWin = window.openDialog(getBrowserURL(), "_blank", "chrome,all,dialog=no");
  waitForExplicitFinish();
  SimpleTest.waitForFocus(function() {
    let notificationCount = 0;
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        is(aTopic, "last-pb-context-exited", "Correct topic should be dispatched");
        ++notificationCount;
      }
    };
    Services.obs.addObserver(observer, "last-pb-context-exited", false);
    newWin.gPrivateBrowsingUI.privateWindow = true;
    SimpleTest.is(notificationCount, 0, "last-pb-context-exited should not be fired yet");
    newWin.gPrivateBrowsingUI.privateWindow = false;
    newWin.close();
    newWin = null;
    window.QueryInterface(Ci.nsIInterfaceRequestor)
          .getInterface(Ci.nsIDOMWindowUtils)
          .garbageCollect(); 
    SimpleTest.is(notificationCount, 1, "last-pb-context-exited should be fired once");
    Services.obs.removeObserver(observer, "last-pb-context-exited", false);

    
    gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
    finish();
  }, newWin);
}
