









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-network.html";

function consoleOpened(aHud) {
  hud = aHud;

  for (let i = 0; i < 200; i++) {
    content.console.log("test message " + i);
  }

  hud.setFilterState("network", false);
  hud.setFilterState("networkinfo", false);

  hud.ui.filterBox.value = "test message";
  hud.ui.adjustVisibilityOnSearchStringChange();

  let waitForNetwork = {
    name: "network message",
    validatorFn: function()
    {
      return hud.outputNode.querySelector(".webconsole-msg-network");
    },
    successFn: testScroll,
    failureFn: finishTest,
  };

  waitForSuccess({
    name: "console messages displayed",
    validatorFn: function()
    {
      return hud.outputNode.textContent.indexOf("test message 199") > -1;
    },
    successFn: function()
    {
      browser.addEventListener("load", function onReload() {
        browser.removeEventListener("load", onReload, true);
        waitForSuccess(waitForNetwork);
      }, true);
      content.location.reload();
    },
    failureFn: finishTest,
  });
}

function testScroll() {
  let msgNode = hud.outputNode.querySelector(".webconsole-msg-network");
  ok(msgNode.classList.contains("hud-filtered-by-type"),
    "network message is filtered by type");
  ok(msgNode.classList.contains("hud-filtered-by-string"),
    "network message is filtered by string");

  let scrollBox = hud.outputNode.scrollBoxObject.element;
  ok(scrollBox.scrollTop > 0, "scroll location is not at the top");

  
  
  let nodeHeight = hud.outputNode.querySelector(".hud-log").clientHeight;
  ok(scrollBox.scrollTop >= scrollBox.scrollHeight - scrollBox.clientHeight -
     nodeHeight * 2, "scroll location is correct");

  hud.setFilterState("network", true);
  hud.setFilterState("networkinfo", true);

  executeSoon(finishTest);
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

