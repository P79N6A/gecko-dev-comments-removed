





const kWidgetId = "test-invalidate-wrapper-cache";

let gTests = [
  {
    desc: "Check createWidget invalidates the widget cache",
    run: function() {
      let groupWrapper = CustomizableUI.getWidget(kWidgetId);
      ok(groupWrapper, "Should get group wrapper.");
      let singleWrapper = groupWrapper.forWindow(window);
      ok(singleWrapper, "Should get single wrapper.");

      CustomizableUI.createWidget({id: kWidgetId, label: "Test invalidating widgets caching"});

      let newGroupWrapper = CustomizableUI.getWidget(kWidgetId);
      ok(newGroupWrapper, "Should get a group wrapper again.");
      isnot(newGroupWrapper, groupWrapper, "Wrappers shouldn't be the same.");
      isnot(newGroupWrapper.provider, groupWrapper.provider, "Wrapper providers shouldn't be the same.");

      let newSingleWrapper = newGroupWrapper.forWindow(window);
      isnot(newSingleWrapper, singleWrapper, "Single wrappers shouldn't be the same.");
      isnot(newSingleWrapper.provider, singleWrapper.provider, "Single wrapper providers shouldn't be the same.");

      CustomizableUI.destroyWidget(kWidgetId);
      ok(!CustomizableUI.getWidget(kWidgetId), "Shouldn't get a wrapper after destroying the widget.");
    },
  },
];

function test() {
  waitForExplicitFinish();
  runTests(gTests);
}
