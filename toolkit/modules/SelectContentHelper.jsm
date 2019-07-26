"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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
    let { top, left, width, height } = this.element.getBoundingClientRect();
    
    
    let rect = { left: left, top: top, width: width, height: height };

    
    let view = this.element.ownerDocument.defaultView;
    for (let frame = view; frame != this.global.content; frame = frame.parent) {
      
      let r = frame.frameElement.getBoundingClientRect();
      let left = frame.getComputedStyle(frame.frameElement, "").borderLeftWidth;
      let top = frame.getComputedStyle(frame.frameElement, "").borderTopWidth;

      rect.left += r.left + parseInt(left, 10);
      rect.top += r.top + parseInt(top, 10);
    }

    return rect;
  },

  _buildOptionList: function() {
    return buildOptionListForChildren(this.element);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Forms:SelectDropDownItem":
        this.element.selectedIndex = message.data.value;

        
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
        textContent: child.tagName == 'OPTGROUP' ? child.getAttribute("label")
                                                 : child.textContent,
        
        
        
        
        
        
        
        children: child.tagName == 'OPTGROUP' ? buildOptionListForChildren(child) : []
      };
      result.push(info);
    }
  }
  return result;
}
