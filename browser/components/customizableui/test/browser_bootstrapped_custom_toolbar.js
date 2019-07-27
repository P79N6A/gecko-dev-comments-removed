



"use strict";

const kTestBarID = "testBar";
const kWidgetID = "characterencoding-button";

function createTestBar(aLegacy) {
  let testBar = document.createElement("toolbar");
  testBar.id = kTestBarID;
  testBar.setAttribute("customizable", "true");
  CustomizableUI.registerArea(kTestBarID, {
    type: CustomizableUI.TYPE_TOOLBAR,
    legacy: aLegacy,
  });
  gNavToolbox.appendChild(testBar);
  return testBar;
}





















function checkRestoredPresence(aWidgetID, aLegacy) {
  return Task.spawn(function* () {
    let testBar = createTestBar(aLegacy);
    CustomizableUI.addWidgetToArea(aWidgetID, kTestBarID);
    let placement = CustomizableUI.getPlacementOfWidget(aWidgetID);
    is(placement.area, kTestBarID,
       "Expected " + aWidgetID + " to be in the test toolbar");

    CustomizableUI.unregisterArea(testBar.id);
    testBar.remove();

    placement = CustomizableUI.getPlacementOfWidget(aWidgetID);
    is(placement, null, "Expected " + aWidgetID + " to be in the palette");

    testBar = createTestBar(aLegacy);

    yield startCustomizing();
    placement = CustomizableUI.getPlacementOfWidget(aWidgetID);
    is(placement.area, kTestBarID,
       "Expected " + aWidgetID + " to be in the test toolbar");
    yield endCustomizing();

    CustomizableUI.unregisterArea(testBar.id);
    testBar.remove();

    yield resetCustomization();
  });
}

add_task(function* () {
  yield checkRestoredPresence("downloads-button", false);
  yield checkRestoredPresence("downloads-button", true);
});

add_task(function* () {
  yield checkRestoredPresence("characterencoding-button", false);
  yield checkRestoredPresence("characterencoding-button", true);
});
