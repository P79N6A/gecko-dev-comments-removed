



"use strict";
const isOSX = (Services.appinfo.OS === "Darwin");

add_task(function() {
  info("Check print button existence and functionality");
  yield PanelUI.show();

  let printButton = document.getElementById("print-button");
  ok(printButton, "Print button exists in Panel Menu");

  if(isOSX) {
    yield PanelUI.hide();
  }
  else {
    printButton.click();
    yield waitForCondition(function() window.gInPrintPreviewMode);

    ok(window.gInPrintPreviewMode, "Entered print preview mode");

    
    PrintUtils.exitPrintPreview();
    yield waitForCondition(() => !window.gInPrintPreviewMode);
  }
});
