


const PRELOAD_PREF = "browser.newtab.preload";

gDirectorySource = "data:application/json," + JSON.stringify({
  "en-US": [{
    url: "http://example.com/organic",
    type: "organic"
  }, {
    url: "http://localhost/sponsored",
    type: "sponsored"
  }]
});

function runTests() {
  Services.prefs.setBoolPref(PRELOAD_PREF, false);
  yield addNewTabPageTab();

  let originalReportLinkAction  = DirectoryLinksProvider.reportLinkAction;
  registerCleanupFunction(() => {
    Services.prefs.clearUserPref(PRELOAD_PREF);
    DirectoryLinksProvider.reportLinkAction = originalReportLinkAction;
  });

  let expected = {};
  DirectoryLinksProvider.reportLinkAction = function(link, action, tileIndex, pinned) {
    is(link.type, expected.type, "got expected type");
    is(link.directoryIndex, expected.link, "got expected link index");
    is(action, expected.action, "got expected action");
    is(tileIndex, expected.tile, "got expected tile index");
    is(pinned, expected.pinned, "got expected pinned");
    executeSoon(TestRunner.next);
  }

  
  let siteNode = getCell(1).node.querySelector(".newtab-site");
  let pinButton = siteNode.querySelector(".newtab-control-pin");
  expected.type = "sponsored";
  expected.link = 1;
  expected.action = "pin";
  expected.tile = 1;
  expected.pinned = false;
  yield EventUtils.synthesizeMouseAtCenter(pinButton, {}, getContentWindow());

  
  expected.action = "unpin";
  expected.pinned = true;
  yield EventUtils.synthesizeMouseAtCenter(pinButton, {}, getContentWindow());
  yield whenPagesUpdated();

  
  let blockedSite = getCell(0).node.querySelector(".newtab-site");
  let blockButton = blockedSite.querySelector(".newtab-control-block");
  expected.type = "organic";
  expected.link = 0;
  expected.action = "block";
  expected.tile = 0;
  expected.pinned = false;
  yield EventUtils.synthesizeMouseAtCenter(blockButton, {}, getContentWindow());
  yield whenPagesUpdated();

  
  expected.type = "sponsored";
  expected.link = 1;
  expected.action = "click";
  yield EventUtils.synthesizeMouseAtCenter(siteNode, {}, getContentWindow());
}
