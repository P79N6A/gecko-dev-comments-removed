



"use strict";

const kWidgetId = "test-892956-destroyWidget-defaultPlacement";


add_task(function() {
  ok(CustomizableUI.inDefaultState, "Should be in the default state when we start");

  let widgetSpec = {
    id: kWidgetId,
    defaultArea: CustomizableUI.AREA_NAVBAR
  };
  CustomizableUI.createWidget(widgetSpec);
  CustomizableUI.destroyWidget(kWidgetId);
  ok(CustomizableUI.inDefaultState, "Should be in the default state when we finish");
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});
