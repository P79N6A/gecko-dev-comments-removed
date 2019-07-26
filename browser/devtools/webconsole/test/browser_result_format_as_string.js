





const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-result-format-as-string.html";

function test()
{
  waitForExplicitFinish();

  addTab(TEST_URI);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openConsole(null, performTest);
  }, true);
}

function performTest(hud)
{
  hud.jsterm.clearOutput(true);

  hud.jsterm.execute("document.querySelector('p')");
  waitForSuccess({
    name: "eval result shown",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(".webconsole-msg-output");
    },
    successFn: function()
    {
      is(hud.outputNode.textContent.indexOf("bug772506_content"), -1,
            "no content element found");
      ok(!hud.outputNode.querySelector("div"), "no div element found");

      let msg = hud.outputNode.querySelector(".webconsole-msg-output");
      ok(msg, "eval output node found");
      is(msg.textContent.indexOf("HTMLDivElement"), -1,
         "HTMLDivElement string not displayed");
      EventUtils.synthesizeMouseAtCenter(msg, {type: "mousemove"});
      ok(!gBrowser._bug772506, "no content variable");

      finishTest();
    },
    failureFn: finishTest,
  });
}
