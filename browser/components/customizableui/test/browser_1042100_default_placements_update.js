


"use strict";



function test() {
  
  CustomizableUI.addWidgetToArea("feed-button", CustomizableUI.AREA_NAVBAR);

  
  let CustomizableUIBSPass = Cu.import("resource:///modules/CustomizableUI.jsm", {});

  is(CustomizableUIBSPass.gFuturePlacements.size, 0,
     "All future placements should be dealt with by now.");

  let {CustomizableUIInternal, gFuturePlacements, gPalette} = CustomizableUIBSPass;
  CustomizableUIInternal._introduceNewBuiltinWidgets();
  is(gFuturePlacements.size, 0,
     "No change to future placements initially.");

  let currentVersion = CustomizableUIBSPass.kVersion;


  
  let testWidgetNew = {
    id: "test-messing-with-default-placements-new",
    label: "Test messing with default placements - should be inserted",
    defaultArea: CustomizableUI.AREA_NAVBAR,
    introducedInVersion: currentVersion + 1,
  };

  let normalizedWidget = CustomizableUIInternal.normalizeWidget(testWidgetNew,
                                                                CustomizableUI.SOURCE_BUILTIN);
  ok(normalizedWidget, "Widget should be normalizable");
  if (!normalizedWidget) {
    return;
  }
  CustomizableUIBSPass.gPalette.set(testWidgetNew.id, normalizedWidget);

  let testWidgetOld = {
    id: "test-messing-with-default-placements-old",
    label: "Test messing with default placements - should NOT be inserted",
    defaultArea: CustomizableUI.AREA_NAVBAR,
    introducedInVersion: currentVersion,
  };

  normalizedWidget = CustomizableUIInternal.normalizeWidget(testWidgetOld,
                                                            CustomizableUI.SOURCE_BUILTIN);
  ok(normalizedWidget, "Widget should be normalizable");
  if (!normalizedWidget) {
    return;
  }
  CustomizableUIBSPass.gPalette.set(testWidgetOld.id, normalizedWidget);


  
  CustomizableUIBSPass.kVersion++;

  let hadSavedState = !!CustomizableUIBSPass.gSavedState
  if (!hadSavedState) {
    CustomizableUIBSPass.gSavedState = {currentVersion: CustomizableUIBSPass.kVersion - 1};
  }

  
  CustomizableUIInternal._introduceNewBuiltinWidgets();
  is(gFuturePlacements.size, 1,
     "Should have 1 more future placement");
  let haveNavbarPlacements = gFuturePlacements.has(CustomizableUI.AREA_NAVBAR);
  ok(haveNavbarPlacements, "Should have placements for nav-bar");
  if (haveNavbarPlacements) {
    let placements = [...gFuturePlacements.get(CustomizableUI.AREA_NAVBAR)];
    is(placements.length, 1, "Should have 1 newly placed widget in nav-bar");
    is(placements[0], testWidgetNew.id, "Should have our test widget to be placed in nav-bar");
  }

  gFuturePlacements.delete(CustomizableUI.AREA_NAVBAR);
  CustomizableUIBSPass.kVersion--;
  gPalette.delete(testWidgetNew.id);
  gPalette.delete(testWidgetOld.id);
  if (!hadSavedState) {
    CustomizableUIBSPass.gSavedState = null;
  }
}

