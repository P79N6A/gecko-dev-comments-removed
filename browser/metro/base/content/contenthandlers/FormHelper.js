




let Ci = Components.interfaces;
let Cc = Components.classes;

dump("### FormHelper.js loaded\n");

let HTMLTextAreaElement = Ci.nsIDOMHTMLTextAreaElement;
let HTMLInputElement = Ci.nsIDOMHTMLInputElement;
let HTMLSelectElement = Ci.nsIDOMHTMLSelectElement;
let HTMLIFrameElement = Ci.nsIDOMHTMLIFrameElement;
let HTMLDocument = Ci.nsIDOMHTMLDocument;
let HTMLHtmlElement = Ci.nsIDOMHTMLHtmlElement;
let HTMLBodyElement = Ci.nsIDOMHTMLBodyElement;
let HTMLLabelElement = Ci.nsIDOMHTMLLabelElement;
let HTMLButtonElement = Ci.nsIDOMHTMLButtonElement;
let HTMLOptGroupElement = Ci.nsIDOMHTMLOptGroupElement;
let HTMLOptionElement = Ci.nsIDOMHTMLOptionElement;
let XULMenuListElement = Ci.nsIDOMXULMenuListElement;




function FormAssistant() {
  addMessageListener("FormAssist:Closed", this);
  addMessageListener("FormAssist:ChoiceSelect", this);
  addMessageListener("FormAssist:ChoiceChange", this);
  addMessageListener("FormAssist:AutoComplete", this);
  addMessageListener("FormAssist:Update", this);

  


  addEventListener("text", this, false);
  addEventListener("focus", this, true);
  addEventListener("blur", this, true);
  addEventListener("pageshow", this, false);
  addEventListener("pagehide", this, false);
  addEventListener("submit", this, false);
}

