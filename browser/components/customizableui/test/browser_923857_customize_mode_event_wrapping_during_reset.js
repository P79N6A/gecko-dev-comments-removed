



let gTests = [
  {
    desc: "Customize mode reset button should revert correctly",
    setup: function() {
      yield startCustomizing();
    },
    run: function() {
      let devButton = document.getElementById("developer-button");
      let downloadsButton = document.getElementById("downloads-button");
      let searchBox = document.getElementById("search-container");
      let palette = document.getElementById("customization-palette");
      ok(devButton && downloadsButton && searchBox && palette, "Stuff should exist");
      simulateItemDrag(devButton, downloadsButton);
      simulateItemDrag(searchBox, palette);
      gCustomizeMode.reset();
      yield waitForCondition(function() !gCustomizeMode.resetting);
      ok(CustomizableUI.inDefaultState, "Should be back in default state");
    },
    teardown: function() {
      yield endCustomizing();
    }
  }
];

function asyncCleanup() {
  Services.prefs.clearUserPref("browser.uiCustomization.skipSourceNodeCheck");
  yield resetCustomization();
}

function test() {
  Services.prefs.setBoolPref("browser.uiCustomization.skipSourceNodeCheck", true);
  waitForExplicitFinish();
  runTests(gTests, asyncCleanup);
}
