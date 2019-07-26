



"use strict";

const kTestBtnId = "test-removable-navbar-customize-mode";


add_task(function() {
  let btn = createDummyXULButton(kTestBtnId, "Test removable in navbar in customize mode");
  document.getElementById("nav-bar").customizationTarget.appendChild(btn);
  yield startCustomizing();
  ok(!CustomizableUI.isWidgetRemovable(kTestBtnId), "Widget should not be considered removable");
  yield endCustomizing();
  document.getElementById(kTestBtnId).remove();
});

add_task(function asyncCleanup() {
  yield endCustomizing();
  yield resetCustomization();
});
