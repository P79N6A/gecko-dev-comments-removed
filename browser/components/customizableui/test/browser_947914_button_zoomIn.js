



"use strict";

let initialPageZoom = ZoomManager.zoom;

add_task(function() {
  info("Check zoom in button existence and functionality");

  is(initialPageZoom, 1, "Initial zoom factor should be 1");

  yield PanelUI.show();
  info("Menu panel was opened");

  let zoomInButton = document.getElementById("zoom-in-button");
  ok(zoomInButton, "Zoom in button exists in Panel Menu");

  zoomInButton.click();
  let pageZoomLevel = parseInt(ZoomManager.zoom * 100);
  let zoomResetButton = document.getElementById("zoom-reset-button");
  let expectedZoomLevel = parseInt(zoomResetButton.getAttribute("label"), 10);
  ok(pageZoomLevel > 100 && pageZoomLevel == expectedZoomLevel, "Page zoomed in correctly");

  
  let panelHiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHiddenPromise;
  info("Menu panel was closed");
});

add_task(function asyncCleanup() {
  
  ZoomManager.zoom = initialPageZoom;
  info("Zoom level was restored");
});
