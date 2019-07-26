



const kWidgetId = "test-892956-destroyWidget-defaultPlacement";

let gTests = [
  {
    desc: "destroyWidget should clean up defaultPlacements if the widget had a defaultArea",
    run: function() {
      ok(CustomizableUI.inDefaultState, "Should be in the default state when we start");

      let widgetSpec = {
        id: kWidgetId,
        defaultArea: CustomizableUI.AREA_NAVBAR
      };
      CustomizableUI.createWidget(widgetSpec);
      CustomizableUI.destroyWidget(kWidgetId);
      ok(CustomizableUI.inDefaultState, "Should be in the default state when we finish");
    }
  }
];

function asyncCleanup() {
  yield resetCustomization();
}

function test() {
  waitForExplicitFinish();
  runTests(gTests, asyncCleanup);
}
