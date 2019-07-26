



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
    }
  },
  {
    desc: "Add separators around the downloads button.",
    run: function() {
      CustomizableUI.addWidgetToArea("separator", kToolbarName, 1);
      CustomizableUI.addWidgetToArea("separator", kToolbarName, 3);
      assertAreaPlacements(kToolbarName,
                           [/customizableui-special-spring\d+/,
                            /customizableui-special-separator\d+/,
                            "downloads-button",
                            /customizableui-special-separator\d+/,
                            /customizableui-special-spring\d+/]);
      let [,separator1Id,,separator2Id,] = getAreaWidgetIds(kToolbarName);
      isnot(separator1Id, separator2Id, "Separator ids shouldn't be equal.");
    }
  },
  {
    desc: "Add spacers around the downloads button.",
    run: function() {
      CustomizableUI.addWidgetToArea("spacer", kToolbarName, 2);
      CustomizableUI.addWidgetToArea("spacer", kToolbarName, 4);
      assertAreaPlacements(kToolbarName,
                           [/customizableui-special-spring\d+/,
                            /customizableui-special-separator\d+/,
                            /customizableui-special-spacer\d+/,
                            "downloads-button",
                            /customizableui-special-spacer\d+/,
                            /customizableui-special-separator\d+/,
                            /customizableui-special-spring\d+/]);
      let [,,spacer1Id,,spacer2Id,,] = getAreaWidgetIds(kToolbarName);
      isnot(spacer1Id, spacer2Id, "Spacer ids shouldn't be equal.");
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