FormAssistant.prototype = {
  _els: Cc["@mozilla.org/eventlistenerservice;1"].getService(Ci.nsIEventListenerService),
  _open: false,
  _focusSync: false,
  _debugEvents: false,
  _selectWrapper: null,
  _currentElement: null,
  invalidSubmit: false,

  get focusSync() {
    return this._focusSync;
  },

  set focusSync(aVal) {
    this._focusSync = aVal;
  },

  get currentElement() {
    return this._currentElement;
  },

  set currentElement(aElement) {
    if (!aElement || !this._isVisibleElement(aElement)) {
      return null;
    }

    this._currentElement = aElement;
    gFocusManager.setFocus(this._currentElement, Ci.nsIFocusManager.FLAG_NOSCROLL);

    
    
    this._executeDelayed(function(self) {
      
      
      
      
      if (self._isVisibleElement(gFocusManager.focusedElement)) {
        self._sendJsonMsgWrapper("FormAssist:Show");
      }
    });
    return this._currentElement;
  },

  open: function formHelperOpen(aElement, aEvent) {
    
    
    if (aElement instanceof HTMLOptionElement &&
        aElement.parentNode instanceof HTMLSelectElement &&
        !aElement.disabled) {
      aElement = aElement.parentNode;
    }

    
    if (aElement instanceof HTMLSelectElement && aEvent) {
      if ((aElement.multiple || aElement.size > 1) &&
          aEvent.mozInputSource != Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH) {
        return false;
      }
      
      aEvent.preventDefault();
      aEvent.stopPropagation();
    }

    
    
    
    
    if (!this._isValidElement(aElement)) {
      let passiveButtons = { button: true, checkbox: true, file: true, radio: true, reset: true };
      if ((aElement instanceof HTMLInputElement || aElement instanceof HTMLButtonElement) &&
          passiveButtons[aElement.type] && !aElement.disabled)
        return false;
      return this.close();
    }

    
    if (this._isEditable(aElement)) {
      aElement = this._getTopLevelEditable(aElement);
    }

    
    if (!this._isSelectElement(aElement) &&
        !this._isAutocomplete(aElement)) {
      return this.close();
    }

    
    if (this._isAutocomplete(aElement) && this._open && this._isNavigationKey(aEvent)) {
      return false;
    }

    
    this.currentElement = aElement;
    return this._open = true;
  },

  close: function close() {
    if (this._open) {
      this._currentElement = null;
      sendAsyncMessage("FormAssist:Hide", { });
      this._open = false;
    }

    return this._open;
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn(aMessage.name);

    let currentElement = this.currentElement;
    if ((!this._isAutocomplete(currentElement) &&
         !getWrapperForElement(currentElement)) ||
        !currentElement) {
      return;
    }

    let json = aMessage.json;

    switch (aMessage.name) {
      case "FormAssist:ChoiceSelect": {
        this._selectWrapper = getWrapperForElement(currentElement);
        this._selectWrapper.select(json.index, json.selected);
        break;
      }

      case "FormAssist:ChoiceChange": {
        
        
        this._selectWrapper = getWrapperForElement(currentElement);
        this._selectWrapper.fireOnChange();

        
        
        
        
        this._executeDelayed(function(self) {
          let currentElement = self.currentElement;
          if (!currentElement)
            return;
          self._currentElement = currentElement;
        });
        break;
      }

      case "FormAssist:AutoComplete": {
        try {
          currentElement = currentElement.QueryInterface(Ci.nsIDOMNSEditableElement);
          let imeEditor = currentElement.editor.QueryInterface(Ci.nsIEditorIMESupport);
          if (imeEditor.composing)
            imeEditor.forceCompositionEnd();
        }
        catch(e) {}

        currentElement.value = json.value;

        let event = currentElement.ownerDocument.createEvent("Events");
        event.initEvent("DOMAutoComplete", true, true);
        currentElement.dispatchEvent(event);
        break;
      }

      case "FormAssist:Closed":
        currentElement.blur();
        this._open = false;
        break;

      case "FormAssist:Update":
        this._sendJsonMsgWrapper("FormAssist:Show");
        break;
    }
  },

  handleEvent: function formHelperHandleEvent(aEvent) {
    if (this._debugEvents) Util.dumpLn(aEvent.type, this.currentElement);
    
    
    let shouldIgnoreFocus = (aEvent.type == "focus" && !this._open && !this.focusSync);
    if ((!this._open && aEvent.type != "focus") || shouldIgnoreFocus) {
      return;
    }

    let currentElement = this.currentElement;
    switch (aEvent.type) {
      case "submit":
        
        this.close();
        break;

      case "pagehide":
      case "pageshow":
        
        
        
        if (gFocusManager.focusedElement != currentElement)
          this.close();
        break;

      case "focus":
        let focusedElement =
          gFocusManager.getFocusedElementForWindow(content, true, {}) ||
          aEvent.target;

        
        
        
        if (focusedElement && this._isEditable(focusedElement)) {
          let editableElement = this._getTopLevelEditable(focusedElement);
          if (this._isValidElement(editableElement)) {
            this._executeDelayed(function(self) {
              self.open(editableElement);
            });
          }
          return;
        }

        
        
        if (!currentElement) {
          if (focusedElement && this._isValidElement(focusedElement)) {
            this._executeDelayed(function(self) {
              self.open(focusedElement);
            });
          }
          return;
        }

        if (this._currentElement != focusedElement)
          this.currentElement = focusedElement;
        break;

      case "blur":
        content.setTimeout(function(self) {
          if (!self._open)
            return;

          
          
          let focusedElement = gFocusManager.getFocusedElementForWindow(content, true, {});
          if (!focusedElement)
            self.close();
        }, 0, this);
        break;

      case "text":
        if (this._isAutocomplete(aEvent.target)) {
          this._sendJsonMsgWrapper("FormAssist:AutoComplete");
        }
        break;
    }
  },

  _isNavigationKey: function (aEvent) {
    
    if (aEvent.keyCode) {
      let navigationKeys = [
        aEvent.DOM_VK_DOWN,
        aEvent.DOM_VK_UP,
        aEvent.DOM_VK_LEFT,
        aEvent.DOM_VK_RIGHT,
        aEvent.DOM_VK_PAGE_UP,
        aEvent.DOM_VK_PAGE_DOWN];

      return navigationKeys.indexOf(aEvent.keyCode) != -1;
    }
    return false;
  },

  _executeDelayed: function formHelperExecuteSoon(aCallback) {
    let self = this;
    let timer = new Util.Timeout(function() {
      aCallback(self);
    });
    timer.once(0);
  },

  _isEditable: function formHelperIsEditable(aElement) {
    if (!aElement)
      return false;
    let canEdit = false;

    if (aElement.isContentEditable || aElement.designMode == "on") {
      canEdit = true;
    } else if (aElement instanceof HTMLIFrameElement &&
               (aElement.contentDocument.body.isContentEditable ||
                aElement.contentDocument.designMode == "on")) {
      canEdit = true;
    } else {
      canEdit = aElement.ownerDocument && aElement.ownerDocument.designMode == "on";
    }

    return canEdit;
  },

  _getTopLevelEditable: function formHelperGetTopLevelEditable(aElement) {
    if (!(aElement instanceof HTMLIFrameElement)) {
      let element = aElement;

      
      if (element instanceof HTMLHtmlElement)
        element = element.ownerDocument.body;
      else if (element instanceof HTMLDocument)
        element = element.body;

      while (element && !this._isEditable(element))
        element = element.parentNode;

      
      if (element && element instanceof HTMLBodyElement && element.ownerDocument.defaultView != content.document.defaultView)
        return element.ownerDocument.defaultView.frameElement;
    }

    return aElement;
  },

  _isAutocomplete: function formHelperIsAutocomplete(aElement) {
    if (aElement instanceof HTMLInputElement) {
      if (aElement.getAttribute("type") == "password")
        return false;

      let autocomplete = aElement.getAttribute("autocomplete");
      let allowedValues = ["off", "false", "disabled"];
      if (allowedValues.indexOf(autocomplete) == -1)
        return true;
    }

    return false;
  },

  




  _getListSuggestions: function formHelperGetListSuggestions(aElement) {
    if (!(aElement instanceof HTMLInputElement) || !aElement.list)
      return [];

    let suggestions = [];
    let filter = !aElement.hasAttribute("mozNoFilter");
    let lowerFieldValue = aElement.value.toLowerCase();

    let options = aElement.list.options;
    let length = options.length;
    for (let i = 0; i < length; i++) {
      let item = options.item(i);

      let label = item.value;
      if (item.label)
        label = item.label;
      else if (item.text)
        label = item.text;

      if (filter && label.toLowerCase().indexOf(lowerFieldValue) == -1)
        continue;
       suggestions.push({ label: label, value: item.value });
    }

    return suggestions;
  },

  _isValidElement: function formHelperIsValidElement(aElement) {
    if (!aElement.getAttribute)
      return false;

    let formExceptions = { button: true, checkbox: true, file: true, image: true, radio: true, reset: true, submit: true };
    if (aElement instanceof HTMLInputElement && formExceptions[aElement.type])
      return false;

    if (aElement instanceof HTMLButtonElement ||
        (aElement.getAttribute("role") == "button" && aElement.hasAttribute("tabindex")))
      return false;

    return this._isNavigableElement(aElement) && this._isVisibleElement(aElement);
  },

  _isNavigableElement: function formHelperIsNavigableElement(aElement) {
    if (aElement.disabled || aElement.getAttribute("tabindex") == "-1")
      return false;

    if (aElement.getAttribute("role") == "button" && aElement.hasAttribute("tabindex"))
      return true;

    if (this._isSelectElement(aElement) || aElement instanceof HTMLTextAreaElement)
      return true;

    if (aElement instanceof HTMLInputElement || aElement instanceof HTMLButtonElement)
      return !(aElement.type == "hidden");

    return this._isEditable(aElement);
  },

  _isVisibleElement: function formHelperIsVisibleElement(aElement) {
    if (!aElement || !aElement.ownerDocument) {
      return false;
    }
    let style = aElement.ownerDocument.defaultView.getComputedStyle(aElement, null);
    if (!style)
      return false;

    let isVisible = (style.getPropertyValue("visibility") != "hidden");
    let isOpaque = (style.getPropertyValue("opacity") != 0);

    let rect = aElement.getBoundingClientRect();

    
    
    
    
    
    
    return isVisible && (isOpaque || this._isSelectElement(aElement)) && (rect.height != 0 || rect.width != 0);
  },

  _isSelectElement: function formHelperIsSelectElement(aElement) {
    return (aElement instanceof HTMLSelectElement || aElement instanceof XULMenuListElement);
  },

  
  _getCaretRect: function _formHelperGetCaretRect() {
    let element = this.currentElement;
    let focusedElement = gFocusManager.getFocusedElementForWindow(content, true, {});
    if (element && (element.mozIsTextField && element.mozIsTextField(false) ||
        element instanceof HTMLTextAreaElement) && focusedElement == element && this._isVisibleElement(element)) {
      let utils = Util.getWindowUtils(element.ownerDocument.defaultView);
      let rect = utils.sendQueryContentEvent(utils.QUERY_CARET_RECT, element.selectionEnd, 0, 0, 0);
      if (rect) {
        let scroll = ContentScroll.getScrollOffset(element.ownerDocument.defaultView);
        return new Rect(scroll.x + rect.left, scroll.y + rect.top, rect.width, rect.height);
      }
    }

    return new Rect(0, 0, 0, 0);
  },

  
  _getRect: function _formHelperGetRect(aOptions={}) {
    const kDistanceMax = 100;
    let element = this.currentElement;
    let elRect = getBoundingContentRect(element);

    if (aOptions.alignToLabel) {
      let labels = this._getLabels();
      for (let i=0; i<labels.length; i++) {
        let labelRect = labels[i].rect;
        if (labelRect.left < elRect.left) {
          let isClose = Math.abs(labelRect.left - elRect.left) - labelRect.width < kDistanceMax &&
                        Math.abs(labelRect.top - elRect.top) - labelRect.height < kDistanceMax;
          if (isClose) {
            let width = labelRect.width + elRect.width + (elRect.left - labelRect.left - labelRect.width);
            return new Rect(labelRect.left, labelRect.top, width, elRect.height).expandToIntegers();
          }
        }
      }
    }
    return elRect;
  },

  _getLabels: function formHelperGetLabels() {
    let associatedLabels = [];
    if (!this.currentElement)
      return associatedLabels;
    let element = this.currentElement;
    let labels = element.ownerDocument.getElementsByTagName("label");
    for (let i=0; i<labels.length; i++) {
      let label = labels[i];
      if ((label.control == element || label.getAttribute("for") == element.id) && this._isVisibleElement(label)) {
        associatedLabels.push({
          rect: getBoundingContentRect(label),
          title: label.textContent
        });
      }
    }

    return associatedLabels;
  },

  _sendJsonMsgWrapper: function (aMsg) {
    let json = this._getJSON();
    if (json) {
      sendAsyncMessage(aMsg, json);
    }
  },

  _getJSON: function() {
    let element = this.currentElement;
    if (!element) {
      return null;
    }
    let choices = getListForElement(element);
    let editable = (element instanceof HTMLInputElement && element.mozIsTextField(false)) || this._isEditable(element);

    let labels = this._getLabels();
    return {
      current: {
        id: element.id,
        name: element.name,
        title: labels.length ? labels[0].title : "",
        value: element.value,
        maxLength: element.maxLength,
        type: (element.getAttribute("type") || "").toLowerCase(),
        choices: choices,
        isAutocomplete: this._isAutocomplete(element),
        list: this._getListSuggestions(element),
        rect: this._getRect(),
        caretRect: this._getCaretRect(),
        editable: editable
      },
    };
  },

  



  _filterRadioButtons: function(aNodes) {
    
    let chosenRadios = {};
    for (let i=0; i < aNodes.length; i++) {
      let node = aNodes[i];
      if (node.type == "radio" && (!chosenRadios.hasOwnProperty(node.name) || node.checked))
        chosenRadios[node.name] = node;
    }

    
    let result = [];
    for (let i=0; i < aNodes.length; i++) {
      let node = aNodes[i];
      if (node.type == "radio" && chosenRadios[node.name] != node)
        continue;
      result.push(node);
    }
    return result;
  }
};









