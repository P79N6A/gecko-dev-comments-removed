







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  let crhCommand = document.getElementById("Tools:Sanitize");

  
  ok(!crhCommand.hasAttribute("disabled"),
    "Clear Recent History command should not be disabled outside of the private browsing mode");

  
  pb.privateBrowsingEnabled = true;

  ok(crhCommand.hasAttribute("disabled"),
    "Clear Recent History command should be disabled inside of the private browsing mode");

  
  pb.privateBrowsingEnabled = false;

  ok(!crhCommand.hasAttribute("disabled"),
    "Clear Recent History command should not be disabled after leaving the private browsing mode");
}
