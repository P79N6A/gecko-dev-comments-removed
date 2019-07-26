



"use strict";

add_task(function() {
  info("Check save page button existence");

  yield PanelUI.show();

  let savePageButton = document.getElementById("save-page-button");
  ok(savePageButton, "Save Page button exists in Panel Menu");

  let panelHiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHiddenPromise;
});
