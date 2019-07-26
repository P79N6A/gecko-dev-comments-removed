"use strict";

let overflowPanel = document.getElementById("widget-overflow");

const isOSX = (Services.appinfo.OS === "Darwin");

let originalWindowWidth;
registerCleanupFunction(function() {
  window.resizeTo(originalWindowWidth, window.outerHeight);
});



add_task(function() {
  originalWindowWidth = window.outerWidth;
  let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
  ok(!navbar.hasAttribute("overflowing"), "Should start with a non-overflowing toolbar.");
  let oldChildCount = navbar.customizationTarget.childElementCount;
  window.resizeTo(400, window.outerHeight);

  yield waitForCondition(() => navbar.hasAttribute("overflowing"));
  ok(navbar.hasAttribute("overflowing"), "Should have an overflowing toolbar.");

  let chevron = document.getElementById("nav-bar-overflow-button");
  let shownPanelPromise = promisePanelElementShown(window, overflowPanel);
  chevron.click();
  yield shownPanelPromise;

  let contextMenu = document.getElementById("toolbar-context-menu");
  let shownContextPromise = contextMenuShown(contextMenu);
  let homeButton = document.getElementById("home-button");
  ok(homeButton, "home-button was found");
  ok(homeButton.classList.contains("overflowedItem"), "Home button is overflowing");
  EventUtils.synthesizeMouse(homeButton, 2, 2, {type: "contextmenu", button: 2});
  yield shownContextPromise;

  is(overflowPanel.state, "open", "The widget overflow panel should still be open.");

  let expectedEntries = [
    [".customize-context-moveToPanel", true],
    [".customize-context-removeFromToolbar", true],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  );
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenContextPromise = contextMenuHidden(contextMenu);
  let hiddenPromise = promisePanelElementHidden(window, overflowPanel);
  let moveToPanel = contextMenu.querySelector(".customize-context-moveToPanel");
  if (moveToPanel) {
    moveToPanel.click();
  }
  contextMenu.hidePopup();
  yield hiddenContextPromise;
  yield hiddenPromise;

  let homeButtonPlacement = CustomizableUI.getPlacementOfWidget("home-button");
  ok(homeButtonPlacement, "Home button should still have a placement");
  is(homeButtonPlacement && homeButtonPlacement.area, "PanelUI-contents", "Home button should be in the panel now");
  CustomizableUI.reset();

  
  yield waitForCondition(() => navbar.hasAttribute("overflowing"));
  ok(navbar.hasAttribute("overflowing"), "Should have an overflowing toolbar.");

  let homeButtonPlacement = CustomizableUI.getPlacementOfWidget("home-button");
  ok(homeButtonPlacement, "Home button should still have a placement");
  is(homeButtonPlacement && homeButtonPlacement.area, "nav-bar", "Home button should be back in the navbar now");

  ok(homeButton.classList.contains("overflowedItem"), "Home button should still be overflowed");
});
