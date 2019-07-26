



const kToolbarName = "test-specials-toolbar";

let gTests = [
  {
    desc: "Add a toolbar with two springs and the downloads button.",
    run: function() {
      
      createToolbarWithPlacements(kToolbarName, ["spring"]);
      ok(document.getElementById(kToolbarName), "Toolbar should be created.");

      
      assertAreaPlacements(kToolbarName, [/customizableui-special-spring\d+/]);
      let [springId] = getAreaWidgetIds(kToolbarName);

      
      CustomizableUI.addWidgetToArea("spring", kToolbarName);
      assertAreaPlacements(kToolbarName, [springId,
                                          /customizableui-special-spring\d+/]);
      let [, spring2Id] = getAreaWidgetIds(kToolbarName);

      isnot(springId, spring2Id, "Springs shouldn't have identical IDs.");

      
      CustomizableUI.addWidgetToArea("downloads-button", kToolbarName, 1);
      assertAreaPlacements(kToolbarName, [springId, "downloads-button", spring2Id]);
    },
    teardown: removeCustomToolbars
  },
  {
    desc: "Add separators around the downloads button.",
    run: function() {
      createToolbarWithPlacements(kToolbarName, ["separator"]);
      ok(document.getElementById(kToolbarName), "Toolbar should be created.");

      
      assertAreaPlacements(kToolbarName, [/customizableui-special-separator\d+/]);
      let [separatorId] = getAreaWidgetIds(kToolbarName);

      CustomizableUI.addWidgetToArea("separator", kToolbarName);
      assertAreaPlacements(kToolbarName, [separatorId,
                                          /customizableui-special-separator\d+/]);
      let [, separator2Id] = getAreaWidgetIds(kToolbarName);

      isnot(separatorId, separator2Id, "Separator ids shouldn't be equal.");

      CustomizableUI.addWidgetToArea("downloads-button", kToolbarName, 1);
      assertAreaPlacements(kToolbarName, [separatorId, "downloads-button", separator2Id]);
    },
    teardown: removeCustomToolbars
  },
  {
    desc: "Add spacers around the downloads button.",
    run: function() {
      createToolbarWithPlacements(kToolbarName, ["spacer"]);
      ok(document.getElementById(kToolbarName), "Toolbar should be created.");

      
      assertAreaPlacements(kToolbarName, [/customizableui-special-spacer\d+/]);
      let [spacerId] = getAreaWidgetIds(kToolbarName);

      CustomizableUI.addWidgetToArea("spacer", kToolbarName);
      assertAreaPlacements(kToolbarName, [spacerId,
                                          /customizableui-special-spacer\d+/]);
      let [, spacer2Id] = getAreaWidgetIds(kToolbarName);

      isnot(spacerId, spacer2Id, "Spacer ids shouldn't be equal.");

      CustomizableUI.addWidgetToArea("downloads-button", kToolbarName, 1);
      assertAreaPlacements(kToolbarName, [spacerId, "downloads-button", spacer2Id]);
    },
    teardown: removeCustomToolbars
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
