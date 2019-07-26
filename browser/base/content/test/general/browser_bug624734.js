





function test() {
  waitForExplicitFinish();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  tab.linkedBrowser.addEventListener("load", (function(event) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    is(BookmarkingUI.star.getAttribute("tooltiptext"),
       BookmarkingUI._unstarredTooltip,
       "Star icon should have the unstarred tooltip text");
  
    gBrowser.removeCurrentTab();
    finish();
  }), true);

  tab.linkedBrowser.loadURI("http://example.com/browser/browser/base/content/test/general/dummy_page.html");
}
