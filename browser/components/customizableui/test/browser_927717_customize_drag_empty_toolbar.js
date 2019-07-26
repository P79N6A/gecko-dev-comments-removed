



const kTestToolbarId = "test-empty-drag";
let gTests = [
  {
    desc: "Attempting to drag an item to an empty container should work.",
    setup: function() {
      createToolbarWithPlacements(kTestToolbarId, "");
    },
    run: function() {
      yield startCustomizing();
      let downloadButton = document.getElementById("downloads-button");
      let customToolbar = document.getElementById(kTestToolbarId);
      simulateItemDrag(downloadButton, customToolbar);
      assertAreaPlacements(kTestToolbarId, ["downloads-button"]);
      ok(downloadButton.parentNode && downloadButton.parentNode.parentNode == customToolbar,
         "Button should really be in toolbar");
    },
    teardown: function() {
      yield endCustomizing();
      removeCustomToolbars();
    }
  },
];
function asyncCleanup() {
  yield endCustomizing();
  Services.prefs.clearUserPref("browser.uiCustomization.skipSourceNodeCheck");
  yield resetCustomization();
}

function test() {
  Services.prefs.setBoolPref("browser.uiCustomization.skipSourceNodeCheck", true);
  waitForExplicitFinish();
  runTests(gTests, asyncCleanup);
}
