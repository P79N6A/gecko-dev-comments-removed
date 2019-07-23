







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  const kTestSearchString = "privatebrowsing";
  let findBox = gFindBar.getElement("findbar-textbox");
  gFindBar.startFind(gFindBar.FIND_NORMAL);

  
  is(findBox.editor.transactionManager.numberOfUndoItems, 0,
    "No items in the undo list of the findbar control");
  is(findBox.value, "",
    "findbar text is empty");

  findBox.value = kTestSearchString;

  
  pb.privateBrowsingEnabled = true;

  is(findBox.value, kTestSearchString,
    "entering the private browsing mode should not clear the findbar");
  ok(findBox.editor.transactionManager.numberOfUndoItems > 0,
    "entering the private browsing mode should not reset the undo list of the findbar control");

  
  findBox.value = "something else";

  
  pb.privateBrowsingEnabled = false;

  is(findBox.value, kTestSearchString,
    "leaving the private browsing mode should restore the findbar contents");
  is(findBox.editor.transactionManager.numberOfUndoItems, 1,
    "leaving the private browsing mode should only leave 1 item in the undo list of the findbar control");

  
  prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
  gFindBar.close();
}
