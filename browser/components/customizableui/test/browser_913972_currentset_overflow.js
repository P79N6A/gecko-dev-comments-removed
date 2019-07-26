



"use strict";

let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);


add_task(function() {
  let originalWindowWidth = window.outerWidth;
  let oldCurrentSet = navbar.currentSet;
  ok(!navbar.hasAttribute("overflowing"), "Should start with a non-overflowing toolbar.");
  ok(CustomizableUI.inDefaultState, "Should start in default state.");
  let oldChildCount = navbar.customizationTarget.childElementCount;
  window.resizeTo(400, window.outerHeight);
  yield waitForCondition(() => navbar.hasAttribute("overflowing"));
  ok(navbar.hasAttribute("overflowing"), "Should have an overflowing toolbar.");
  is(navbar.currentSet, oldCurrentSet, "Currentset should be the same when overflowing.");
  ok(CustomizableUI.inDefaultState, "Should still be in default state when overflowing.");
  ok(navbar.customizationTarget.childElementCount < oldChildCount, "Should have fewer children.");
  window.resizeTo(originalWindowWidth, window.outerHeight);
  yield waitForCondition(() => !navbar.hasAttribute("overflowing"));
  ok(!navbar.hasAttribute("overflowing"), "Should no longer have an overflowing toolbar.");
  is(navbar.currentSet, oldCurrentSet, "Currentset should still be the same now we're no longer overflowing.");
  ok(CustomizableUI.inDefaultState, "Should still be in default state now we're no longer overflowing.");

  
  let placementCounter = 0;
  let placements = CustomizableUI.getWidgetIdsInArea(CustomizableUI.AREA_NAVBAR);
  for (let node of navbar.customizationTarget.childNodes) {
    if (node.getAttribute("skipintoolbarset") == "true") {
      continue;
    }
    is(placements[placementCounter++], node.id, "Nodes should match after overflow");
  }
  is(placements.length, placementCounter, "Should have as many nodes as expected");
  is(navbar.customizationTarget.childElementCount, oldChildCount, "Number of nodes should match");
});


add_task(function() {
  let oldCurrentSet = navbar.currentSet;
  ok(CustomizableUI.inDefaultState, "Should start in default state.");
  yield startCustomizing();
  ok(CustomizableUI.inDefaultState, "Should be in default state in customization mode.");
  is(navbar.currentSet, oldCurrentSet, "Currentset should be the same in customization mode.");
  yield endCustomizing();
  ok(CustomizableUI.inDefaultState, "Should be in default state after customization mode.");
  is(navbar.currentSet, oldCurrentSet, "Currentset should be the same after customization mode.");
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});
