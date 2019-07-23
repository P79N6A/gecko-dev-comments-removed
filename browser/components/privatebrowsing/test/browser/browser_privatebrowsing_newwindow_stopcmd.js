







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  waitForExplicitFinish();

  pb.privateBrowsingEnabled = true;

  let win = OpenBrowserWindow();
  win.addEventListener("load", function() {
    executeSoon(function() {
      executeSoon(function() {
        let cmd = win.document.getElementById("Tools:PrivateBrowsing");
        ok(!cmd.hasAttribute("disabled"),
           "The Private Browsing command in a new window should be enabled");

        win.close();
        pb.privateBrowsingEnabled = false;
        finish();
      });
    });
  }, false);
}
