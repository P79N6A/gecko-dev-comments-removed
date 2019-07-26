



"use strict";

const kWidgetId = "test-destroy-in-palette";


add_task(function() {
  CustomizableUI.createWidget({id: kWidgetId, label: "Test destroying widgets in palette."});
  yield startCustomizing();
  yield endCustomizing();
  ok(gNavToolbox.palette.querySelector("#" + kWidgetId), "Widget still exists in palette.");
  CustomizableUI.destroyWidget(kWidgetId);
  ok(!gNavToolbox.palette.querySelector("#" + kWidgetId), "Widget no longer exists in palette.");
});
