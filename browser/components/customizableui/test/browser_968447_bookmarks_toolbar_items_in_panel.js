



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
  let hideTrace = function() {
    info(new Error().stack);
    info("Panel was hidden.");
  };
  newWin.PanelUI.panel.addEventListener("popuphidden", hideTrace);

  yield newWin.PanelUI.show();
  let newWinBookmarksToolbarPlaceholder = newWin.document.getElementById(buttonId);
  ok(newWinBookmarksToolbarPlaceholder.classList.contains("toolbarbutton-1"),
     "Button in new window should have toolbarbutton-1 class");
  is(newWinBookmarksToolbarPlaceholder.getAttribute("wrap"), "true",
     "Button in new window should have 'wrap' attribute");

  newWin.PanelUI.panel.removeEventListener("popuphidden", hideTrace);
  
  
  if (newWin.PanelUI.panel.state != "closed") {
    info("Panel is still open in new window, waiting for it to close");
    panelHiddenPromise = promisePanelHidden(newWin);
    newWin.PanelUI.hide();
    yield panelHiddenPromise;
  } else {
    info("panel was already closed");
  }

  info("Waiting for new window to close");
  yield promiseWindowClosed(newWin);
});

add_task(function asyncCleanUp() {
  yield endCustomizing();
  CustomizableUI.reset();
});

