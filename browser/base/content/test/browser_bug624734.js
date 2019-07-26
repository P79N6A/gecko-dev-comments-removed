





function finishTest() {
  is(BookmarkingUI.button.getAttribute("buttontooltiptext"),
     BookmarkingUI._unstarredTooltip,
     "Star icon should have the unstarred tooltip text");

  gBrowser.removeCurrentTab();
  finish();
}

function test() {
  waitForExplicitFinish();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  tab.linkedBrowser.addEventListener("load", (function(event) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    if (BookmarkingUI._pendingStmt) {
      waitForCondition(function() !BookmarkingUI._pendingStmt, finishTest, "BookmarkingUI held pending statement for too long");
    } else {
      finishTest();
    }
  }), true);

  tab.linkedBrowser.loadURI("http://example.com/browser/browser/base/content/test/dummy_page.html");
}
