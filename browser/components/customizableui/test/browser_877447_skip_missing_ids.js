



let gTests = [
  {
    run: function() {
      const kButtonId = "look-at-me-disappear-button";
      CustomizableUI.reset();
      ok(CustomizableUI.inDefaultState, "Should be in the default state.");
      let btn = createDummyXULButton(kButtonId, "Gone!");
      CustomizableUI.addWidgetToArea(kButtonId, CustomizableUI.AREA_NAVBAR);
      ok(!CustomizableUI.inDefaultState, "Should no longer be in the default state.");
      is(btn.parentNode.parentNode.id, CustomizableUI.AREA_NAVBAR, "Button should be in navbar");
      btn.remove();
      is(btn.parentNode, null, "Button is no longer in the navbar");
      ok(CustomizableUI.inDefaultState, "Should be back in the default state, " +
                                        "despite unknown button ID in placements.");
    }
  }
];

function cleanup() {
  removeCustomToolbars();
  resetCustomization();
}

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(cleanup);
  runTests(gTests);
}
