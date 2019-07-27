



"use strict";

const BUTTONID = "test-widget-saved-earlier";
const AREAID = "test-area-saved-earlier";

let hadSavedState;
function test() {
  
  let backstagePass = Cu.import("resource:///modules/CustomizableUI.jsm", {});
  hadSavedState = backstagePass.gSavedState != null;
  if (!hadSavedState) {
    backstagePass.gSavedState = {placements: {}};
  }
  backstagePass.gSavedState.placements[AREAID] = [BUTTONID];
  
  
  backstagePass.gSavedState.placements[CustomizableUI.AREA_NAVBAR] = ["bogus-navbar-item"];

  backstagePass.gDirty = true;
  backstagePass.CustomizableUIInternal.saveState();

  let newSavedState = JSON.parse(Services.prefs.getCharPref("browser.uiCustomization.state"));
  let savedArea = Array.isArray(newSavedState.placements[AREAID]);
  ok(savedArea, "Should have re-saved the state, even though the area isn't registered");

  if (savedArea) {
    placementArraysEqual(AREAID, newSavedState.placements[AREAID], [BUTTONID]);
  }
  ok(!backstagePass.gPlacements.has(AREAID), "Placements map shouldn't have been affected");

  let savedNavbar = Array.isArray(newSavedState.placements[CustomizableUI.AREA_NAVBAR]);
  ok(savedNavbar, "Should have saved nav-bar contents");
  if (savedNavbar) {
    placementArraysEqual(CustomizableUI.AREA_NAVBAR, newSavedState.placements[CustomizableUI.AREA_NAVBAR],
                         CustomizableUI.getWidgetIdsInArea(CustomizableUI.AREA_NAVBAR));
  }
};

registerCleanupFunction(function() {
  let backstagePass = Cu.import("resource:///modules/CustomizableUI.jsm", {});
  if (!hadSavedState) {
    backstagePass.gSavedState = null;
  } else {
    let savedPlacements = backstagePass.gSavedState.placements;
    delete savedPlacements[AREAID];
    let realNavBarPlacements = CustomizableUI.getWidgetIdsInArea(CustomizableUI.AREA_NAVBAR);
    savedPlacements[CustomizableUI.AREA_NAVBAR] = realNavBarPlacements;
  }
  backstagePass.gDirty = true;
  backstagePass.CustomizableUIInternal.saveState();
});

