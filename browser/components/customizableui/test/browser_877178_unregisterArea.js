



let gTests = [
  {
    desc: "Sanity checks",
    run: function() {
      SimpleTest.doesThrow(function() CustomizableUI.registerArea("@foo"),
                           "Registering areas with an invalid ID should throw.");

      SimpleTest.doesThrow(function() CustomizableUI.registerArea([]),
                           "Registering areas with an invalid ID should throw.");

      SimpleTest.doesThrow(function() CustomizableUI.registerArea(CustomizableUI.AREA_NAVBAR),
                           "Registering an area with an ID that's already registered should throw.");

      SimpleTest.doesThrow(function() CustomizableUI.unregisterArea("@foo"),
                           "Unregistering areas with an invalid ID should throw.");

      SimpleTest.doesThrow(function() CustomizableUI.unregisterArea([]),
                           "Unregistering areas with an invalid ID should throw.");

      SimpleTest.doesThrow(function() CustomizableUI.unregisterArea("unknown"),
                           "Unregistering an area that's not registered should throw.");
    }
  },
  {
    desc: "Check areas are loaded with their default placements.",
    run: function() {
      ok(CustomizableUI.inDefaultState, "Everything should be in its default state.");
    }
  },
  {
    desc: "Check registering and unregistering a new area.",
    run: function() {
      const kToolbarId = "test-registration-toolbar";
      const kButtonId = "test-registration-button";
      createDummyXULButton(kButtonId);
      createToolbarWithPlacements(kToolbarId, ["spring", kButtonId, "spring"]);
      assertAreaPlacements(kToolbarId,
                           [/customizableui-special-spring\d+/,
                            kButtonId,
                            /customizableui-special-spring\d+/]);
      ok(CustomizableUI.inDefaultState, "With a new toolbar and default placements, " +
                                        "everything should still be in a default state.");
      removeCustomToolbars(); 
      ok(CustomizableUI.inDefaultState, "When the toolbar is unregistered, " +
                                        "everything should still be in a default state.");
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

