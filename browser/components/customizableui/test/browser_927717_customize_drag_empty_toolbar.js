



"use strict";

const kTestToolbarId = "test-empty-drag";
Services.prefs.setBoolPref("browser.uiCustomization.skipSourceNodeCheck", true);


add_task(function() {
  yield createToolbarWithPlacements(kTestToolbarId, []);
  yield startCustomizing();
  let downloadButton = document.getElementById("downloads-button");
  let customToolbar = document.getElementById(kTestToolbarId);
  simulateItemDrag(downloadButton, customToolbar);
  assertAreaPlacements(kTestToolbarId, ["downloads-button"]);
  ok(downloadButton.parentNode && downloadButton.parentNode.parentNode == customToolbar,
     "Button should really be in toolbar");
  yield endCustomizing();
  removeCustomToolbars();
});

add_task(function asyncCleanup() {
  yield endCustomizing();
  Services.prefs.clearUserPref("browser.uiCustomization.skipSourceNodeCheck");
  yield resetCustomization();
});
