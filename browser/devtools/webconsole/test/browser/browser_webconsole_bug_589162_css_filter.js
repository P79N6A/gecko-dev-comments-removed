










const TEST_URI = "data:text/html,<div style='font-size:3em;" +
  "foobarCssParser:baz'>test CSS parser filter</div>";

function onContentLoaded()
{
  browser.removeEventListener("load", arguments.callee, true);

  let HUD = HUDService.getHudByWindow(content);
  let hudId = HUD.hudId;
  let outputNode = HUD.outputNode;

  let msg = "the unknown CSS property warning is displayed";
  testLogEntry(outputNode, "foobarCssParser", msg, true);

  HUDService.setFilterState(hudId, "cssparser", false);

  executeSoon(
    function (){
      let msg = "the unknown CSS property warning is not displayed, " +
                "after filtering";
      testLogEntry(outputNode, "foobarCssParser", msg, true, true);

      HUDService.setFilterState(hudId, "cssparser", true);
      finishTest();
    }
  );
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

