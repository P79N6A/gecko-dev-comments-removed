







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  waitForExplicitFinish();

  pb.privateBrowsingEnabled = true;

  let win = OpenBrowserWindow();
  Services.obs.addObserver(function(subject, topic, data) {
    Services.obs.removeObserver(arguments.callee, "browser-delayed-startup-finished");
    var win = subject.QueryInterface(Ci.nsIDOMWindow);

    let cmd = win.document.getElementById("Tools:PrivateBrowsing");
    ok(!cmd.hasAttribute("disabled"),
       "The Private Browsing command in a new window should be enabled");

    win.close();
    pb.privateBrowsingEnabled = false;
    finish();
  }, "browser-delayed-startup-finished", false);
}
