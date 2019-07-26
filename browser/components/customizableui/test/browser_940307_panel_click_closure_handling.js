



"use strict";

let button, menuButton;

add_task(function() {
  button = document.createElement("toolbarbutton");
  button.id = "browser_940307_button";
  button.setAttribute("label", "Button");
  PanelUI.contents.appendChild(button);
  yield PanelUI.show();
  let hiddenAgain = promisePanelHidden(window);
  EventUtils.synthesizeMouseAtCenter(button, {});
  yield hiddenAgain;
  button.remove();
});


add_task(function() {
  menuButton = document.createElement("toolbarbutton");
  menuButton.setAttribute("type", "menu-button");
  menuButton.id = "browser_940307_menubutton";
  menuButton.setAttribute("label", "Menu button");

  let menuPopup = document.createElement("menupopup");
  menuPopup.id = "browser_940307_menupopup";

  let menuItem = document.createElement("menuitem");
  menuItem.setAttribute("label", "Menu item");
  menuItem.id = "browser_940307_menuitem";

  menuPopup.appendChild(menuItem);
  menuButton.appendChild(menuPopup);
  PanelUI.contents.appendChild(menuButton);

  yield PanelUI.show();
  let hiddenAgain = promisePanelHidden(window);
  let innerButton = document.getAnonymousElementByAttribute(menuButton, "anonid", "button");
  EventUtils.synthesizeMouseAtCenter(innerButton, {});
  yield hiddenAgain;

  
  yield PanelUI.show();
  hiddenAgain = promisePanelHidden(window);
  let menuShown = promisePanelElementShown(window, menuPopup);
  let dropmarker = document.getAnonymousElementByAttribute(menuButton, "type", "menu-button");
  EventUtils.synthesizeMouseAtCenter(dropmarker, {});
  yield menuShown;
  
  ok(isPanelUIOpen(), "Panel should still be open");
  let menuHidden = promisePanelElementHidden(window, menuPopup);
  
  EventUtils.synthesizeMouseAtCenter(menuItem, {});
  yield menuHidden;
  yield hiddenAgain;
  menuButton.remove();
});

add_task(function*() {
  let searchbar = document.getElementById("searchbar");
  gCustomizeMode.addToPanel(searchbar);
  let placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_PANEL, "Should be in panel");
  yield PanelUI.show();
  yield waitForCondition(() => "value" in searchbar && searchbar.value === "");

  searchbar.value = "foo";
  searchbar.focus();
  
  let textbox = document.getAnonymousElementByAttribute(searchbar.textbox, "anonid", "textbox-input-box");
  let contextmenu = document.getAnonymousElementByAttribute(textbox, "anonid", "input-box-contextmenu");
  let contextMenuShown = promisePanelElementShown(window, contextmenu);
  EventUtils.synthesizeMouseAtCenter(searchbar, {type: "contextmenu", button: 2});
  yield contextMenuShown;

  ok(isPanelUIOpen(), "Panel should still be open");

  let selectAll = contextmenu.querySelector("[cmd='cmd_selectAll']");
  let contextMenuHidden = promisePanelElementHidden(window, contextmenu);
  EventUtils.synthesizeMouseAtCenter(selectAll, {});
  yield contextMenuHidden;

  ok(isPanelUIOpen(), "Panel should still be open");

  let hiddenPanelPromise = promisePanelHidden(window);
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield hiddenPanelPromise;
  ok(!isPanelUIOpen(), "Panel should no longer be open");
});

add_task(function*() {
  button = document.createElement("toolbarbutton");
  button.id = "browser_946166_button_disabled";
  button.setAttribute("disabled", "true");
  button.setAttribute("label", "Button");
  PanelUI.contents.appendChild(button);
  yield PanelUI.show();
  EventUtils.synthesizeMouseAtCenter(button, {});
  is(PanelUI.panel.state, "open", "Popup stays open");
  button.removeAttribute("disabled");
  let hiddenAgain = promisePanelHidden(window);
  EventUtils.synthesizeMouseAtCenter(button, {});
  yield hiddenAgain;
  button.remove();
});

registerCleanupFunction(function() {
  if (button && button.parentNode) {
    button.remove();
  }
  if (menuButton && menuButton.parentNode) {
    menuButton.remove();
  }
  
  
  
  if (isPanelUIOpen()) {
    PanelUI.hide();
  }
});

