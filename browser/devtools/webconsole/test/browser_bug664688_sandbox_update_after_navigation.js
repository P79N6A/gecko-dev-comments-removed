








function test()
{
  const TEST_URI1 = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";
  const TEST_URI2 = "http://example.org/browser/browser/devtools/webconsole/test/test-console.html";

  let hud;

  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab(TEST_URI1);
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openConsole(gBrowser.selectedTab, pageLoad1);
  }, true);


  function pageLoad1(aHud)
  {
    hud = aHud;

    hud.jsterm.clearOutput();
    hud.jsterm.execute("window.location.href");

    waitForSuccess(waitForLocation1);
  }

  let waitForLocation1 = {
    name: "window.location.href result is displayed",
    validatorFn: function()
    {
      let node = hud.outputNode.getElementsByClassName("webconsole-msg-output")[0];
      return node && node.textContent.indexOf(TEST_URI1) > -1;
    },
    successFn: function()
    {
      let node = hud.outputNode.getElementsByClassName("webconsole-msg-input")[0];
      isnot(node.textContent.indexOf("window.location.href"), -1,
            "jsterm input is also displayed");

      is(hud.outputNode.textContent.indexOf("Permission denied"), -1,
         "no permission denied errors");

      gBrowser.selectedBrowser.addEventListener("load", onPageLoad2, true);
      content.location = TEST_URI2;
    },
    failureFn: finishTestWithError,
  };

  function onPageLoad2() {
    gBrowser.selectedBrowser.removeEventListener("load", onPageLoad2, true);

    hud.jsterm.clearOutput();
    hud.jsterm.execute("window.location.href");

    waitForSuccess(waitForLocation2);
  }

  let waitForLocation2 = {
    name: "window.location.href result is displayed after page navigation",
    validatorFn: function()
    {
      let node = hud.outputNode.getElementsByClassName("webconsole-msg-output")[0];
      return node && node.textContent.indexOf(TEST_URI2) > -1;
    },
    successFn: function()
    {
      let node = hud.outputNode.getElementsByClassName("webconsole-msg-input")[0];
      isnot(node.textContent.indexOf("window.location.href"), -1,
            "jsterm input is also displayed");
      is(hud.outputNode.textContent.indexOf("Permission denied"), -1,
         "no permission denied errors");

      gBrowser.goBack();
      waitForSuccess(waitForBack);
    },
    failureFn: finishTestWithError,
  };

  let waitForBack = {
    name: "go back",
    validatorFn: function()
    {
      return content.location.href == TEST_URI1;
    },
    successFn: function()
    {
      hud.jsterm.clearOutput();
      hud.jsterm.execute("window.location.href");

      waitForSuccess(waitForLocation3);
    },
    failureFn: finishTestWithError,
  };

  let waitForLocation3 = {
    name: "window.location.href result is displayed after goBack()",
    validatorFn: function()
    {
      let node = hud.outputNode.getElementsByClassName("webconsole-msg-output")[0];
      return node && node.textContent.indexOf(TEST_URI1) > -1;
    },
    successFn: function()
    {
      let node = hud.outputNode.getElementsByClassName("webconsole-msg-input")[0];
      isnot(node.textContent.indexOf("window.location.href"), -1,
            "jsterm input is also displayed");
      is(hud.outputNode.textContent.indexOf("Permission denied"), -1,
         "no permission denied errors");

      executeSoon(finishTest);
    },
    failureFn: finishTestWithError,
  };

  function finishTestWithError()
  {
    info("output content: " + hud.outputNode.textContent);
    finishTest();
  }
}
