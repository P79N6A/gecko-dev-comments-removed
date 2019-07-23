




































let testPage = "data:text/plain,test bug 491431 Page";

function test() {
  waitForExplicitFinish();

  let newWin, tabA, tabB;

  
  tabA = gBrowser.addTab(testPage);
  gBrowser.addEventListener("TabClose", function(aEvent) {
    gBrowser.removeEventListener("TabClose", arguments.callee, true);
    ok(!aEvent.detail, "This was a normal tab close");

    
    tabB = gBrowser.addTab(testPage);
    gBrowser.addEventListener("TabClose", function(aEvent) {
      gBrowser.removeEventListener("TabClose", arguments.callee, true);
      executeSoon(function() {
        ok(aEvent.detail, "This was a tab closed by moving");

        
        newWin.close();
        executeSoon(finish);
      });
    }, true);
    newWin = gBrowser.replaceTabWithWindow(tabB);
  }, true);
  gBrowser.removeTab(tabA);
}