function getWrapperForElement(aElement) {
  let wrapper = null;
  if (aElement instanceof HTMLSelectElement) {
    wrapper = new SelectWrapper(aElement);
  }
  else if (aElement instanceof XULMenuListElement) {
    wrapper = new MenulistWrapper(aElement);
  }

  return wrapper;
}

function getListForElement(aElement) {
  let wrapper = getWrapperForElement(aElement);
  if (!wrapper)
    return null;

  let optionIndex = 0;
  let result = {
    multiple: wrapper.getMultiple(),
    choices: []
  };

  
  
  
  
  let children = wrapper.getChildren();
  for (let i = 0; i < children.length; i++) {
    let child = children[i];
    if (wrapper.isGroup(child)) {
      
      
      result.choices.push({ group: true,
                            text: child.label || child.firstChild.data,
                            disabled: child.disabled
                          });
      let subchildren = child.children;
      for (let j = 0; j < subchildren.length; j++) {
        let subchild = subchildren[j];
        result.choices.push({
          group: false,
          inGroup: true,
          text: wrapper.getText(subchild),
          disabled: child.disabled || subchild.disabled,
          selected: subchild.selected,
          optionIndex: optionIndex++
        });
      }
    }
    else if (wrapper.isOption(child)) {
      
      result.choices.push({
        group: false,
        inGroup: false,
        text: wrapper.getText(child),
        disabled: child.disabled,
        selected: child.selected,
        optionIndex: optionIndex++
      });
    }
  }

  return result;
}


