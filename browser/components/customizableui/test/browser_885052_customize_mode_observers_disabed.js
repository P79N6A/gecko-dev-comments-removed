



"use strict";


add_task(function() {
  
  
  let shownPanelPromise = promisePanelShown(window);
  PanelUI.toggle({type: "command"});
  yield shownPanelPromise;
  let hiddenPanelPromise = promisePanelHidden(window);
  PanelUI.toggle({type: "command"});
  yield hiddenPanelPromise;

  let fullscreenButton = document.getElementById("fullscreen-button");
  ok(!fullscreenButton.checked, "Fullscreen button should not be checked when not in fullscreen.")

  BrowserFullScreen();
  yield waitForCondition(function() fullscreenButton.checked);
  ok(fullscreenButton.checked, "Fullscreen button should be checked when in fullscreen.")

  yield startCustomizing();

  let fullscreenButtonWrapper = document.getElementById("wrapper-fullscreen-button");
  ok(fullscreenButtonWrapper.hasAttribute("itemobserves"), "Observer should be moved to wrapper");
  fullscreenButton = document.getElementById("fullscreen-button");
  ok(!fullscreenButton.hasAttribute("observes"), "Observer should be removed from button");
  ok(!fullscreenButton.checked, "Fullscreen button should no longer be checked during customization mode");

  yield endCustomizing();

  BrowserFullScreen();
  fullscreenButton = document.getElementById("fullscreen-button");
  yield waitForCondition(function() !fullscreenButton.checked);
  ok(!fullscreenButton.checked, "Fullscreen button should not be checked when not in fullscreen.")
});
