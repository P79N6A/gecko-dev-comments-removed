







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  
  let stateBackup = ss.getWindowState(window);

  function pretendToBeAPopup(whatToPretend) {
    let state = whatToPretend ?
      '{"windows":[{"tabs":[{"entries":[],"attributes":{}}],"isPopup":true,"hidden":"toolbar"}]}' :
      '{"windows":[{"tabs":[{"entries":[],"attributes":{}}],"isPopup":false}]}';
    ss.setWindowState(window, state, true);
    if (whatToPretend) {
      ok(gURLBar.readOnly, "pretendToBeAPopup correctly made the URL bar read-only");
      is(gURLBar.getAttribute("enablehistory"), "false",
         "pretendToBeAPopup correctly disabled autocomplete on the URL bar");
    }
    else {
      ok(!gURLBar.readOnly, "pretendToBeAPopup correctly made the URL bar read-write");
      is(gURLBar.getAttribute("enablehistory"), "true",
         "pretendToBeAPopup correctly enabled autocomplete on the URL bar");
    }
  }

  

  
  pretendToBeAPopup(true);

  
  pb.privateBrowsingEnabled = true;

  
  ok(!gURLBar.readOnly,
     "URL bar should not be read-only after entering the private browsing mode");
  is(gURLBar.getAttribute("enablehistory"), "true",
     "URL bar autocomplete should be enabled after entering the private browsing mode");

  
  pb.privateBrowsingEnabled = false;

  
  pretendToBeAPopup(false);

  

  
  pb.privateBrowsingEnabled = true;

  
  pretendToBeAPopup(true);

  
  pb.privateBrowsingEnabled = false;

  
  ok(!gURLBar.readOnly,
     "URL bar should not be read-only after leaving the private browsing mode");
  is(gURLBar.getAttribute("enablehistory"), "true",
     "URL bar autocomplete should be enabled after leaving the private browsing mode");

  
  ss.setWindowState(window, stateBackup, true);
}
