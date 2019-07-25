





dump("###################################### forms.js loaded\n");

"use strict";

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
    Services.obs.addObserver(this, "ime-enabled-state-changed", false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  isKeyboardOpened: false,
  previousTarget : null,
  handleEvent: function fa_handleEvent(evt) {
    let previousTarget = this.previousTarget;
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
    
        if (evt.target instanceof HTMLSelectElement) { 
          content.setTimeout(function showIMEForSelect() {
            sendAsyncMessage("Forms:Input", getJSON(evt.target));
          });
          this.previousTarget = evt.target;
        } else if (evt.target instanceof HTMLOptionElement &&
                   evt.target.parentNode instanceof HTMLSelectElement) {
          content.setTimeout(function showIMEForSelect() {
            sendAsyncMessage("Forms:Input", getJSON(evt.target.parentNode));
          });
        } else if ((target instanceof HTMLInputElement && !ignore[target.type]) ||
                    target instanceof HTMLTextAreaElement) {
          this.isKeyboardOpened = this.tryShowIme(evt.target);
          this.previousTarget = evt.target;
        }
        break;

      case "blur":
        if (this.previousTarget) {
          sendAsyncMessage("Forms:Input", { "type": "blur" });
          this.previousTarget = null;
        }
        break;

      case "resize":
        if (!this.isKeyboardOpened)
          return;

        let focusedElement = this.previousTarget;
        if (focusedElement) {
          focusedElement.scrollIntoView(false);
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
    let target = this.previousTarget;
    if (!target) {
      return;
    }

    let json = msg.json;
    switch (msg.name) {
      case "Forms:Input:Value":
        target.value = json.value;
        break;

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
            this.previousTarget = null;
            return;
          } else {
            this.previousTarget = target;
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

  return {
    "type": type.toLowerCase(),
    "choices": getListForElement(element)
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

