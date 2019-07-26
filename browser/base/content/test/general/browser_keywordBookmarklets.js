


function test() {
  waitForExplicitFinish();

  let bmFolder = Application.bookmarks.menu.addFolder("keyword-test");
  let tab = gBrowser.selectedTab = gBrowser.addTab();

  registerCleanupFunction (function () {
    bmFolder.remove();
    gBrowser.removeTab(tab);
  });

  let bm = bmFolder.addBookmark("bookmarklet", makeURI("javascript:1;"));
  bm.keyword = "bm";

  addPageShowListener(function () {
    let originalPrincipal = gBrowser.contentPrincipal;

    
    gURLBar.value = "bm";
    gURLBar.focus();
    EventUtils.synthesizeKey("VK_RETURN", {});

    addPageShowListener(function () {
      ok(gBrowser.contentPrincipal.equals(originalPrincipal), "javascript bookmarklet should inherit principal");
      finish();
    });
  });
}

function addPageShowListener(func) {
  gBrowser.selectedBrowser.addEventListener("pageshow", function loadListener() {
    gBrowser.selectedBrowser.removeEventListener("pageshow", loadListener, false);
    func();
  });
}
