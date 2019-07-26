



"use strict";

let addonbarID = CustomizableUI.AREA_ADDONBAR;
let addonbar = document.getElementById(addonbarID);


add_task(function() {
  let placements = CustomizableUI.getWidgetIdsInArea(addonbarID);
  is(placements.join(','), addonbar.getAttribute("currentset"), "Addon-bar currentset should match default placements");
  ok(CustomizableUI.inDefaultState, "Should be in default state");
  info("Adding a spring to add-on bar shim");
  CustomizableUI.addWidgetToArea("spring", addonbarID, 1);
  ok(addonbar.getElementsByTagName("toolbarspring").length, "There should be a spring in the toolbar");
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state");
  placements = CustomizableUI.getWidgetIdsInArea(addonbarID);
  is(placements.join(','), addonbar.getAttribute("currentset"), "Addon-bar currentset should match placements after spring addition");

  yield startCustomizing();
  yield gCustomizeMode.reset();
  ok(CustomizableUI.inDefaultState, "Should be in default state after reset");
  placements = CustomizableUI.getWidgetIdsInArea(addonbarID);
  is(placements.join(','), addonbar.getAttribute("currentset"), "Addon-bar currentset should match default placements after reset");
  ok(!addonbar.getElementsByTagName("toolbarspring").length, "There should be no spring in the toolbar");
  yield endCustomizing();
});

