




"use strict";

const snappedSize = 330;
const portraitSize = 900;

function setSnappedViewstate() {
  ok(isLandscapeMode(), "setSnappedViewstate expects landscape mode to work.");

  let browser = Browser.selectedBrowser;

  
  let fullWidth = browser.clientWidth;
  let padding = fullWidth - snappedSize;

  browser.style.borderRight = padding + "px solid gray";

  
  Services.obs.notifyObservers(null, 'metro_viewstate_changed', 'snapped');

  
  yield waitForMs(0);
}

function setPortraitViewstate() {
  ok(isLandscapeMode(), "setPortraitViewstate expects landscape mode to work.");

  let browser = Browser.selectedBrowser;

  let fullWidth = browser.clientWidth;
  let padding = fullWidth - portraitSize;

  browser.style.borderRight = padding + "px solid gray";

  Services.obs.notifyObservers(null, 'metro_viewstate_changed', 'portrait');

  
  yield waitForMs(0);
}

function restoreViewstate() {
  ok(isLandscapeMode(), "restoreViewstate expects landscape mode to work.");

  Services.obs.notifyObservers(null, 'metro_viewstate_changed', 'landscape');

  Browser.selectedBrowser.style.removeProperty("border-right");

  yield waitForMs(0);
}
