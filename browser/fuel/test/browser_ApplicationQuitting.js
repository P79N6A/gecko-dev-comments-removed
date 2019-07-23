function test() {
  function quitRequestObserver(aSubject, aTopic, aData) {
    ok(aTopic == "quit-application-requested" &&
       aSubject instanceof Components.interfaces.nsISupportsPRBool,
       "Received a quit request we're going to deny");
    aSubject.data = true;
  }

  
  Services.obs.addObserver(quitRequestObserver, "quit-application-requested", false);

  ok(!Application.quit(),    "Tried to quit - and didn't succeed");
  ok(!Application.restart(), "Tried to restart - and didn't succeed");

  
  Services.obs.removeObserver(quitRequestObserver, "quit-application-requested", false);
}
