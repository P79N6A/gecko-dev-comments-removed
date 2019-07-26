



"use strict";

let tempElements = [];

function insertTempItemsIntoMenu(parentMenu) {
  
  let beforeEls = [parentMenu.firstChild, parentMenu.lastChild, null];
  for (let i = 0; i < beforeEls.length; i++) {
    let sep = document.createElement("menuseparator");
    tempElements.push(sep);
    parentMenu.insertBefore(sep, beforeEls[i]);
    let menu = document.createElement("menu");
    tempElements.push(menu);
    parentMenu.insertBefore(menu, beforeEls[i]);
    
    sep = document.createElement("menuseparator");
    tempElements.push(sep);
    parentMenu.insertBefore(sep, beforeEls[i]);
  }
}

function checkSeparatorInsertion(menuId, buttonId, subviewId) {
  return function() {
    info("Checking for duplicate separators in " + buttonId + " widget");
    let menu = document.getElementById(menuId);
    insertTempItemsIntoMenu(menu);

    let placement = CustomizableUI.getPlacementOfWidget(buttonId);
    let changedPlacement = false;
    if (!placement || placement.area != CustomizableUI.AREA_PANEL) {
      CustomizableUI.addWidgetToArea(buttonId, CustomizableUI.AREA_PANEL);
      changedPlacement = true;
    }
    yield PanelUI.show();

    let button = document.getElementById(buttonId);
    button.click();

    yield waitForCondition(() => !PanelUI.multiView.hasAttribute("transitioning"));
    let subview = document.getElementById(subviewId);
    ok(subview.firstChild, "Subview should have a kid");
    is(subview.firstChild.localName, "toolbarbutton", "There should be no separators to start with");

    for (let kid of subview.children) {
      if (kid.localName == "menuseparator") {
        ok(kid.previousSibling && kid.previousSibling.localName != "menuseparator",
           "Separators should never have another separator next to them, and should never be the first node.");
      }
    }

    let panelHiddenPromise = promisePanelHidden(window);
    PanelUI.hide();
    yield panelHiddenPromise;

    if (changedPlacement) {
      CustomizableUI.reset();
    }
  };
}

add_task(checkSeparatorInsertion("menuWebDeveloperPopup", "developer-button", "PanelUI-developerItems"));
add_task(checkSeparatorInsertion("viewSidebarMenu", "sidebar-button", "PanelUI-sidebarItems"));

registerCleanupFunction(function() {
  for (let el of tempElements) {
    el.remove();
  }
  tempElements = null;
});
