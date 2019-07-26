



"use strict";

add_task(function() {
  info("Check fullscreen button existence and functionality");
  yield PanelUI.show();

  let fullscreenButton = document.getElementById("fullscreen-button");
  ok(fullscreenButton, "Fullscreen button appears in Panel Menu");
  fullscreenButton.click();

  yield waitForCondition(function() window.fullScreen);
  ok(window.fullScreen, "Fullscreen mode was opened");

  
  window.fullScreen = !window.fullScreen;
  yield waitForCondition(function() !window.fullScreen);
});
