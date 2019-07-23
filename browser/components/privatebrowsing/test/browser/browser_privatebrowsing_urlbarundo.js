






































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  pb.privateBrowsingEnabled = true;

  
  gURLBar.value = "some test value";

  ok(gURLBar.editor.transactionManager.numberOfUndoItems > 0,
     "The undo history for the URL bar should not be empty");

  
  pb.privateBrowsingEnabled = false;

  is(gURLBar.editor.transactionManager.numberOfUndoItems, 0,
     "The undo history of the URL bar should be cleared after leaving the private browsing mode");

  
  prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
}

