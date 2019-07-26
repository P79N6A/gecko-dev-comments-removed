











const TEST_URI = "data:text/html;charset=utf-8,Web Console test for bug 586142";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole(null, testNewlines);
}

function testNewlines(aHud) {
  hud = aHud;
  hud.jsterm.clearOutput();

  for (let i = 0; i < 20; i++) {
    content.console.log("Hello world #" + i);
  }

  waitForSuccess({
    name: "20 console.log messages displayed",
    validatorFn: function()
    {
      return hud.outputNode.itemCount == 20;
    },
    successFn: testClipboard,
    failureFn: finishTest,
  });
}

function testClipboard() {
  let outputNode = hud.outputNode;

  outputNode.selectAll();
  outputNode.focus();

  let clipboardTexts = [];
  for (let i = 0; i < outputNode.itemCount; i++) {
    let item = outputNode.getItemAtIndex(i);
    clipboardTexts.push("[" +
                        WCU_l10n.timestampString(item.timestamp) +
                        "] " + item.clipboardText);
  }

  waitForClipboard(clipboardTexts.join("\n"),
                   function() { goDoCommand("cmd_copy"); },
                   finishTest, finishTest);
}

