



"use strict";

function getLoadContext() {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsILoadContext);
}






















function waitForSelection(aExpectedStringOrValidatorFn, aSetupFn,
                          aSuccessFn, aFailureFn, aFlavor) {
    let requestedFlavor = aFlavor || "text/unicode";

    
    var inputValidatorFn = typeof(aExpectedStringOrValidatorFn) == "string"
        ? function(aData) aData == aExpectedStringOrValidatorFn
        : aExpectedStringOrValidatorFn;

    let clipboard = Cc["@mozilla.org/widget/clipboard;1"].
                    getService(Ci.nsIClipboard);

    
    function reset() {
      waitForSelection._polls = 0;
    }

    function wait(validatorFn, successFn, failureFn, flavor) {
      if (++waitForSelection._polls > 50) {
        
        ok(false, "Timed out while polling the X11 primary selection buffer.");
        reset();
        failureFn();
        return;
      }

      let transferable = Cc["@mozilla.org/widget/transferable;1"].
                         createInstance(Ci.nsITransferable);
      transferable.init(getLoadContext());
      transferable.addDataFlavor(requestedFlavor);

      clipboard.getData(transferable, clipboard.kSelectionClipboard);

      let str = {};
      let strLength = {};

      transferable.getTransferData(requestedFlavor, str, strLength);

      let data = null;
      if (str.value) {
        let strValue = str.value.QueryInterface(Ci.nsISupportsString);
        data = strValue.data.substring(0, strLength.value / 2);
      }

      if (validatorFn(data)) {
        
        if (preExpectedVal) {
          preExpectedVal = null;
        } else {
          ok(true, "The X11 primary selection buffer has the correct value");
        }
        reset();
        successFn();
      } else {
        setTimeout(function() wait(validatorFn, successFn, failureFn, flavor), 100);
      }
    }

    
    var preExpectedVal = waitForSelection._monotonicCounter +
                         "-waitForSelection-known-value";

    let clipboardHelper = Cc["@mozilla.org/widget/clipboardhelper;1"].
                          getService(Ci.nsIClipboardHelper);
    clipboardHelper.copyStringToClipboard(preExpectedVal,
                                          Ci.nsIClipboard.kSelectionClipboard,
                                          document);

    wait(function(aData) aData == preExpectedVal,
         function() {
           
           aSetupFn();
           wait(inputValidatorFn, aSuccessFn, aFailureFn, requestedFlavor);
         }, aFailureFn, "text/unicode");
}

waitForSelection._polls = 0;
waitForSelection.__monotonicCounter = 0;
waitForSelection.__defineGetter__("_monotonicCounter", function () {
  return waitForSelection.__monotonicCounter++;
});










function openSourceEditorWindow(aCallback, aOptions) {
  const windowUrl = "data:text/xml;charset=UTF-8,<?xml version='1.0'?>" +
    "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'" +
    " title='Test for Source Editor' width='600' height='500'><box flex='1'/></window>";
  const windowFeatures = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";

  let editor = null;
  let testWin = Services.ww.openWindow(null, windowUrl, "_blank",
                                       windowFeatures, null);
  testWin.addEventListener("load", function onWindowLoad() {
    testWin.removeEventListener("load", onWindowLoad, false);
    waitForFocus(initEditor, testWin);
  }, false);

  function initEditor()
  {
    let tempScope = {};
    Cu.import("resource:///modules/devtools/sourceeditor/source-editor.jsm", tempScope);

    let box = testWin.document.querySelector("box");
    editor = new tempScope.SourceEditor();
    editor.init(box, aOptions || {}, editorLoaded);
  }

  function editorLoaded()
  {
    editor.focus();
    waitForFocus(aCallback.bind(null, editor, testWin), testWin);
  }
}












function fillEditor(aEditor, aPages) {
  let view = aEditor._view;
  let model = aEditor._model;

  let lineHeight = view.getLineHeight();
  let editorHeight = view.getClientArea().height;
  let linesPerPage = Math.floor(editorHeight / lineHeight);
  let totalLines = aPages * linesPerPage;

  let text = "";
  for (let i = 0; i < totalLines; i++) {
    text += "l" + i + " lorem ipsum dolor sit amet. lipsum foobaris bazbaz,\n";
  }

  return text;
}
