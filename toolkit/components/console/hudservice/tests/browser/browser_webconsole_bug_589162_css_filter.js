










const TEST_URI = "data:text/html,<div style='font-size:3em;" +
  "foobarCssParser:baz'>test CSS parser filter</div>";

function onContentLoaded()
{
  browser.removeEventListener("load", arguments.callee, true);

  hudId = HUDService.displaysIndex()[0];
  HUD = HUDService.hudReferences[hudId].HUDBox;
  let filterBox = HUD.querySelector(".hud-filter-box");
  let outputNode = HUD.querySelector(".hud-output-node");

  let warningFound = "the unknown CSS property warning is displayed";
  let warningNotFound = "could not find the unknown CSS property warning";

  testLogEntry(outputNode, "foobarCssParser",
               { success: warningFound, err: warningNotFound }, true);

  HUDService.setFilterState(hudId, "cssparser", false);
  let nodes = HUD.querySelectorAll(".hud-msg-node");

  executeSoon(
    function (){
      HUDService.setFilterState(hudId, "cssparser", false);

      warningNotFound = "the unknown CSS property warning is not displayed, " +
        "after filtering";
      warningFound = "the unknown CSS property warning is still displayed, " +
        "after filtering";

      testLogEntry(outputNode, "foobarCssParser",
                   { success: warningNotFound, err: warningFound }, true, true);
    }
  );

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

