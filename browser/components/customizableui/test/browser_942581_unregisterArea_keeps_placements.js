



const kToolbarName = "test-unregisterArea-placements-toolbar";
const kTestWidgetPfx = "test-widget-for-unregisterArea-placements-";
const kTestWidgetCount = 3;

let gTests = [
  {
    desc: "unregisterArea should keep placements by default and restore them when re-adding the area",
    run: function() {
      let widgetIds = []
      for (let i = 0; i < kTestWidgetCount; i++) {
        let id = kTestWidgetPfx + i;
        widgetIds.push(id);
        let spec = {id: id, type: 'button', removable: true, label: "unregisterArea test", tooltiptext: "" + i};
        CustomizableUI.createWidget(spec);
      }
      for (let i = kTestWidgetCount; i < kTestWidgetCount * 2; i++) {
        let id = kTestWidgetPfx + i;
        widgetIds.push(id);
        createDummyXULButton(id, "unregisterArea XUL test " + i);
      }
      let toolbarNode = createToolbarWithPlacements(kToolbarName, widgetIds);
      checkAbstractAndRealPlacements(toolbarNode, widgetIds);

      
      CustomizableUI.moveWidgetWithinArea(kTestWidgetPfx + kTestWidgetCount, 0);
      
      let modifiedWidgetIds = [...widgetIds];
      let movedWidget = modifiedWidgetIds.splice(kTestWidgetCount, 1)[0];
      modifiedWidgetIds.unshift(movedWidget);

      
      checkAbstractAndRealPlacements(toolbarNode, modifiedWidgetIds);

      
      CustomizableUI.unregisterArea(kToolbarName);

      
      checkWidgetFates(widgetIds);
      
      toolbarNode.remove();

      
      let lastWidget = modifiedWidgetIds.pop();
      CustomizableUI.removeWidgetFromArea(lastWidget);
      lastWidget = modifiedWidgetIds.pop();
      CustomizableUI.addWidgetToArea(lastWidget, CustomizableUI.AREA_NAVBAR);

      
      toolbarNode = createToolbarWithPlacements(kToolbarName, widgetIds);
      
      
      checkAbstractAndRealPlacements(toolbarNode, modifiedWidgetIds);

      
      CustomizableUI.unregisterArea(kToolbarName, true);
      checkWidgetFates(modifiedWidgetIds);
      toolbarNode.remove();

      
      
      toolbarNode = createToolbarWithPlacements(kToolbarName, widgetIds);
      
      checkAbstractAndRealPlacements(toolbarNode, widgetIds);
      CustomizableUI.unregisterArea(kToolbarName, true);
      checkWidgetFates(widgetIds);
      toolbarNode.remove();

      
      gAddedToolbars.delete(kToolbarName);

      
      for (let widget of widgetIds) {
        let widgetWrapper = CustomizableUI.getWidget(widget);
        if (widgetWrapper.provider == CustomizableUI.PROVIDER_XUL) {
          gNavToolbox.palette.querySelector("#" + widget).remove();
        } else {
          CustomizableUI.destroyWidget(widget);
        }
      }
    },
  }
];

function checkAbstractAndRealPlacements(aNode, aExpectedPlacements) {
  assertAreaPlacements(kToolbarName, aExpectedPlacements);
  let physicalWidgetIds = [node.id for (node of aNode.childNodes)];
  placementArraysEqual(aNode.id, physicalWidgetIds, aExpectedPlacements);
}

function checkWidgetFates(aWidgetIds) {
  for (let widget of aWidgetIds) {
    ok(!CustomizableUI.getPlacementOfWidget(widget), "Widget should be in palette");
    ok(!document.getElementById(widget), "Widget should not be in the DOM");
    let widgetInPalette = !!gNavToolbox.palette.querySelector("#" + widget);
    let widgetProvider = CustomizableUI.getWidget(widget).provider;
    let widgetIsXULWidget = widgetProvider == CustomizableUI.PROVIDER_XUL;
    is(widgetInPalette, widgetIsXULWidget, "Just XUL Widgets should be in the palette");
  }
}

function asyncCleanup() {
  yield resetCustomization();
}

function cleanup() {
  removeCustomToolbars();
}

function test() {
  waitForExplicitFinish();
  registerCleanupFunction(cleanup);
  runTests(gTests, asyncCleanup);
}