function SelectWrapper(aControl) {
  this._control = aControl;
}

SelectWrapper.prototype = {
  getSelectedIndex: function() {
    return this._control.selectedIndex;
  },

  getMultiple: function() {
    return this._control.multiple;
  },

  getOptions: function() {
    return this._control.options;
  },

  getChildren: function() {
    return this._control.children;
  },

  getText: function(aChild) {
    return aChild.text;
  },

  isOption: function(aChild) {
    return aChild instanceof HTMLOptionElement;
  },

  isGroup: function(aChild) {
    return aChild instanceof HTMLOptGroupElement;
  },

  select: function(aIndex, aSelected) {
    let options = this._control.options;
    if (this.getMultiple())
      options[aIndex].selected = aSelected;
    else
      options.selectedIndex = aIndex;
  },

  fireOnChange: function() {
    let control = this._control;
    let evt = this._control.ownerDocument.createEvent("Events");
    evt.initEvent("change", true, true, this._control.ownerDocument.defaultView, 0,
                  false, false,
                  false, false, null);
    content.setTimeout(function() {
      control.dispatchEvent(evt);
    }, 0);
  }
};




function MenulistWrapper(aControl) {
  this._control = aControl;
}

MenulistWrapper.prototype = {
  getSelectedIndex: function() {
    let control = this._control.wrappedJSObject || this._control;
    let result = control.selectedIndex;
    return ((typeof result == "number" && !isNaN(result)) ? result : -1);
  },

  getMultiple: function() {
    return false;
  },

  getOptions: function() {
    let control = this._control.wrappedJSObject || this._control;
    return control.menupopup.children;
  },

  getChildren: function() {
    let control = this._control.wrappedJSObject || this._control;
    return control.menupopup.children;
  },

  getText: function(aChild) {
    return aChild.label;
  },

  isOption: function(aChild) {
    return aChild instanceof Ci.nsIDOMXULSelectControlItemElement;
  },

  isGroup: function(aChild) {
    return false;
  },

  select: function(aIndex, aSelected) {
    let control = this._control.wrappedJSObject || this._control;
    control.selectedIndex = aIndex;
  },

  fireOnChange: function() {
    let control = this._control;
    let evt = document.createEvent("XULCommandEvent");
    evt.initCommandEvent("command", true, true, window, 0,
                         false, false,
                         false, false, null);
    content.setTimeout(function() {
      control.dispatchEvent(evt);
    }, 0);
  }
};
