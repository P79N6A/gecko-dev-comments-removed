



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
    updateEditUIVisibility();
    goDoCommand("cmd_paste");

    waitForSuccess(waitForPaste);
  }

  let waitForPaste = {
    name: "no completion value after paste",
    validatorFn: function()
    {
      return !jsterm.completeNode.value;
    },
    successFn: onClipboardPaste,
    failureFn: finishTest,
  };

  function onClipboardPaste() {
    goDoCommand("cmd_undo");
    waitForSuccess({
      name: "completion value for 'docu' after undo",
      validatorFn: function()
      {
        return !!jsterm.completeNode.value;
      },
      successFn: onCompletionValueAfterUndo,
      failureFn: finishTest,
    });
  }

  function onCompletionValueAfterUndo() {
    is(jsterm.completeNode.value, completionValue,
       "same completeNode.value after undo");

    EventUtils.synthesizeKey("v", {accelKey: true});
    waitForSuccess({
      name: "no completion after ctrl-v (paste)",
      validatorFn: function()
      {
        return !jsterm.completeNode.value;
      },
      successFn: finishTest,
      failureFn: finishTest,
    });
  }

  EventUtils.synthesizeKey("u", {});

  waitForSuccess({
    name: "completion value for 'docu'",
    validatorFn: function()
    {
      return !!jsterm.completeNode.value;
    },
    successFn: onCompletionValue,
    failureFn: finishTest,
  });
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}
