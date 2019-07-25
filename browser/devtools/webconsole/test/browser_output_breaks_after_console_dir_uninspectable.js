






function test()
{
  waitForExplicitFinish();

  addTab("data:text/html;charset=utf8,test for bug 773466");

  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openConsole(null, performTest);
  }, true);
}

function performTest(hud)
{
  hud.jsterm.clearOutput(true);

  content.console.log("fooBug773466a");
  content.console.dir(function funBug773466(){});
  waitForSuccess({
    name: "eval results are shown",
    validatorFn: function()
    {
      return hud.outputNode.textContent.indexOf("funBug773466") > -1;
    },
    successFn: function()
    {
      isnot(hud.outputNode.textContent.indexOf("fooBug773466a"), -1,
            "fooBug773466a shows");
      ok(hud.outputNode.querySelector(".webconsole-msg-inspector"),
         "the console.dir() tree shows");

      content.console.log("fooBug773466b");

      waitForSuccess(waitForAnotherConsoleLogCall);
    },
    failureFn: finishTest,
  });

  let waitForAnotherConsoleLogCall = {
    name: "eval result after console.dir()",
    validatorFn: function()
    {
      return hud.outputNode.textContent.indexOf("fooBug773466b") > -1;
    },
    successFn: finishTest,
    failureFn: finishTest,
  };
}
