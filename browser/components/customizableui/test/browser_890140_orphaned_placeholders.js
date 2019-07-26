



let gTests = [
  {
    desc: "One orphaned item should have two placeholders next to it.",
    setup: startCustomizing,
    run: function() {
      let btn = document.getElementById("developer-button");
      let panel = document.getElementById(CustomizableUI.AREA_PANEL);
      let placements = getAreaWidgetIds(CustomizableUI.AREA_PANEL);

      let placementsAfterAppend = placements.concat(["developer-button"]);
      simulateItemDrag(btn, panel);
      assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterAppend);
      ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
      is(getVisiblePlaceholderCount(panel), 2, "Should only have 2 visible placeholders before exiting");

      yield endCustomizing();
      yield startCustomizing();
      is(getVisiblePlaceholderCount(panel), 2, "Should only have 2 visible placeholders after re-entering");

      let palette = document.getElementById("customization-palette");
      simulateItemDrag(btn, palette);
      ok(CustomizableUI.inDefaultState, "Should be in default state again.");
    },
  },
  {
    desc: "Two orphaned items should have one placeholder next to them (case 1).",
    setup: startCustomizing,
    run: function() {
      let btn = document.getElementById("developer-button");
      let panel = document.getElementById(CustomizableUI.AREA_PANEL);
      let placements = getAreaWidgetIds(CustomizableUI.AREA_PANEL);

      let placementsAfterAppend = placements.concat(["developer-button", "sync-button"]);
      simulateItemDrag(btn, panel);
      btn = document.getElementById("sync-button");
      simulateItemDrag(btn, panel);
      assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterAppend);
      ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
      is(getVisiblePlaceholderCount(panel), 1, "Should only have 1 visible placeholders before exiting");

      yield endCustomizing();
      yield startCustomizing();
      is(getVisiblePlaceholderCount(panel), 1, "Should only have 1 visible placeholders after re-entering");

      let palette = document.getElementById("customization-palette");
      simulateItemDrag(btn, palette);
      btn = document.getElementById("developer-button");
      simulateItemDrag(btn, palette);
      ok(CustomizableUI.inDefaultState, "Should be in default state again.");
    },
  },
  {
    desc: "Two orphaned items should have one placeholder next to them (case 2).",
    setup: startCustomizing,
    run: function() {
      let btn = document.getElementById("add-ons-button");
      let panel = document.getElementById(CustomizableUI.AREA_PANEL);
      let palette = document.getElementById("customization-palette");
      let placements = getAreaWidgetIds(CustomizableUI.AREA_PANEL);

      let placementsAfterAppend = placements.filter(p => p != btn.id);
      simulateItemDrag(btn, palette);
      assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterAppend);
      ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
      is(getVisiblePlaceholderCount(panel), 1, "Should only have 1 visible placeholders before exiting");

      yield endCustomizing();
      yield startCustomizing();
      is(getVisiblePlaceholderCount(panel), 1, "Should only have 1 visible placeholders after re-entering");

      simulateItemDrag(btn, panel);
      assertAreaPlacements(CustomizableUI.AREA_PANEL, placements);
      ok(CustomizableUI.inDefaultState, "Should be in default state again.");
    },
  },
  {
    desc: "A wide widget at the bottom of the panel should have three placeholders after it.",
    setup: startCustomizing,
    run: function() {
      let btn = document.getElementById("edit-controls");
      let panel = document.getElementById(CustomizableUI.AREA_PANEL);
      let placements = getAreaWidgetIds(CustomizableUI.AREA_PANEL);

      let placementsAfterAppend = placements.concat([placements.shift()]);
      simulateItemDrag(btn, panel);
      assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterAppend);
      ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
      is(getVisiblePlaceholderCount(panel), 3, "Should have 3 visible placeholders before exiting");

      yield endCustomizing();
      yield startCustomizing();
      is(getVisiblePlaceholderCount(panel), 3, "Should have 3 visible placeholders after re-entering");

      let zoomControls = document.getElementById("zoom-controls");
      simulateItemDrag(btn, zoomControls);
      ok(CustomizableUI.inDefaultState, "Should be in default state again.");
    },
  },
  {
    desc: "The default placements should have three placeholders at the bottom.",
    setup: startCustomizing,
    run: function() {
      let panel = document.getElementById(CustomizableUI.AREA_PANEL);
      ok(CustomizableUI.inDefaultState, "Should be in default state.");
      is(getVisiblePlaceholderCount(panel), 3, "Should have 3 visible placeholders before exiting");

      yield endCustomizing();
      yield startCustomizing();
      is(getVisiblePlaceholderCount(panel), 3, "Should have 3 visible placeholders after re-entering");

      ok(CustomizableUI.inDefaultState, "Should still be in default state.");
    },
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

function getVisiblePlaceholderCount(aPanel) {
  let visiblePlaceholders = aPanel.querySelectorAll(".panel-customization-placeholder:not([hidden=true])");
  return visiblePlaceholders.length;
}
