



"use strict";

add_task(function() {
  info("Check history button existence and functionality");

  yield PanelUI.show();

  let historyButton = document.getElementById("history-panelmenu");
  ok(historyButton, "History button appears in Panel Menu");

  historyButton.click();
  let historyPanel = document.getElementById("PanelUI-history");
  ok(historyPanel.getAttribute("current"), "History Panel is in view");

  let panelHiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHiddenPromise
});
