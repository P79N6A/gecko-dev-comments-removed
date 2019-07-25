









const TEST_URI = "data:text/html,<div style='font-size:3em;" +
  "foobarCssParser:baz'>test CSS parser filter</div>"

function onContentLoaded()
{
  browser.removeEventListener("load", arguments.callee, true);

  let HUD = HUDService.getDisplayByURISpec(content.location.href);
  let hudId = HUD.getAttribute("id");
  let filterBox = HUD.querySelector(".hud-filter-box");
  let outputNode = HUD.querySelector(".hud-output-node");

  let warningFound = "the unknown CSS property warning is displayed";
  let warningNotFound = "could not find the unknown CSS property warning";

  testLogEntry(outputNode, "foobarCssParser",
    { success: warningFound, err: warningNotFound }, true);

  HUDService.setFilterState(hudId, "cssparser", false);

  warningNotFound = "the unknown CSS property warning is not displayed, " +
    "after filtering";
  warningFound = "the unknown CSS property warning is still displayed, " +
    "after filtering";

  testLogEntry(outputNode, "foobarCssParser",
    { success: warningNotFound, err: warningFound }, true, true);

  finishTest();
}





function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    openConsole();
    browser.addEventListener("load", onContentLoaded, true);
    content.location.reload();
  }, true);
}

