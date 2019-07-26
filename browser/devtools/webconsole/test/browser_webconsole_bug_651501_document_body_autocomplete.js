







function test() {
  addTab("data:text/html;charset=utf-8,Web Console autocompletion bug in document.body");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

let gHUD;

function consoleOpened(aHud) {
  gHUD = aHud;
  let jsterm = gHUD.jsterm;
  let popup = jsterm.autocompletePopup;
  let completeNode = jsterm.completeNode;

  ok(!popup.isOpen, "popup is not open");

  popup._panel.addEventListener("popupshown", function onShown() {
    popup._panel.removeEventListener("popupshown", onShown, false);

    ok(popup.isOpen, "popup is open");

    
    
    
    
    ok(popup.itemCount >= 14, "popup.itemCount is correct");

    popup._panel.addEventListener("popuphidden", autocompletePopupHidden, false);

    EventUtils.synthesizeKey("VK_ESCAPE", {});
  }, false);

  jsterm.setInputValue("document.body");
  EventUtils.synthesizeKey(".", {});
}

function autocompletePopupHidden()
{
  let jsterm = gHUD.jsterm;
  let popup = jsterm.autocompletePopup;
  let completeNode = jsterm.completeNode;
  let inputNode = jsterm.inputNode;

  popup._panel.removeEventListener("popuphidden", autocompletePopupHidden, false);

  ok(!popup.isOpen, "popup is not open");
  let inputStr = "document.b";
  jsterm.setInputValue(inputStr);
  EventUtils.synthesizeKey("o", {});
  let testStr = inputStr.replace(/./g, " ") + " ";

  waitForSuccess({
    name: "autocomplete shows document.body",
    validatorFn: function()
    {
      return completeNode.value == testStr + "dy";
    },
    successFn: testPropertyPanel,
    failureFn: finishTest,
  });
}

function testPropertyPanel()
{
  let jsterm = gHUD.jsterm;
  jsterm.clearOutput();
  jsterm.execute("document", (msg) => {
    jsterm.once("variablesview-fetched", onVariablesViewReady);
    let anchor = msg.querySelector(".body a");
    EventUtils.synthesizeMouse(anchor, 2, 2, {}, gHUD.iframeWindow);
  });
}

function onVariablesViewReady(aEvent, aView)
{
  findVariableViewProperties(aView, [
    { name: "body", value: "HTMLBodyElement" },
  ], { webconsole: gHUD }).then(finishTest);
}

