







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let pbCmd = document.getElementById("Tools:PrivateBrowsing");
  waitForExplicitFinish();

  let observer = {
    pass: 1,
    observe: function(aSubject, aTopic, aData) {
      switch (aTopic) {
        case "private-browsing":
          setTimeout(function() {
            ok(document.getElementById("Tools:PrivateBrowsing").hasAttribute("disabled"),
               "The private browsing command should be disabled immediately after the mode switch");
          }, 0);
          break;

        case "private-browsing-transition-complete":
          if (this.pass++ == 1) {
            setTimeout(function() {
              ok(!pbCmd.hasAttribute("disabled"),
                 "The private browsing command should be re-enabled after entering the private browsing mode");

              pb.privateBrowsingEnabled = false;
            }, 100);
          }
          else {
            setTimeout(function() {
              ok(!pbCmd.hasAttribute("disabled"),
                 "The private browsing command should be re-enabled after exiting the private browsing mode");

              os.removeObserver(observer, "private-browsing");
              os.removeObserver(observer, "private-browsing-transition-complete");
              finish();
            }, 100);
          }
          break;
      }
    }
  };
  os.addObserver(observer, "private-browsing", false);
  os.addObserver(observer, "private-browsing-transition-complete", false);

  pb.privateBrowsingEnabled = true;
}
