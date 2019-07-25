



const TEST_URI = "data:text/html,<p>test for bug 642615";

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();

  let hudId = HUDService.getHudIdByWindow(content);
  let HUD = HUDService.hudReferences[hudId];
  let jsterm = HUD.jsterm;
  let stringToCopy = "foobazbarBug642615";

  jsterm.clearOutput();

  ok(!jsterm.completeNode.value, "no completeNode.value");

  jsterm.setInputValue("doc");

  
  jsterm.inputNode.addEventListener("keyup", function() {
    jsterm.inputNode.removeEventListener("keyup", arguments.callee, false);

    let completionValue = jsterm.completeNode.value;
    ok(completionValue, "we have a completeNode.value");

    
    jsterm.inputNode.addEventListener("input", function() {
      jsterm.inputNode.removeEventListener("input", arguments.callee, false);

      ok(!jsterm.completeNode.value, "no completeNode.value after clipboard paste");

      
      jsterm.inputNode.addEventListener("input", function() {
        jsterm.inputNode.removeEventListener("input", arguments.callee, false);

        is(jsterm.completeNode.value, completionValue,
           "same completeNode.value after undo");

        
        jsterm.inputNode.addEventListener("keyup", function() {
          jsterm.inputNode.removeEventListener("keyup", arguments.callee, false);

          ok(!jsterm.completeNode.value,
             "no completeNode.value after clipboard paste (via keyboard event)");

          executeSoon(finishTest);
        }, false);

        EventUtils.synthesizeKey("v", {accelKey: true});
      }, false);

      goDoCommand("cmd_undo");
    }, false);

    
    waitForClipboard(
      stringToCopy,
      function() {
        clipboardHelper.copyString(stringToCopy);
      },
      function() {
        updateEditUIVisibility();
        goDoCommand("cmd_paste");
      },
      finish);
  }, false);

  EventUtils.synthesizeKey("u", {});
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoad, true);
}
