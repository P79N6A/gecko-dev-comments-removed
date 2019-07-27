



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
                                  "resource://gre/modules/BrowserUtils.jsm");

this.EXPORTED_SYMBOLS = [
  "SelectContentHelper"
];

this.SelectContentHelper = function (aElement, aGlobal) {
  this.element = aElement;
  this.initialSelection = aElement[aElement.selectedIndex] || null;
  this.global = aGlobal;
  this.init();
  this.showDropDown();
}

this.SelectContentHelper.prototype = {
  init: function() {
    this.global.addMessageListener("Forms:SelectDropDownItem", this);
    this.global.addMessageListener("Forms:DismissedDropDown", this);
    this.global.addEventListener("pagehide", this);
  },

  uninit: function() {
    this.global.removeMessageListener("Forms:SelectDropDownItem", this);
    this.global.removeMessageListener("Forms:DismissedDropDown", this);
    this.global.removeEventListener("pagehide", this);
    this.element = null;
    this.global = null;
  },

  showDropDown: function() {
    let rect = this._getBoundingContentRect();

    this.global.sendAsyncMessage("Forms:ShowDropDown", {
      rect: rect,
      options: this._buildOptionList(),
      selectedIndex: this.element.selectedIndex,
      direction: getComputedDirection(this.element)
    });
  },

  _getBoundingContentRect: function() {
    return BrowserUtils.getElementBoundingScreenRect(this.element);
  },

  _buildOptionList: function() {
    return buildOptionListForChildren(this.element);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Forms:SelectDropDownItem":
        this.element.selectedIndex = message.data.value;
        break;

      case "Forms:DismissedDropDown":
        if (this.initialSelection != this.element.item[this.element.selectedIndex]) {
          let event = this.element.ownerDocument.createEvent("Events");
          event.initEvent("change", true, true);
          this.element.dispatchEvent(event);
        }

        this.uninit();
        break;
    }
  },

  handleEvent: function(event) {
    switch (event.type) {
      case "pagehide":
        this.global.sendAsyncMessage("Forms:HideDropDown", {});
        this.uninit();
        break;
    }
  }

}

function getComputedDirection(element) {
  return element.ownerDocument.defaultView.getComputedStyle(element).getPropertyValue("direction");
}

function buildOptionListForChildren(node) {
  let result = [];

  for (let child of node.children) {
    let tagName = child.tagName.toUpperCase();

    if (tagName == 'OPTION' || tagName == 'OPTGROUP') {
      let textContent =
        tagName == 'OPTGROUP' ? child.getAttribute("label")
                              : child.textContent;

      if (textContent != null) {
        textContent = textContent.trim();
      } else {
        textContent = ""
      }

      let info = {
        tagName: child.tagName,
        textContent: textContent,
        disabled: child.disabled,
        
        
        textDirection: getComputedDirection(child),
        tooltip: child.title,
        
        
        
        
        
        
        
        children: tagName == 'OPTGROUP' ? buildOptionListForChildren(child) : []
      };
      result.push(info);
    }
  }
  return result;
}
