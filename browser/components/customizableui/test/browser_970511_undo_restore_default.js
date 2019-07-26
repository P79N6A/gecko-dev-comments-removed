



"use strict";


add_task(function() {
  let homeButtonId = "home-button";
  CustomizableUI.removeWidgetFromArea(homeButtonId);
  yield startCustomizing();
  ok(!CustomizableUI.inDefaultState, "Not in default state to begin with");
  is(CustomizableUI.getPlacementOfWidget(homeButtonId), null, "Home button is in palette");
  let undoReset = document.getElementById("customization-undo-reset");
  is(undoReset.hidden, true, "The undo button is hidden before reset");

  yield gCustomizeMode.reset();

  ok(CustomizableUI.inDefaultState, "In default state after reset");
  is(undoReset.hidden, false, "The undo button is visible after reset");

  undoReset.click();
  yield waitForCondition(function() !gCustomizeMode.resetting);
  ok(!CustomizableUI.inDefaultState, "Not in default state after reset-undo");
  is(undoReset.hidden, true, "The undo button is hidden after clicking on the undo button");
  is(CustomizableUI.getPlacementOfWidget(homeButtonId), null, "Home button is in palette");

  yield gCustomizeMode.reset();
});


add_task(function() {
  let homeButtonId = "home-button";
  CustomizableUI.removeWidgetFromArea(homeButtonId);
  ok(!CustomizableUI.inDefaultState, "Not in default state to begin with");
  is(CustomizableUI.getPlacementOfWidget(homeButtonId), null, "Home button is in palette");
  let undoReset = document.getElementById("customization-undo-reset");
  is(undoReset.hidden, true, "The undo button is hidden before reset");

  yield gCustomizeMode.reset();

  ok(CustomizableUI.inDefaultState, "In default state after reset");
  is(undoReset.hidden, false, "The undo button is visible after reset");

  CustomizableUI.addWidgetToArea(homeButtonId, CustomizableUI.AREA_PANEL);
  is(undoReset.hidden, true, "The undo button is hidden after another change");
});


add_task(function() {
  let undoReset = document.getElementById("customization-undo-reset");
  is(undoReset.hidden, true, "The undo button is hidden before a reset");
  ok(!CustomizableUI.inDefaultState, "The browser should not be in default state");
  yield gCustomizeMode.reset();

  is(undoReset.hidden, false, "The undo button is hidden after a reset");
  yield endCustomizing();
  yield startCustomizing();
  is(undoReset.hidden, true, "The undo reset button should be hidden after entering customization mode");
});

add_task(function asyncCleanup() {
  yield gCustomizeMode.reset();
  yield endCustomizing();
});
