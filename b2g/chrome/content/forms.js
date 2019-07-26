





"use strict";

dump("###################################### forms.js loaded\n");

let Ci = Components.interfaces;
let Cc = Components.classes;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyServiceGetter(Services, "fm",
                                   "@mozilla.org/focus-manager;1",
                                   "nsIFocusManager");

XPCOMUtils.defineLazyGetter(this, "domWindowUtils", function () {
  return content.QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIDOMWindowUtils);
});

const RESIZE_SCROLL_DELAY = 20;

let HTMLDocument = Ci.nsIDOMHTMLDocument;
let HTMLHtmlElement = Ci.nsIDOMHTMLHtmlElement;
let HTMLBodyElement = Ci.nsIDOMHTMLBodyElement;
let HTMLIFrameElement = Ci.nsIDOMHTMLIFrameElement;
let HTMLInputElement = Ci.nsIDOMHTMLInputElement;
let HTMLTextAreaElement = Ci.nsIDOMHTMLTextAreaElement;
let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLOptGroupElement = Ci.nsIDOMHTMLOptGroupElement;
let HTMLOptionElement = Ci.nsIDOMHTMLOptionElement;

let FormVisibility = {
  





  findScrolled: function fv_findScrolled(node) {
    let win = node.ownerDocument.defaultView;

    while (!(node instanceof HTMLBodyElement)) {

      
      
      
      if (node.scrollTop !== 0) {
        
        
        
        
        
        return node;
      } else {
        
        
        
        node = node.parentNode;
        continue;
      }
    }

    
    
    
    if (win.scrollMaxX || win.scrollMaxY) {
      return win;
    }

    return null;
  },

  







  yAxisVisible: function fv_yAxisVisible(top, height, maxHeight) {
    return (top > 0 && (top + height) < maxHeight);
  },

  






  scrollablesVisible: function fv_scrollablesVisible(element, pos) {
    while ((element = this.findScrolled(element))) {
      if (element.window && element.self === element)
        break;

      
      
      
      let offset = element.getBoundingClientRect();

      
      
      
      
      let adjustedTop = pos.top - offset.top;

      let visible = this.yAxisVisible(
        adjustedTop,
        pos.height,
        pos.width
      );

      if (!visible)
        return false;

      element = element.parentNode;
    }

    return true;
  },

  






  isVisible: function fv_isVisible(element) {
    
    let rect = element.getBoundingClientRect();
    let parent = element.ownerDocument.defaultView;

    
    
    
    
    
    let pos = {
      top: rect.top,
      height: rect.height,
      width: rect.width
    };

    let visible = true;

    do {
      let frame = parent.frameElement;
      visible = visible &&
                this.yAxisVisible(pos.top, pos.height, parent.innerHeight) &&
                this.scrollablesVisible(element, pos);

      
      
      
      
      if (!visible)
        return false;

      if (frame) {
        let frameRect = frame.getBoundingClientRect();

        pos.top += frameRect.top + frame.clientTop;
      }
    } while (
      (parent !== parent.parent) &&
      (parent = parent.parent)
    );

    return visible;
  }
};

