







Cu.import("resource://gre/modules/PropertyPanel.jsm");

function test() {
  addTab("data:text/html,Web Console autocompletion bug in document.body");
  browser.addEventListener("load", onLoad, true);
}

var gHUD;

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);
  openConsole();
  let hudId = HUDService.getHudIdByWindow(content);
  gHUD = HUDService.hudReferences[hudId];
  let jsterm = gHUD.jsterm;
  let popup = jsterm.autocompletePopup;
  let completeNode = jsterm.completeNode;

  ok(!popup.isOpen, "popup is not open");

  popup._panel.addEventListener("popupshown", function() {
    popup._panel.removeEventListener("popupshown", arguments.callee, false);

    ok(popup.isOpen, "popup is open");

    let props = namesAndValuesOf(content.wrappedJSObject.document.body).length;
    is(popup.itemCount, props, "popup.itemCount is correct");

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

  popup._panel.removeEventListener("popuphidden", arguments.callee, false);

  ok(!popup.isOpen, "popup is not open");
  let inputStr = "document.b";
  jsterm.setInputValue(inputStr);
  EventUtils.synthesizeKey("o", {});
  let testStr = inputStr.replace(/./g, " ") + " ";
  is(completeNode.value, testStr + "dy", "completeNode is empty");
  jsterm.setInputValue("");

  
  let propPanel = jsterm.openPropertyPanel("Test", content.document);
  let docProps = 0;
  for (let prop in content.document) {
    docProps++;
  }
  is (propPanel.treeView.rowCount, docProps, "all document properties shown in propertyPanel");

  let treeRows = propPanel.treeView._rows;
  is (treeRows[30].display, "body: Object",  "found document.body");
  propPanel.destroy();
  executeSoon(finishTest);
}

