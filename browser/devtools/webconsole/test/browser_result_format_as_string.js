





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

  hud.jsterm.execute("document.querySelector('p')", (msg) => {
    is(hud.outputNode.textContent.indexOf("bug772506_content"), -1,
       "no content element found");
    ok(!hud.outputNode.querySelector("#foobar"), "no #foobar element found");

    ok(msg, "eval output node found");
    is(msg.textContent.indexOf("HTMLDivElement"), -1,
       "HTMLDivElement string is not displayed");
    isnot(msg.textContent.indexOf("HTMLParagraphElement"), -1,
          "HTMLParagraphElement string is displayed");

    EventUtils.synthesizeMouseAtCenter(msg, {type: "mousemove"});
    ok(!gBrowser._bug772506, "no content variable");

    finishTest();
  });
}
