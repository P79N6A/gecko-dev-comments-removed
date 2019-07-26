



"use strict";



add_task(function() {
  const buttonId = "bookmarks-toolbar-placeholder";
  yield startCustomizing();
  CustomizableUI.addWidgetToArea("personal-bookmarks", CustomizableUI.AREA_PANEL);
  yield endCustomizing();

  yield PanelUI.show();

  let bookmarksToolbarPlaceholder = document.getElementById(buttonId);
  ok(bookmarksToolbarPlaceholder.classList.contains("toolbarbutton-1"),
     "Button should have toolbarbutton-1 class");
  is(bookmarksToolbarPlaceholder.getAttribute("wrap"), "true",
     "Button should have the 'wrap' attribute");

  info("Waiting for panel to close");
  let panelHiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield panelHiddenPromise;

  info("Waiting for window to open");
  let newWin = yield openAndLoadWindow({}, true);

  info("Waiting for panel in new window to open");
  yield newWin.PanelUI.show();
  let newWinBookmarksToolbarPlaceholder = newWin.document.getElementById(buttonId);
  ok(newWinBookmarksToolbarPlaceholder.classList.contains("toolbarbutton-1"),
     "Button in new window should have toolbarbutton-1 class");
  is(newWinBookmarksToolbarPlaceholder.getAttribute("wrap"), "true",
     "Button in new window should have 'wrap' attribute");

  info("Waiting for panel in new window to close");
  panelHiddenPromise = promisePanelHidden(newWin);
  newWin.PanelUI.hide();
  yield panelHiddenPromise;

  info("Waiting for new window to close");
  yield promiseWindowClosed(newWin);
});

add_task(function asyncCleanUp() {
  yield endCustomizing();
  CustomizableUI.reset();
});

