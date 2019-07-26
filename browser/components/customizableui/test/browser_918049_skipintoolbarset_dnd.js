



"use strict";

let navbar;
let skippedItem;

Services.prefs.setBoolPref("browser.uiCustomization.skipSourceNodeCheck", true);


add_task(function() {
  navbar = document.getElementById("nav-bar");
  skippedItem = document.createElement("toolbarbutton");
  skippedItem.id = "test-skipintoolbarset-item";
  skippedItem.setAttribute("label", "Test");
  skippedItem.setAttribute("skipintoolbarset", "true");
  navbar.customizationTarget.appendChild(skippedItem);
  let downloadsButton = document.getElementById("downloads-button");
  yield startCustomizing();
  ok(CustomizableUI.inDefaultState, "Should still be in default state");
  simulateItemDrag(skippedItem, downloadsButton);
  ok(CustomizableUI.inDefaultState, "Should still be in default state");
  let skippedItemWrapper = skippedItem.parentNode;
  is(skippedItemWrapper.nextSibling && skippedItemWrapper.nextSibling.id,
     downloadsButton.parentNode.id, "Should be next to downloads button");
  simulateItemDrag(downloadsButton, skippedItem);
  let downloadWrapper = downloadsButton.parentNode;
  is(downloadWrapper.nextSibling && downloadWrapper.nextSibling.id,
     skippedItem.parentNode.id, "Should be next to skipintoolbarset item");
  ok(CustomizableUI.inDefaultState, "Should still be in default state");
});

add_task(function asyncCleanup() {
  yield endCustomizing();
  skippedItem.remove();
  Services.prefs.clearUserPref("browser.uiCustomization.skipSourceNodeCheck");
  yield resetCustomization();
});
