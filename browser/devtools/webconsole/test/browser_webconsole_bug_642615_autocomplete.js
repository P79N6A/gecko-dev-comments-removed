



const TEST_URI = "data:text/html;charset=utf-8,<p>test for bug 642615";

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
                                   "@mozilla.org/widget/clipboardhelper;1",
                                   "nsIClipboardHelper");

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
    info("wait for completion update after clipboard paste");
    jsterm.once("autocomplete-updated", onClipboardPaste);

    updateEditUIVisibility();
    goDoCommand("cmd_paste");
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
