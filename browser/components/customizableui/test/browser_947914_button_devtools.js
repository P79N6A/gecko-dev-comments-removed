



"use strict";

add_task(function() {
  info("Check developer tools button existence and functionality");
  yield PanelUI.show();

  let devtoolsButton = document.getElementById("developer-button");
  ok(devtoolsButton, "Developer tools button appears in Panel Menu");

  devtoolsButton.click();
  let devtoolsPanel = document.getElementById("PanelUI-developer");
  ok(devtoolsPanel.getAttribute("current"), "Developer tools Panel is in view");

  yield PanelUI.hide();
});
