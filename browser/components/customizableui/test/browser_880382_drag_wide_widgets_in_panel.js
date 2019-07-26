



"use strict";

Services.prefs.setBoolPref("browser.uiCustomization.skipSourceNodeCheck", true);
requestLongerTimeout(5);


add_task(function() {
  yield startCustomizing();
  let zoomControls = document.getElementById("zoom-controls");
  let printButton = document.getElementById("print-button");
  let placementsAfterMove = ["edit-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "zoom-controls",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(zoomControls, printButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let newWindowButton = document.getElementById("new-window-button");
  simulateItemDrag(zoomControls, newWindowButton);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});


add_task(function() {
  yield startCustomizing();
  let zoomControls = document.getElementById("zoom-controls");
  let savePageButton = document.getElementById("save-page-button");
  let placementsAfterMove = ["edit-controls",
                             "zoom-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(zoomControls, savePageButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  ok(CustomizableUI.inDefaultState, "Should be in default state.");
});



add_task(function() {
  yield startCustomizing();
  let zoomControls = document.getElementById("zoom-controls");
  let newWindowButton = document.getElementById("new-window-button");
  let placementsAfterMove = ["edit-controls",
                             "zoom-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(zoomControls, newWindowButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});


add_task(function() {
  yield startCustomizing();
  let zoomControls = document.getElementById("zoom-controls");
  let historyPanelMenu = document.getElementById("history-panelmenu");
  let placementsAfterMove = ["edit-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "zoom-controls",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(zoomControls, historyPanelMenu);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let newWindowButton = document.getElementById("new-window-button");
  simulateItemDrag(zoomControls, newWindowButton);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});



add_task(function() {
  yield startCustomizing();
  let zoomControls = document.getElementById("zoom-controls");
  let preferencesButton = document.getElementById("preferences-button");
  let placementsAfterMove = ["edit-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "zoom-controls",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(zoomControls, preferencesButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let newWindowButton = document.getElementById("new-window-button");
  simulateItemDrag(zoomControls, newWindowButton);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});


add_task(function() {
  yield startCustomizing();
  let developerButton = document.getElementById("developer-button");
  let zoomControls = document.getElementById("zoom-controls");
  let placementsAfterInsert = ["edit-controls",
                               "developer-button",
                               "new-window-button",
                               "privatebrowsing-button",
                               "zoom-controls",
                               "save-page-button",
                               "print-button",
                               "history-panelmenu",
                               "fullscreen-button",
                               "find-button",
                               "preferences-button",
                               "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterInsert);
  simulateItemDrag(developerButton, zoomControls);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterInsert);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let palette = document.getElementById("customization-palette");
  
  let feedWrapper = document.getElementById("wrapper-feed-button");
  let feedButton = document.getElementById("feed-button");
  is(feedButton.parentNode, feedWrapper,
     "feed-button should be a child of wrapper-feed-button");
  is(feedWrapper.getAttribute("place"), "palette",
     "The feed-button wrapper should have it's place set to 'palette'");
  simulateItemDrag(developerButton, palette);
  is(developerButton.parentNode.tagName, "toolbarpaletteitem",
     "The developer-button should be wrapped by a toolbarpaletteitem");
  let newWindowButton = document.getElementById("new-window-button");
  simulateItemDrag(zoomControls, newWindowButton);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});



add_task(function() {
  yield startCustomizing();
  let developerButton = document.getElementById("developer-button");
  let editControls = document.getElementById("edit-controls");
  let placementsAfterInsert = ["developer-button",
                               "new-window-button",
                               "privatebrowsing-button",
                               "edit-controls",
                               "zoom-controls",
                               "save-page-button",
                               "print-button",
                               "history-panelmenu",
                               "fullscreen-button",
                               "find-button",
                               "preferences-button",
                               "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterInsert);
  simulateItemDrag(developerButton, editControls);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterInsert);
  ok(!CustomizableUI.inDefaultState, "Should no longer be in default state.");
  let palette = document.getElementById("customization-palette");
  
  let feedWrapper = document.getElementById("wrapper-feed-button");
  let feedButton = document.getElementById("feed-button");
  is(feedButton.parentNode, feedWrapper,
     "feed-button should be a child of wrapper-feed-button");
  is(feedWrapper.getAttribute("place"), "palette",
     "The feed-button wrapper should have it's place set to 'palette'");
  simulateItemDrag(developerButton, palette);
  is(developerButton.parentNode.tagName, "toolbarpaletteitem",
     "The developer-button should be wrapped by a toolbarpaletteitem");
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});



add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let zoomControls = document.getElementById("zoom-controls");
  let placementsAfterMove = ["edit-controls",
                             "zoom-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(editControls, zoomControls);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});



add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let newWindowButton = document.getElementById("new-window-button");
  let placementsAfterMove = ["zoom-controls",
                             "edit-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(editControls, newWindowButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  let zoomControls = document.getElementById("zoom-controls");
  simulateItemDrag(editControls, zoomControls);
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});




add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let privateBrowsingButton = document.getElementById("privatebrowsing-button");
  let placementsAfterMove = ["zoom-controls",
                             "edit-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(editControls, privateBrowsingButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  let zoomControls = document.getElementById("zoom-controls");
  simulateItemDrag(editControls, zoomControls);
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});




add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let savePageButton = document.getElementById("save-page-button");
  let placementsAfterMove = ["zoom-controls",
                             "edit-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(editControls, savePageButton);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  let zoomControls = document.getElementById("zoom-controls");
  simulateItemDrag(editControls, zoomControls);
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});



add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let panel = document.getElementById(CustomizableUI.AREA_PANEL);
  let placementsAfterMove = ["zoom-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button",
                             "edit-controls"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(editControls, panel);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  let zoomControls = document.getElementById("zoom-controls");
  simulateItemDrag(editControls, zoomControls);
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});



add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let palette = document.getElementById("customization-palette");
  let placementsAfterMove = ["zoom-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  let paletteChildElementCount = palette.childElementCount;
  simulateItemDrag(editControls, palette);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  is(paletteChildElementCount + 1, palette.childElementCount,
     "The palette should have a new child, congratulations!");
  is(editControls.parentNode.id, "wrapper-edit-controls",
     "The edit-controls should be properly wrapped.");
  is(editControls.parentNode.getAttribute("place"), "palette",
     "The edit-controls should have the place of 'palette'.");
  let zoomControls = document.getElementById("zoom-controls");
  simulateItemDrag(editControls, zoomControls);
  is(paletteChildElementCount, palette.childElementCount,
     "The palette child count should have returned to its prior value.");
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});



add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let panel = document.getElementById(CustomizableUI.AREA_PANEL);
  let numPlaceholders = isInWin8() ? 2 : 3;
  for (let i = 0; i < numPlaceholders; i++) {
    
    
    let placeholder = panel.getElementsByClassName("panel-customization-placeholder")[i];
    let placementsAfterMove = ["zoom-controls",
                               "new-window-button",
                               "privatebrowsing-button",
                               "save-page-button",
                               "print-button",
                               "history-panelmenu",
                               "fullscreen-button",
                               "find-button",
                               "preferences-button",
                               "add-ons-button",
                               "edit-controls"];
    addSwitchToMetroButtonInWindows8(placementsAfterMove);
    simulateItemDrag(editControls, placeholder);
    assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
    let zoomControls = document.getElementById("zoom-controls");
    simulateItemDrag(editControls, zoomControls);
    ok(CustomizableUI.inDefaultState, "Should still be in default state.");
  }
});


add_task(function() {
  yield startCustomizing();
  let developerButton = document.getElementById("developer-button");
  is(developerButton.parentNode.tagName, "toolbarpaletteitem",
     "developer-button should be wrapped by a toolbarpaletteitem");
  simulateItemDrag(developerButton, developerButton);
  is(developerButton.parentNode.tagName, "toolbarpaletteitem",
     "developer-button should be wrapped by a toolbarpaletteitem");
  let editControls = document.getElementById("edit-controls");
  is(editControls.parentNode.tagName, "toolbarpaletteitem",
     "edit-controls should be wrapped by a toolbarpaletteitem");
  ok(CustomizableUI.inDefaultState, "Should still be in default state.");
});


add_task(function() {
  yield startCustomizing();
  let editControls = document.getElementById("edit-controls");
  let panel = document.getElementById(CustomizableUI.AREA_PANEL);
  let target = panel.getElementsByClassName("panel-customization-placeholder")[0];
  let placementsAfterMove = ["zoom-controls",
                             "new-window-button",
                             "privatebrowsing-button",
                             "save-page-button",
                             "print-button",
                             "history-panelmenu",
                             "fullscreen-button",
                             "find-button",
                             "preferences-button",
                             "add-ons-button",
                             "edit-controls"];
  addSwitchToMetroButtonInWindows8(placementsAfterMove);
  simulateItemDrag(editControls, target);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);
  let itemToDrag = "sync-button";
  let button = document.getElementById(itemToDrag);
  if (!isInWin8()) {
    placementsAfterMove.push(itemToDrag);
  } else {
    placementsAfterMove.splice(11, 0, itemToDrag);
  }
  simulateItemDrag(button, editControls);
  assertAreaPlacements(CustomizableUI.AREA_PANEL, placementsAfterMove);

  
  let palette = document.getElementById("customization-palette");
  let zoomControls = document.getElementById("zoom-controls");
  simulateItemDrag(button, palette);
  simulateItemDrag(editControls, zoomControls);
  ok(CustomizableUI.inDefaultState, "Should be in default state again.");
});

add_task(function asyncCleanup() {
  yield endCustomizing();
  Services.prefs.clearUserPref("browser.uiCustomization.skipSourceNodeCheck");
  yield resetCustomization();
});
