










const TEST_URI = "data:text/html;charset=utf-8,<div style='font-size:3em;" +
  "foobarCssParser:baz'>test CSS parser filter</div>";

function onContentLoaded()
{
  browser.removeEventListener("load", onContentLoaded, true);

  let HUD = HUDService.getHudByWindow(content);
  let hudId = HUD.hudId;
  let outputNode = HUD.outputNode;

  HUD.jsterm.clearOutput();

  waitForSuccess({
    name: "css error displayed",
    validatorFn: function()
    {
      return outputNode.textContent.indexOf("foobarCssParser") > -1;
    },
    successFn: function()
    {
      HUDService.setFilterState(hudId, "cssparser", false);

      let msg = "the unknown CSS property warning is not displayed, " +
                "after filtering";
      testLogEntry(outputNode, "foobarCssParser", msg, true, true);

      HUDService.setFilterState(hudId, "cssparser", true);
      finishTest();
    },
    failureFn: finishTest,
  });
}





function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);

    openConsole();
    browser.addEventListener("load", onContentLoaded, true);
    content.location.reload();
  }, true);
}

