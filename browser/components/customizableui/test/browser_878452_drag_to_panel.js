



"use strict";


add_task(function() {
  yield startCustomizing();
  let btn = document.getElementById("feed-button");
  let panel = document.getElementById(CustomizableUI.AREA_PANEL);
  let placements = getAreaWidgetIds(CustomizableUI.AREA_PANEL);

  let lastButtonIndex = placements.length - 1;
  let lastButton = placements[lastButtonIndex];
  let placementsAfterInsert = placements.slice(0, lastButtonIndex).concat(["feed-button", lastButton]);
  let lastButtonNode = document.getElementById(lastButton);
  simulateItemDrag(btn, lastButtonNode);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterInsert);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let palette = document.getElementById("customization-palette");
  simulateItemDrag(btn, palette);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});


add_task(function() {
  yield startCustomizing();
  let btn = document.getElementById("feed-button");
  let panel = document.getElementById(CustomizableUI.AREA_PANEL);
  let placements = getAreaWidgetIds(CustomizableUI.AREA_PANEL);

  let placementsAfterAppend = placements.concat(["feed-button"]);
  simulateItemDrag(btn, panel);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterAppend);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let palette = document.getElementById("customization-palette");
  simulateItemDrag(btn, palette);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});


add_task(function() {
  let widgetIds = getAreaWidgetIds(CustomizableUI.AREA_PANEL);
  while (widgetIds.length) {
    CustomizableUI.removeWidgetFromArea(widgetIds.shift());
  }
  yield startCustomizing();
  let btn = document.getElementById("feed-button");
  let panel = document.getElementById(CustomizableUI.AREA_PANEL);

  assertAreaPlacements(panel.id, []);

  let placementsAfterAppend = ["feed-button"];
  simulateItemDrag(btn, panel);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterAppend);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let palette = document.getElementById("customization-palette");
  simulateItemDrag(btn, palette);
  assertAreaPlacements(panel.id, []);
});

add_task(function asyncCleanup() {
  yield endCustomizing();
  yield resetCustomization();
});
