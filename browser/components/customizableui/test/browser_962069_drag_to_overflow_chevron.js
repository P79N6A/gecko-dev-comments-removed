



"use strict";

let originalWindowWidth;


add_task(function*() {
  originalWindowWidth = window.outerWidth;
  let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
  ok(!navbar.hasAttribute("overflowing"), "Should start with a non-overflowing toolbar.");
  ok(CustomizableUI.inDefaultState, "Should start in default state.");
  let oldChildCount = navbar.customizationTarget.childElementCount;
  window.resizeTo(400, window.outerHeight);
  yield waitForCondition(() => navbar.hasAttribute("overflowing"));
  ok(navbar.hasAttribute("overflowing"), "Should have an overflowing toolbar.");

  let widgetOverflowPanel = document.getElementById("widget-overflow");
  let panelShownPromise = promisePanelElementShown(window, widgetOverflowPanel);
  let identityBox = document.getElementById("identity-box");
  let overflowChevron = document.getElementById("nav-bar-overflow-button");
  ChromeUtils.synthesizeDrop(identityBox, overflowChevron, [], null);
  yield panelShownPromise;

  info("Overflow panel is shown.");

  let panelHiddenPromise = promisePanelElementHidden(window, widgetOverflowPanel);
  widgetOverflowPanel.hidePopup();
  yield panelHiddenPromise;
});

add_task(function*() {
  window.resizeTo(originalWindowWidth, window.outerHeight);
  let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
  yield waitForCondition(() => !navbar.hasAttribute("overflowing"));
  ok(!navbar.hasAttribute("overflowing"), "Should not have an overflowing toolbar.");
});
