




"use strict";

const snappedSize = 330;
const portraitSize = 900;
const maxPortraitHeight = 900;

function setSnappedViewstate() {
  ok(isLandscapeMode(), "setSnappedViewstate expects landscape mode to work.");

  let browser = Browser.selectedBrowser;

  
  let fullWidth = browser.clientWidth;
  let padding = fullWidth - snappedSize;

  browser.style.borderRight = padding + "px solid gray";

  
  ContentAreaObserver._updateViewState("snapped");
  ContentAreaObserver._dispatchBrowserEvent("SizeChanged");
  yield waitForMessage("Content:SetWindowSize:Complete", browser.messageManager);

  
  yield waitForMs(0);
}

function setPortraitViewstate() {
  ok(isLandscapeMode(), "setPortraitViewstate expects landscape mode to work.");

  let browser = Browser.selectedBrowser;

  let fullWidth = browser.clientWidth;
  let fullHeight = browser.clientHeight;
  let padding = fullWidth - portraitSize;

  browser.style.borderRight = padding + "px solid gray";

  
  if (fullHeight > maxPortraitHeight)
    browser.style.borderBottom = (fullHeight - maxPortraitHeight) + "px solid gray";

  ContentAreaObserver._updateViewState("portrait");
  ContentAreaObserver._dispatchBrowserEvent("SizeChanged");
  yield waitForMessage("Content:SetWindowSize:Complete", browser.messageManager);

  
  yield waitForMs(0);
}

function restoreViewstate() {
  ContentAreaObserver._updateViewState("landscape");
  ContentAreaObserver._dispatchBrowserEvent("SizeChanged");
  yield waitForMessage("Content:SetWindowSize:Complete", Browser.selectedBrowser.messageManager);

  ok(isLandscapeMode(), "restoreViewstate should restore landscape mode.");

  Browser.selectedBrowser.style.removeProperty("border-right");
  Browser.selectedBrowser.style.removeProperty("border-bottom");

  yield waitForMs(0);
}
