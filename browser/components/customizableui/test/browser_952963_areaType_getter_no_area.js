



"use strict";

const kToolbarName = "test-unregisterArea-areaType";
const kUnregisterAreaTestWidget = "test-widget-for-unregisterArea-areaType";
const kTestWidget = "test-widget-no-area-areaType";
registerCleanupFunction(removeCustomToolbars);

function checkAreaType(widget) {
  try {
    is(widget.areaType, null, "areaType should be null");
  } catch (ex) {
    info("Fetching areaType threw: " + ex);
    ok(false, "areaType getter shouldn't throw.");
  }
}


add_task(function() {
  
  
  let toolbarNode = createToolbarWithPlacements(kToolbarName, [kUnregisterAreaTestWidget]);
  CustomizableUI.unregisterArea(kToolbarName);
  toolbarNode.remove();

  let w = CustomizableUI.getWidget(kUnregisterAreaTestWidget);
  checkAreaType(w);

  w = CustomizableUI.getWidget(kTestWidget);
  checkAreaType(w);

  let spec = {id: kUnregisterAreaTestWidget, type: 'button', removable: true,
              label: "areaType test", tooltiptext: "areaType test"};
  CustomizableUI.createWidget(spec);
  toolbarNode = createToolbarWithPlacements(kToolbarName, [kUnregisterAreaTestWidget]);
  CustomizableUI.unregisterArea(kToolbarName);
  toolbarNode.remove();
  w = CustomizableUI.getWidget(spec.id);
  checkAreaType(w);
  CustomizableUI.removeWidgetFromArea(kUnregisterAreaTestWidget);
  checkAreaType(w);
  
  gAddedToolbars.delete(kToolbarName);
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});

