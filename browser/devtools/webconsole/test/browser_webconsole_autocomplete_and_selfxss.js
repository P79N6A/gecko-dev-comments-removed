



const TEST_URI = "data:text/html;charset=utf-8,<p>test for bug 642615";

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");
let WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;

function consoleOpened(HUD) {
  let jsterm = HUD.jsterm;
  let stringToCopy = "foobazbarBug642615";

  jsterm.clearOutput();

  ok(!jsterm.completeNode.value, "no completeNode.value");

  jsterm.setInputValue("doc");

  let completionValue;

  
  function onCompletionValue() {
    completionValue = jsterm.completeNode.value;

    
    waitForClipboard(
      stringToCopy,
      function() {
        clipboardHelper.copyString(stringToCopy, document);
      },
      onClipboardCopy,
      finishTest);
  }

  function onClipboardCopy() {
    testSelfXss();

    jsterm.setInputValue("docu");
    info("wait for completion update after clipboard paste");
    updateEditUIVisibility();
    jsterm.once("autocomplete-updated", onClipboardPaste);
    goDoCommand("cmd_paste");
  }


  
  function testSelfXss(){
    info("Self-xss paste tests")
    WebConsoleUtils.usageCount = 0;
    is(WebConsoleUtils.usageCount, 0, "Test for usage count getter")
    
    for(let i = 0; i <= 3; i++){
      jsterm.setInputValue(i);
      jsterm.execute();
    }
    is(WebConsoleUtils.usageCount, 4, "Usage count incremented")
    WebConsoleUtils.usageCount = 0;
    updateEditUIVisibility();

    let oldVal = jsterm.inputNode.value;
    goDoCommand("cmd_paste");
    let notificationbox = jsterm.hud.document.getElementById("webconsole-notificationbox");
    let notification = notificationbox.getNotificationWithValue('selfxss-notification');
    ok(notification,  "Self-xss notification shown");
    is(oldVal, jsterm.inputNode.value, "Paste blocked by self-xss prevention");

    
    jsterm.inputNode.value = "allow pasting";
    var evt = document.createEvent("KeyboardEvent");
    evt.initKeyEvent ("keyup", true, true, window,
                      0, 0, 0, 0,
                      0, " ".charCodeAt(0));
    jsterm.inputNode.dispatchEvent(evt);
    jsterm.inputNode.value = "";
    goDoCommand("cmd_paste");
    isnot("", jsterm.inputNode.value, "Paste works");
  }
  function onClipboardPaste() {
    ok(!jsterm.completeNode.value, "no completion value after paste");

    info("wait for completion update after undo");
    jsterm.once("autocomplete-updated", onCompletionValueAfterUndo);

    
    executeSoon(() => {
      goDoCommand("cmd_undo");
    });
  }

  function onCompletionValueAfterUndo() {
    is(jsterm.completeNode.value, completionValue,
       "same completeNode.value after undo");

    info("wait for completion update after clipboard paste (ctrl-v)");
    jsterm.once("autocomplete-updated", () => {
      ok(!jsterm.completeNode.value, "no completion value after paste (ctrl-v)");

      
      executeSoon(finishTest);
    });

    
    executeSoon(() => {
      EventUtils.synthesizeKey("v", {accelKey: true});
    });
  }

  info("wait for completion value after typing 'docu'");
  jsterm.once("autocomplete-updated", onCompletionValue);

  EventUtils.synthesizeKey("u", {});
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}
