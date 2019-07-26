



"use strict";

const kWidgetId = "test-non-removable-widget";


add_task(function() {
  let navbar = document.getElementById("nav-bar");
  ok(CustomizableUI.inDefaultState, "Should start in default state");

  let button = createDummyXULButton(kWidgetId, "Test non-removable inDefaultState handling");
  CustomizableUI.addWidgetToArea(kWidgetId, CustomizableUI.AREA_NAVBAR);
  button.setAttribute("removable", "false");
  ok(CustomizableUI.inDefaultState, "Should still be in default state after navbar addition");
  button.remove();

  button = createDummyXULButton(kWidgetId, "Test non-removable inDefaultState handling");
  CustomizableUI.addWidgetToArea(kWidgetId, CustomizableUI.AREA_PANEL);
  button.setAttribute("removable", "false");
  ok(CustomizableUI.inDefaultState, "Should still be in default state after panel addition");
  button.remove();
  ok(CustomizableUI.inDefaultState, "Should be in default state after destroying both widgets");
});
