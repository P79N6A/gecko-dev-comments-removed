



"use strict";

add_task(function() {
  info("Check zoom out button existence and functionality");

  let initialPageZoom = ZoomManager.zoom;
  is(initialPageZoom, 1, "Initial zoom factor should be 1");

  yield PanelUI.show();

  let zoomOutButton = document.getElementById("zoom-out-button");
  ok(zoomOutButton, "Zoom out button exists in Panel Menu");

  zoomOutButton.click();
  let pageZoomLevel = Math.round(ZoomManager.zoom*100);

  let zoomResetButton = document.getElementById("zoom-reset-button");
  let expectedZoomLevel = parseInt(zoomResetButton.getAttribute("label"), 10);
  ok(pageZoomLevel < 100 && pageZoomLevel == expectedZoomLevel, "Page zoomed out correctly");

  
  let panelHiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHiddenPromise;

  
  ZoomManager.zoom = initialPageZoom;
});
