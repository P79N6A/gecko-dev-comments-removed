



"use strict";

const kHidden1Id = "test-hidden-button-1";
const kHidden2Id = "test-hidden-button-2";

let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);




add_task(function() {
  ok(CustomizableUI.inDefaultState, "Should be in the default state");

  
  
  let placements = CustomizableUI.getWidgetsInArea(CustomizableUI.AREA_NAVBAR);
  let lastVisible = null;
  for (let widgetGroup of placements.reverse()) {
    let widget = widgetGroup.forWindow(window);
    if (widget && widget.node && !widget.node.hidden) {
      lastVisible = widget.node;
      break;
    }
  }

  if (!lastVisible) {
    ok(false, "Apparently, there are no visible items in the nav-bar.");
  }

  info("The last visible item in the nav-bar has ID: " + lastVisible.id);

  let hidden1 = createDummyXULButton(kHidden1Id, "You can't see me");
  let hidden2 = createDummyXULButton(kHidden2Id, "You can't see me either.");
  hidden1.hidden = hidden2.hidden = true;

  
  navbar.insertItem(hidden1.id);
  navbar.insertItem(hidden2.id);

  
  
  yield startCustomizing();
  let downloadsButton = document.getElementById("downloads-button");
  simulateItemDrag(downloadsButton, navbar.customizationTarget);

  yield endCustomizing();

  is(downloadsButton.previousSibling.id, lastVisible.id,
     "The downloads button should be placed after the last visible item.");

  yield resetCustomization();
});
