










const TEST_URI = "data:text/html,<div style='font-size:3em;" +
  "foobarCssParser:baz'>test CSS parser filter</div>";

function onContentLoaded()
{
  browser.removeEventListener("load", arguments.callee, true);

  hudId = HUDService.displaysIndex()[0];
  HUD = HUDService.hudReferences[hudId].HUDBox;
  let filterBox = HUD.querySelector(".hud-filter-box");
  let outputNode = HUD.querySelector(".hud-output-node");

  let msg = "the unknown CSS property warning is displayed";
  testLogEntry(outputNode, "foobarCssParser", msg, true);

  HUDService.setFilterState(hudId, "cssparser", false);
  let nodes = HUD.querySelectorAll(".hud-msg-node");

  executeSoon(
    function (){
      HUDService.setFilterState(hudId, "cssparser", false);

      let msg = "the unknown CSS property warning is not displayed, " +
                "after filtering";
      testLogEntry(outputNode, "foobarCssParser", msg, true, true);
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

