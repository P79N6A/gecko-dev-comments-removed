



"use strict";






















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
                                          Ci.nsIClipboard.kSelectionClipboard);

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

