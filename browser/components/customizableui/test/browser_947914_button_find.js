



"use strict";

add_task(function() {
  info("Check find button existence and functionality");

  yield PanelUI.show();
  info("Menu panel was opened");

  let findButton = document.getElementById("find-button");
  ok(findButton, "Find button exists in Panel Menu");

  findButton.click();
  ok(!gFindBar.hasAttribute("hidden"), "Findbar opened successfully");

  
  gFindBar.close();
  info("Findbar was closed");
});
