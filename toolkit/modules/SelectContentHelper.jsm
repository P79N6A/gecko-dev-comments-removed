



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
        if (this.element.selectedIndex != message.data.value) {
          this.element.selectedIndex = message.data.value;

          let event = this.element.ownerDocument.createEvent("Events");
          event.initEvent("change", true, true);
          this.element.dispatchEvent(event);
        }

        
      case "Forms:DismissedDropDown":
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

function buildOptionListForChildren(node) {
  let result = [];
  for (let child = node.firstChild; child; child = child.nextSibling) {
    if (child.tagName == 'OPTION' || child.tagName == 'OPTGROUP') {
      let info = {
        tagName: child.tagName,
        textContent: child.tagName == 'OPTGROUP' ? child.getAttribute("label").trim()
                                                 : child.textContent.trim(),
        
        
        
        
        
        
        
        children: child.tagName == 'OPTGROUP' ? buildOptionListForChildren(child) : []
      };
      result.push(info);
    }
  }
  return result;
}
