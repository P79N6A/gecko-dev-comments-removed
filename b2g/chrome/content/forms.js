





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

let HTMLInputElement = Ci.nsIDOMHTMLInputElement;
let HTMLTextAreaElement = Ci.nsIDOMHTMLTextAreaElement;
let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLOptGroupElement = Ci.nsIDOMHTMLOptGroupElement;
let HTMLOptionElement = Ci.nsIDOMHTMLOptionElement;

let FormAssistant = {
  init: function fa_init() {
    addEventListener("focus", this, true, false);
    addEventListener("blur", this, true, false);
    addEventListener("keypress", this, true, false);
    addEventListener("resize", this, true, false);
    addMessageListener("Forms:Select:Choice", this);
    addMessageListener("Forms:Input:Value", this);
    addMessageListener("Forms:Select:Blur", this);
    Services.obs.addObserver(this, "ime-enabled-state-changed", false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  isKeyboardOpened: false,
  focusedElement : null,
  selectionStart: 0,
  selectionEnd: 0,

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
        if (this.isKeyboardOpened)
          return;

        let ignore = {
          button: true,
          file: true,
          checkbox: true,
          radio: true,
          reset: true,
          submit: true,
          image: true
        };
    
        if (target instanceof HTMLSelectElement) { 
          content.setTimeout(function showIMEForSelect() {
            sendAsyncMessage("Forms:Input", getJSON(target));
          });
          this.setFocusedElement(target);
        } else if (target instanceof HTMLOptionElement &&
                   target.parentNode instanceof HTMLSelectElement) {
          target = target.parentNode;
          content.setTimeout(function showIMEForSelect() {
            sendAsyncMessage("Forms:Input", getJSON(target));
          });
          this.setFocusedElement(target);
        } else if ((target instanceof HTMLInputElement && !ignore[target.type]) ||
                    target instanceof HTMLTextAreaElement) {
          this.isKeyboardOpened = this.tryShowIme(target);
          this.setFocusedElement(target);
        }
        break;

      case "blur":
        if (this.focusedElement) {
          sendAsyncMessage("Forms:Input", { "type": "blur" });
          this.setFocusedElement(null);
          this.isKeyboardOpened = false;
        }
        break;

      case 'mousedown':
        
        
        this.selectionStart = this.focusedElement.selectionStart;
        this.selectionEnd = this.focusedElement.selectionEnd;
        break;

      case 'mouseup':
        
        
        
        
        if (this.focusedElement.selectionStart !== this.selectionStart ||
            this.focusedElement.selectionEnd !== this.selectionEnd) {
          this.tryShowIme(this.focusedElement);
        }
        break;

      case "resize":
        if (!this.isKeyboardOpened)
          return;

        if (this.focusedElement) {
          this.focusedElement.scrollIntoView(false);
        }
        break;

      case "keypress":
        if (evt.keyCode != evt.DOM_VK_ESCAPE || !this.isKeyboardOpened)
          return;

        sendAsyncMessage("Forms:Input", { "type": "blur" });
        this.isKeyboardOpened = false;

        evt.preventDefault();
        evt.stopPropagation();
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
    switch (topic) {
      case "ime-enabled-state-changed":
        let isOpen = this.isKeyboardOpened;
        let shouldOpen = parseInt(data);
        if (shouldOpen && !isOpen) {
          let target = Services.fm.focusedElement;

          if (!target || !this.tryShowIme(target)) {
            this.setFocusedElement(null);
            return;
          } else {
            this.setFocusedElement(target);
          }
        } else if (!shouldOpen && isOpen) {
          sendAsyncMessage("Forms:Input", { "type": "blur" });
        }
        this.isKeyboardOpened = shouldOpen;
        break;

      case "xpcom-shutdown":
        Services.obs.removeObserver(this, "ime-enabled-state-changed", false);
        Services.obs.removeObserver(this, "xpcom-shutdown");
        removeMessageListener("Forms:Select:Choice", this);
        removeMessageListener("Forms:Input:Value", this);
        break;
    }
  },

  tryShowIme: function(element) {
    if (!element) {
      return;
    }

    
    
    let readonly = element.getAttribute("readonly");
    if (readonly) {
      return false;
    }

    sendAsyncMessage("Forms:Input", getJSON(element));
    return true;
  }
};

FormAssistant.init();


function getJSON(element) {
  let type = element.type || "";

  
  
  
  
  let attributeType = element.getAttribute("type") || "";

  if (attributeType) {
    var typeLowerCase = attributeType.toLowerCase(); 
    switch (typeLowerCase) {
      case "date":
      case "time":
      case "datetime":
      case "datetime-local":
        type = typeLowerCase;
        break;
    }
  }

  
  
  
  
  
  
  let inputmode = element.getAttribute('inputmode');
  if (inputmode) {
    inputmode = inputmode.toLowerCase();
  } else {
    inputmode = '';
  }

  return {
    "type": type.toLowerCase(),
    "choices": getListForElement(element),
    "value": element.value,
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

