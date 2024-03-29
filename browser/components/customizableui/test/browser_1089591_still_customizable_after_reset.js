"use strict";


add_task(function* () {
  yield startCustomizing();
  let historyButton = document.getElementById("wrapper-history-panelmenu");
  let devButton = document.getElementById("wrapper-developer-button");

  ok(historyButton && devButton, "Draggable elements should exist");
  simulateItemDrag(historyButton, devButton);
  gCustomizeMode.reset();
  yield waitForCondition(() => !gCustomizeMode.resetting);
  ok(CustomizableUI.inDefaultState, "Should be back in default state");

  historyButton = document.getElementById("wrapper-history-panelmenu");
  devButton = document.getElementById("wrapper-developer-button");
  ok(historyButton && devButton, "Draggable elements should exist");
  simulateItemDrag(historyButton, devButton);

  yield endCustomizing();
});

add_task(function* asyncCleanup() {
  yield resetCustomization();
});
