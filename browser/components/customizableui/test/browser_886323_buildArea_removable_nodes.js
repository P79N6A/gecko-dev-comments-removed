



"use strict";

const kButtonId = "test-886323-removable-moved-node";
const kLazyAreaId = "test-886323-lazy-area-for-removability-testing";

let gNavBar = document.getElementById(CustomizableUI.AREA_NAVBAR);
let gLazyArea;


add_task(function() {
  let dummyBtn = createDummyXULButton(kButtonId, "Dummy");
  dummyBtn.setAttribute("removable", "true");
  gNavBar.customizationTarget.appendChild(dummyBtn);
  let popupSet = document.getElementById("mainPopupSet");
  gLazyArea = document.createElementNS(kNSXUL, "panel");
  gLazyArea.id = kLazyAreaId;
  gLazyArea.setAttribute("hidden", "true");
  popupSet.appendChild(gLazyArea);
  CustomizableUI.registerArea(kLazyAreaId, {
    type: CustomizableUI.TYPE_MENU_PANEL,
    defaultPlacements: []
  });
  CustomizableUI.addWidgetToArea(kButtonId, kLazyAreaId);
  assertAreaPlacements(kLazyAreaId, [kButtonId],
                       "Placements should have changed because widget is removable.");
  let btn = document.getElementById(kButtonId);
  btn.setAttribute("removable", "false");
  gLazyArea.customizationTarget = gLazyArea;
  CustomizableUI.registerToolbarNode(gLazyArea, []);
  assertAreaPlacements(kLazyAreaId, [], "Placements should no longer include widget.");
  is(btn.parentNode.id, gNavBar.customizationTarget.id,
     "Button shouldn't actually have moved as it's not removable");
  btn = document.getElementById(kButtonId);
  if (btn) btn.remove();
  CustomizableUI.removeWidgetFromArea(kButtonId);
  CustomizableUI.unregisterArea(kLazyAreaId);
  gLazyArea.remove();
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});
