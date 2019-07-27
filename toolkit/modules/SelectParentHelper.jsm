



"use strict";

this.EXPORTED_SYMBOLS = [
  "SelectParentHelper"
];

let currentBrowser = null;

this.SelectParentHelper = {
  populate: function(menulist, items, selectedIndex, zoom) {
    
    menulist.menupopup.textContent = "";
    populateChildren(menulist, items, selectedIndex, zoom);
  },

  open: function(browser, menulist, rect) {
    menulist.hidden = false;
    currentBrowser = browser;
    this._registerListeners(menulist.menupopup);

    menulist.menupopup.openPopupAtScreenRect("after_start", rect.left, rect.top, rect.width, rect.height, false, false);
    menulist.selectedItem.scrollIntoView();
  },

  hide: function(menulist) {
    menulist.menupopup.hidePopup();
  },

  handleEvent: function(event) {
    switch (event.type) {
      case "command":
        if (event.target.hasAttribute("value")) {
          currentBrowser.messageManager.sendAsyncMessage("Forms:SelectDropDownItem", {
            value: event.target.value
          });
        }
        break;

      case "popuphidden":
        currentBrowser.messageManager.sendAsyncMessage("Forms:DismissedDropDown", {});
        currentBrowser = null;
        let popup = event.target;
        this._unregisterListeners(popup);
        popup.parentNode.hidden = true;
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

function populateChildren(menulist, options, selectedIndex, zoom, startIndex = 0,
                          isInGroup = false, isGroupDisabled = false, adjustedTextSize = -1) {
  let index = startIndex;
  let element = menulist.menupopup;

  
  
  if (adjustedTextSize == -1) {
    let win = element.ownerDocument.defaultView;

    
    
    
    let textSize = win.getComputedStyle(element).getPropertyValue("font-size");
    adjustedTextSize = (zoom * parseFloat(textSize, 10)) + "px";
  }

  for (let option of options) {
    let isOptGroup = (option.tagName == 'OPTGROUP');
    let item = element.ownerDocument.createElement(isOptGroup ? "menucaption" : "menuitem");

    item.setAttribute("label", option.textContent);
    item.style.direction = option.textDirection;
    item.style.fontSize = adjustedTextSize;
    item.setAttribute("tooltiptext", option.tooltip);

    element.appendChild(item);

    
    let isDisabled = isGroupDisabled || option.disabled;
    if (isDisabled) {
      item.setAttribute("disabled", "true");
    }

    if (isOptGroup) {
      index = populateChildren(menulist, option.children, selectedIndex, zoom,
                               index, true, isDisabled, adjustedTextSize);
    } else {
      if (index == selectedIndex) {
        
        
        
        
        
        menulist.selectedItem = item;
      }

      item.setAttribute("value", index++);

      if (isInGroup) {
        item.classList.add("contentSelectDropdown-ingroup")
      }
    }
  }

  return index;
}
