



"use strict";




add_task(function() {
  yield startCustomizing();
  CustomizableUI.addWidgetToArea("characterencoding-button", "PanelUI-contents");
  yield endCustomizing();
  yield PanelUI.show();
  let panelHiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHiddenPromise;
  CustomizableUI.addWidgetToArea("characterencoding-button", 'nav-bar');
  let button = document.getElementById("characterencoding-button");
  ok(!button.hasAttribute("disabled"), "Button shouldn't be disabled");
});

add_task(function asyncCleanup() {
  resetCustomization();
});

