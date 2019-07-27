



"use strict";

this.EXPORTED_SYMBOLS = [
  "SelectParentHelper"
];

let currentBrowser = null;

this.SelectParentHelper = {
  populate: function(menulist, items, selectedIndex) {
    
    menulist.menupopup.textContent = "";
    populateChildren(menulist.menupopup, items, selectedIndex);
    
    
    
    
    
    menulist.selectedIndex = selectedIndex;
  },

  open: function(browser, menulist, rect) {
    menulist.hidden = false;
    currentBrowser = browser;
    this._registerListeners(menulist.menupopup);

    menulist.menupopup.openPopupAtScreen(rect.left, rect.top + rect.height);
    menulist.selectedItem.scrollIntoView();
  },

  hide: function(menulist) {
    menulist.menupopup.hidePopup();
  },

  handleEvent: function(event) {
    let popup = event.currentTarget;
    let menulist = popup.parentNode;

    switch (event.type) {
      case "command":
        if (event.target.hasAttribute("value")) {
          currentBrowser.messageManager.sendAsyncMessage("Forms:SelectDropDownItem", {
            value: event.target.value
          });
        }
        popup.hidePopup();
        break;

      case "popuphidden":
        currentBrowser.messageManager.sendAsyncMessage("Forms:DismissedDropDown", {});
        currentBrowser = null;
        this._unregisterListeners(popup);
        menulist.hidden = true;
        break;
    }
  },

  _registerListeners: function(popup) {
    popup.addEventListener("command", this);
    popup.addEventListener("popuphidden", this);
  },

  _unregisterListeners: function(popup) {
    popup.removeEventListener("command", this);
    popup.removeEventListener("popuphidden", this);
  },

};

function populateChildren(element, options, selectedIndex, startIndex = 0, isGroup = false) {
  let index = startIndex;

  for (let option of options) {
    let item = element.ownerDocument.createElement("menuitem");
    item.setAttribute("label", option.textContent);
    item.style.direction = option.textDirection;

    element.appendChild(item);

    if (option.children.length > 0) {
      item.classList.add("contentSelectDropdown-optgroup");
      item.setAttribute("disabled", "true");
      index = populateChildren(element, option.children, selectedIndex, index, true);
    } else {
      item.setAttribute("value", index++);

      if (isGroup) {
        item.classList.add("contentSelectDropdown-ingroup")
      }
    }
  }

  return index;
}
