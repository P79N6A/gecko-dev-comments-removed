




"use strict";

const snappedSize = 330;
const portraitSize = 900;

function setSnappedViewstate() {
  ok(isLandscapeMode(), "setSnappedViewstate expects landscape mode to work.");

  let browser = Browser.selectedBrowser;

  
  let fullWidth = browser.clientWidth;
  let padding = fullWidth - snappedSize;

  browser.style.borderRight = padding + "px solid gray";

  
  ContentAreaObserver._updateViewState("snapped");

  
  yield waitForMs(0);
}

function setPortraitViewstate() {
  ok(isLandscapeMode(), "setPortraitViewstate expects landscape mode to work.");

  let browser = Browser.selectedBrowser;

  let fullWidth = browser.clientWidth;
  let padding = fullWidth - portraitSize;

  browser.style.borderRight = padding + "px solid gray";

  ContentAreaObserver._updateViewState("portrait");

  
  yield waitForMs(0);
}

function restoreViewstate() {
  ContentAreaObserver._updateViewState("landscape");
  ok(isLandscapeMode(), "restoreViewstate should restore landscape mode.");

  Browser.selectedBrowser.style.removeProperty("border-right");

  yield waitForMs(0);
}
