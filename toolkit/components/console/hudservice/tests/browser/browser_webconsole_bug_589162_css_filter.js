




































const Cc = Components.classes;

Cu.import("resource://gre/modules/HUDService.jsm");

const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";












function testLogEntry(aOutputNode, aMatchString, aSuccessErrObj, aOnlyVisible, aFailIfFound)
{
  let found = true;
  let notfound = false;
  let foundMsg = aSuccessErrObj.success;
  let notfoundMsg = aSuccessErrObj.err;

  if (aFailIfFound) {
    found = false;
    notfound = true;
    foundMsg = aSuccessErrObj.err;
    notfoundMsg = aSuccessErrObj.success;
  }

  let selector = ".hud-group > *";

  
  if (aOnlyVisible) {
    selector += ":not(.hud-filtered-by-type)";
  }

  let msgs = aOutputNode.querySelectorAll(selector);
  for (let i = 1, n = msgs.length; i < n; i++) {
    let message = msgs[i].textContent.indexOf(aMatchString);
    if (message > -1) {
      ok(found, foundMsg);
      return;
    }
  }

  ok(notfound, notfoundMsg);
}

function onContentLoaded() {
  gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

  let HUD = HUDService.getDisplayByURISpec(content.location.href);
  let hudId = HUD.getAttribute("id");
  let filterBox = HUD.querySelector(".hud-filter-box");
  let outputNode = HUD.querySelector(".hud-output-node");

  let warningFound = "the unknown CSS property warning is displayed";
  let warningNotFound = "could not find the unknown CSS property warning";

  testLogEntry(outputNode, "foobarCssParser",
    { success: warningFound, err: warningNotFound });

  HUDService.setFilterState(hudId, "cssparser", false);

  warningNotFound = "the unknown CSS property warning is not displayed, " +
    "after filtering";
  warningFound = "the unknown CSS property warning is still displayed, " +
    "after filtering";

  testLogEntry(outputNode, "foobarCssParser",
    { success: warningNotFound, err: warningFound }, true, true);

  HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  finish();
}





function test() {
  waitForExplicitFinish();

  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    waitForFocus(function () {
      HUDService.activateHUDForContext(gBrowser.selectedTab);

      gBrowser.selectedBrowser.addEventListener("load", onContentLoaded, true);

      content.location = "data:text/html,<div style='font-size:3em;" +
        "foobarCssParser:baz'>test CSS parser filter</div>";
    });
  }, true);

  content.location = TEST_URI;
}