let FormAssistant = {
  init: function fa_init() {
    addEventListener("focus", this, true, false);
    addEventListener("blur", this, true, false);
    addEventListener("resize", this, true, false);
    addEventListener("submit", this, true, false);
    addEventListener("pagehide", this, true, false);
    addMessageListener("Forms:Select:Choice", this);
    addMessageListener("Forms:Input:Value", this);
    addMessageListener("Forms:Select:Blur", this);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  ignoredInputTypes: new Set([
    'button', 'file', 'checkbox', 'radio', 'reset', 'submit', 'image'
  ]),

  isKeyboardOpened: false,
  selectionStart: 0,
  selectionEnd: 0,
  scrollIntoViewTimeout: null,
  _focusedElement: null,

  get focusedElement() {
    if (this._focusedElement && Cu.isDeadWrapper(this._focusedElement))
      this._focusedElement = null;

    return this._focusedElement;
  },

  set focusedElement(val) {
    this._focusedElement = val;
  },

  setFocusedElement: function fa_setFocusedElement(element) {
    if (element === this.focusedElement)
      return;

    if (this.focusedElement) {
      this.focusedElement.removeEventListener('mousedown', this);
      this.focusedElement.removeEventListener('mouseup', this);
      if (!element) {
        this.focusedElement.blur();
      }
    }

    if (element) {
      element.addEventListener('mousedown', this);
      element.addEventListener('mouseup', this);
    }

    this.focusedElement = element;
  },

  handleEvent: function fa_handleEvent(evt) {
    let focusedElement = this.focusedElement;
    let target = evt.target;

    switch (evt.type) {
      case "focus":
        if (target && isContentEditable(target)) {
          this.showKeyboard(this.getTopLevelEditable(target));
          break;
        }

        if (target && this.isFocusableElement(target))
          this.showKeyboard(target);
        break;

      case "blur":
      case "submit":
      case "pagehide":
        if (this.focusedElement)
          this.hideKeyboard();
        break;

      case 'mousedown':
        
        
        this.selectionStart = this.focusedElement.selectionStart;
        this.selectionEnd = this.focusedElement.selectionEnd;
        break;

      case 'mouseup':
        
        
        
        
        if (this.focusedElement.selectionStart !== this.selectionStart ||
            this.focusedElement.selectionEnd !== this.selectionEnd) {
          this.sendKeyboardState(this.focusedElement);
        }
        break;

      case "resize":
        if (!this.isKeyboardOpened)
          return;

        if (this.scrollIntoViewTimeout) {
          content.clearTimeout(this.scrollIntoViewTimeout);
          this.scrollIntoViewTimeout = null;
        }

        
        
        if (this.focusedElement) {
          this.scrollIntoViewTimeout = content.setTimeout(function () {
            this.scrollIntoViewTimeout = null;
            if (this.focusedElement && !FormVisibility.isVisible(this.focusedElement)) {
              this.focusedElement.scrollIntoView(false);
            }
          }.bind(this), RESIZE_SCROLL_DELAY);
        }
        break;
    }
  },

  receiveMessage: function fa_receiveMessage(msg) {
    let target = this.focusedElement;
    if (!target) {
      return;
    }

    let json = msg.json;
    switch (msg.name) {
      case "Forms:Input:Value": {
        target.value = json.value;

        let event = content.document.createEvent('HTMLEvents');
        event.initEvent('input', true, false);
        target.dispatchEvent(event);
        break;
      }

      case "Forms:Select:Choice":
        let options = target.options;
        let valueChanged = false;
        if ("index" in json) {
          if (options.selectedIndex != json.index) {
            options.selectedIndex = json.index;
            valueChanged = true;
          }
        } else if ("indexes" in json) {
          for (let i = 0; i < options.length; i++) {
            let newValue = (json.indexes.indexOf(i) != -1);
            if (options.item(i).selected != newValue) {
              options.item(i).selected = newValue;
              valueChanged = true;
            }
          }
        }

        
        if (valueChanged) {
          let event = content.document.createEvent('HTMLEvents');
          event.initEvent('change', true, true);
          target.dispatchEvent(event);
        }
        break;

      case "Forms:Select:Blur": {
        this.setFocusedElement(null);
        break;
      }
    }
  },

  observe: function fa_observe(subject, topic, data) {
    Services.obs.removeObserver(this, "xpcom-shutdown");
    removeMessageListener("Forms:Select:Choice", this);
    removeMessageListener("Forms:Input:Value", this);
  },

  showKeyboard: function fa_showKeyboard(target) {
    if (this.isKeyboardOpened)
      return;

    if (target instanceof HTMLOptionElement)
      target = target.parentNode;

    let kbOpened = this.sendKeyboardState(target);
    if (this.isTextInputElement(target))
      this.isKeyboardOpened = kbOpened;

    this.setFocusedElement(target);
  },

  hideKeyboard: function fa_hideKeyboard() {
    sendAsyncMessage("Forms:Input", { "type": "blur" });
    this.isKeyboardOpened = false;
    this.setFocusedElement(null);
  },

  isFocusableElement: function fa_isFocusableElement(element) {
    if (element instanceof HTMLSelectElement ||
        element instanceof HTMLTextAreaElement)
      return true;

    if (element instanceof HTMLOptionElement &&
        element.parentNode instanceof HTMLSelectElement)
      return true;

    return (element instanceof HTMLInputElement &&
            !this.ignoredInputTypes.has(element.type));
  },

  isTextInputElement: function fa_isTextInputElement(element) {
    return element instanceof HTMLInputElement ||
           element instanceof HTMLTextAreaElement ||
           isContentEditable(element);
  },

  getTopLevelEditable: function fa_getTopLevelEditable(element) {
    function retrieveTopLevelEditable(element) {
      
      if (element instanceof HTMLHtmlElement)
        element = element.ownerDocument.body;
      else if (element instanceof HTMLDocument)
        element = element.body;

      while (element && !isContentEditable(element))
        element = element.parentNode;

      
      if (element &&
          element instanceof HTMLBodyElement &&
          element.ownerDocument.defaultView != content.document.defaultView)
        return element.ownerDocument.defaultView.frameElement;
    }

    if (element instanceof HTMLIFrameElement)
      return element;

    return retrieveTopLevelEditable(element) || element;
  },

  sendKeyboardState: function(element) {
    
    
    let readonly = element.getAttribute("readonly");
    if (readonly) {
      return false;
    }

    sendAsyncMessage("Forms:Input", getJSON(element));
    return true;
  }
};

FormAssistant.init();


function isContentEditable(element) {
  if (element.isContentEditable || element.designMode == "on")
    return true;

  
  
  if (element instanceof HTMLIFrameElement &&
      element.contentDocument &&
      (element.contentDocument.body.isContentEditable ||
       element.contentDocument.designMode == "on"))
    return true;

  return element.ownerDocument && element.ownerDocument.designMode == "on";
}

function getJSON(element) {
  let type = element.type || "";
  let value = element.value || ""

  
  if (isContentEditable(element)) {
    type = "text";
    value = element.textContent;
  }

  
  
  let attributeType = element.getAttribute("type") || "";

  if (attributeType) {
    var typeLowerCase = attributeType.toLowerCase(); 
    switch (typeLowerCase) {
      case "datetime":
      case "datetime-local":
      case "range":
        type = typeLowerCase;
        break;
    }
  }

  
  
  
  
  
  
  let inputmode = element.getAttribute('x-inputmode');
  if (inputmode) {
    inputmode = inputmode.toLowerCase();
  } else {
    inputmode = '';
  }

  return {
    "type": type.toLowerCase(),
    "choices": getListForElement(element),
    "value": value,
    "inputmode": inputmode,
    "selectionStart": element.selectionStart,
    "selectionEnd": element.selectionEnd
  };
}

function getListForElement(element) {
  if (!(element instanceof HTMLSelectElement))
    return null;

  let optionIndex = 0;
  let result = {
    "multiple": element.multiple,
    "choices": []
  };

  
  
  
  
  let children = element.children;
  for (let i = 0; i < children.length; i++) {
    let child = children[i];

    if (child instanceof HTMLOptGroupElement) {
      result.choices.push({
        "group": true,
        "text": child.label || child.firstChild.data,
        "disabled": child.disabled
      });

      let subchildren = child.children;
      for (let j = 0; j < subchildren.length; j++) {
        let subchild = subchildren[j];
        result.choices.push({
          "group": false,
          "inGroup": true,
          "text": subchild.text,
          "disabled": child.disabled || subchild.disabled,
          "selected": subchild.selected,
          "optionIndex": optionIndex++
        });
      }
    } else if (child instanceof HTMLOptionElement) {
      result.choices.push({
        "group": false,
        "inGroup": false,
        "text": child.text,
        "disabled": child.disabled,
        "selected": child.selected,
        "optionIndex": optionIndex++
      });
    }
  }

  return result;
};

