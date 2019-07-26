



"use strict";

function getPlacementArea(id) {
  let placement = CustomizableUI.getPlacementOfWidget(id);
  return placement && placement.area;
}



add_task(function() {
  const kWidgetId = "test-recreate-after-reset";
  let spec = {id: kWidgetId, label: "Test re-create after reset.",
              removable: true, defaultArea: CustomizableUI.AREA_NAVBAR};

  CustomizableUI.createWidget(spec);
  is(getPlacementArea(kWidgetId), CustomizableUI.AREA_NAVBAR,
     "widget is in the navigation bar");

  CustomizableUI.destroyWidget(kWidgetId);
  isnot(getPlacementArea(kWidgetId), CustomizableUI.AREA_NAVBAR,
        "widget removed from the navigation bar");

  CustomizableUI.reset();

  CustomizableUI.createWidget(spec);
  is(getPlacementArea(kWidgetId), CustomizableUI.AREA_NAVBAR,
     "widget recreated and added back to the nav bar");

  CustomizableUI.destroyWidget(kWidgetId);
});
