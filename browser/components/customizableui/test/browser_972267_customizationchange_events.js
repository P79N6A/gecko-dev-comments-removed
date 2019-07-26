



"use strict";



add_task(function() {
  let newWindow = yield openAndLoadWindow();
  let otherToolbox = newWindow.gNavToolbox;

  let handlerCalledCount = 0;
  let handler = (ev) => {
    handlerCalledCount++;
  };

  let homeButton = document.getElementById("home-button");

  gNavToolbox.addEventListener("customizationchange", handler);
  otherToolbox.addEventListener("customizationchange", handler);

  gCustomizeMode.addToPanel(homeButton);

  is(handlerCalledCount, 2, "Should be called for both windows.");

  
  
  if (homeButton.parentNode.id == "BrowserToolbarPalette") {
    yield PanelUI.ensureReady();
    isnot(homeButton.parentNode.id, "BrowserToolbarPalette", "Home button should now be in panel");
  }

  handlerCalledCount = 0;
  gCustomizeMode.addToToolbar(homeButton);
  is(handlerCalledCount, 2, "Should be called for both windows.");

  gNavToolbox.removeEventListener("customizationchange", handler);
  otherToolbox.removeEventListener("customizationchange", handler);

  yield promiseWindowClosed(newWindow);
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});

