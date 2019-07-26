






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

  hud.jsterm.execute("console.log('fooBug773466a')");
  hud.jsterm.execute("myObj = Object.create(null)");
  hud.jsterm.execute("console.dir(myObj)");

  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "fooBug773466a",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    },
    {
      name: "console.dir output",
      consoleDir: "[object Object]",
    }],
  }).then(() => {
    content.console.log("fooBug773466b");
    waitForMessages({
      webconsole: hud,
      messages: [{
        text: "fooBug773466b",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      }],
    }).then(finishTest);
  });
}
