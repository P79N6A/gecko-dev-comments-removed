


"use strict";

const kAnchorAttribute = "cui-anchorid";





add_task(function*() {
  yield startCustomizing();
  let button = document.getElementById("history-panelmenu");
  is(button.getAttribute(kAnchorAttribute), "PanelUI-menu-button",
     "Button (" + button.id + ") starts out with correct anchor");

  let navbar = document.getElementById("nav-bar").customizationTarget;
  simulateItemDrag(button, navbar);
  is(CustomizableUI.getPlacementOfWidget(button.id).area, "nav-bar",
     "Button (" + button.id + ") ends up in nav-bar");

  ok(!button.hasAttribute(kAnchorAttribute),
     "Button (" + button.id + ") has no anchor in toolbar");

  let resetButton = document.getElementById("customization-reset-button");
  ok(!resetButton.hasAttribute("disabled"), "Should be able to reset now.");
  yield gCustomizeMode.reset();

  is(button.getAttribute(kAnchorAttribute), "PanelUI-menu-button",
     "Button (" + button.id + ") has anchor again");

  let undoButton = document.getElementById("customization-undo-reset-button");
  ok(!undoButton.hasAttribute("disabled"), "Should be able to undo now.");
  yield gCustomizeMode.undoReset();

  ok(!button.hasAttribute(kAnchorAttribute),
     "Button (" + button.id + ") once again has no anchor in toolbar");

  yield gCustomizeMode.reset();

  yield endCustomizing();
});






add_task(function*() {
  yield startCustomizing();
  let button = document.getElementById("bookmarks-menu-button");
  ok(!button.hasAttribute(kAnchorAttribute),
     "Button (" + button.id + ") has no anchor in toolbar");

  let panel = document.getElementById("PanelUI-contents");
  simulateItemDrag(button, panel);
  is(CustomizableUI.getPlacementOfWidget(button.id).area, "PanelUI-contents",
     "Button (" + button.id + ") ends up in panel");
  is(button.getAttribute(kAnchorAttribute), "PanelUI-menu-button",
     "Button (" + button.id + ") has correct anchor in the panel");

  let resetButton = document.getElementById("customization-reset-button");
  ok(!resetButton.hasAttribute("disabled"), "Should be able to reset now.");
  yield gCustomizeMode.reset();

  ok(!button.hasAttribute(kAnchorAttribute),
     "Button (" + button.id + ") once again has no anchor in toolbar");

  yield endCustomizing();
});
