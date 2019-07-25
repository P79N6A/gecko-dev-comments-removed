






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testGroups);
  }, true);
}

function testGroups(HUD) {
  let jsterm = HUD.jsterm;
  let outputNode = HUD.outputNode;
  jsterm.clearOutput();

  
  
  
  

  let waitForSecondMessage = {
    name: "second console message",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".webconsole-msg-output").length == 2;
    },
    successFn: function()
    {
      let timestamp1 = Date.now();
      if (timestamp1 - timestamp0 < 5000) {
        is(outputNode.querySelectorAll(".webconsole-new-group").length, 0,
           "no group dividers exist after the second console message");
      }

      for (let i = 0; i < outputNode.itemCount; i++) {
        outputNode.getItemAtIndex(i).timestamp = 0;   
      }

      jsterm.execute("2");
      waitForSuccess(waitForThirdMessage);
    },
    failureFn: finishTest,
  };

  let waitForThirdMessage = {
    name: "one group divider exists after the third console message",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".webconsole-new-group").length == 1;
    },
    successFn: finishTest,
    failureFn: finishTest,
  };

  let timestamp0 = Date.now();
  jsterm.execute("0");

  waitForSuccess({
    name: "no group dividers exist after the first console message",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".webconsole-new-group").length == 0;
    },
    successFn: function()
    {
      jsterm.execute("1");
      waitForSuccess(waitForSecondMessage);
    },
    failureFn: finishTest,
  });
}
