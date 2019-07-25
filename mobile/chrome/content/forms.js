








































let Ci = Components.interfaces;
let Cc = Components.classes;

dump("###################################### forms.js loaded\n");

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
  addMessageListener("FormAssist:Previous", this);
  addMessageListener("FormAssist:Next", this);
  addMessageListener("FormAssist:ChoiceSelect", this);
  addMessageListener("FormAssist:ChoiceChange", this);
  addMessageListener("FormAssist:AutoComplete", this);
  addMessageListener("Content:SetWindowSize", this);

  


  addEventListener("text", this, false);

  addEventListener("keypress", this, true);
  addEventListener("keyup", this, false);
  addEventListener("focus", this, true);
  addEventListener("pageshow", this, false);
  addEventListener("pagehide", this, false);

  this._enabled = Services.prefs.getBoolPref("formhelper.enabled");
};

FormAssistant.prototype = {
  _selectWrapper: null,
  _currentIndex: -1,
  _elements: [],

  get currentElement() {
    return this._elements[this._currentIndex];
  },

  get currentIndex() {
    return this._currentIndex;
  },

  set currentIndex(aIndex) {
    let element = this._elements[aIndex];
    if (!element)
      return -1;

    if (this._isVisibleElement(element)) {
      this._currentIndex = aIndex;
      gFocusManager.setFocus(element, Ci.nsIFocusManager.FLAG_NOSCROLL);

      
      
      this._executeDelayed(function(self) {
        
        
        
        
        if (self._isVisibleElement(gFocusManager.focusedElement))
          sendAsyncMessage("FormAssist:Show", self._getJSON());
      });
    } else {
      
      
      this._elements = [];
      let currentIndex = this._getAllElements(gFocusManager.focusedElement)

      if (aIndex < this._currentIndex)
        this.currentIndex = currentIndex - 1;
      else if (aIndex > this._currentIndex)
        this.currentIndex = currentIndex + 1;
      else if (this._currentIndex != currentIndex)
        this.currentIndex = currentIndex;
    }
    return element;
  },

  _open: false,
  open: function formHelperOpen(aElement) {
    
    if (aElement instanceof HTMLOptionElement && aElement.parentNode instanceof HTMLSelectElement && !aElement.disabled) {
      aElement = aElement.parentNode;
    }

    
    
    
    
    if (!this._isValidElement(aElement)) {
      let passiveButtons = { button: true, checkbox: true, file: true, radio: true, reset: true };
      if ((aElement instanceof HTMLInputElement || aElement instanceof HTMLButtonElement) &&
          passiveButtons[aElement.type] && !aElement.disabled)
        return false;

      return this.close();
    }

    
    if (this._isEditable(aElement))
      aElement = this._getTopLevelEditable(aElement);

    
    
    if (this._open && aElement == this.currentElement) {
      
      
      let utils = Util.getWindowUtils(content);
      if (utils.IMEStatus == utils.IME_STATUS_DISABLED && aElement instanceof HTMLInputElement && aElement.mozIsTextField(false)) {
        aElement.blur();
        aElement.focus();
      }

      
      
      
      if (aElement instanceof HTMLSelectElement) {
        this._executeDelayed(function(self) {
          sendAsyncMessage("FormAssist:Show", self._getJSON());
        });
      }

      return false;
    }

    
    
    this._enabled = Services.prefs.getBoolPref("formhelper.enabled");
    if (!this._enabled && !this._isSelectElement(aElement))
      return this.close();

    if (this._enabled) {
      this._elements = [];
      this.currentIndex = this._getAllElements(aElement);
    }
    else {
      this._elements = [aElement];
      this.currentIndex = 0;
    }

    return this._open = true;
  },

  close: function close() {
    if (this._open) {
      this._currentIndex = -1;
      this._elements = [];
      sendAsyncMessage("FormAssist:Hide", { });
      this._open = false;
    }

    return this._open;
  },

  receiveMessage: function receiveMessage(aMessage) {
    let currentElement = this.currentElement;
    if ((!this._enabled && !getWrapperForElement(currentElement)) || !currentElement)
      return;

    let json = aMessage.json;
    switch (aMessage.name) {
      case "FormAssist:Previous":
        this.currentIndex--;
        break;

      case "FormAssist:Next":
        this.currentIndex++;
        break;

      case "Content:SetWindowSize":
        
        
        sendAsyncMessage("FormAssist:Resize", this._getJSON());
        break;

      case "FormAssist:ChoiceSelect": {
        this._selectWrapper = getWrapperForElement(currentElement);
        this._selectWrapper.select(json.index, json.selected, json.clearAll);
        break;
      }

      case "FormAssist:ChoiceChange": {
        
        
        this._selectWrapper.fireOnChange();

        
        
        
        
        this._executeDelayed(function(self) {
          let currentElement = self.currentElement;
          if (!currentElement)
            return;

          self._elements = [];
          self._currentIndex = self._getAllElements(currentElement);
        });
        break;
      }

      case "FormAssist:AutoComplete": {
        currentElement.value = json.value;

        let event = currentElement.ownerDocument.createEvent("Events");
        event.initEvent("DOMAutoComplete", true, true);
        currentElement.dispatchEvent(event);
        break;
      }

      case "FormAssist:Closed":
        currentElement.blur();
        this._currentIndex = null;
        this._open = false;
        break;
    }
  },

  _els: Cc["@mozilla.org/eventlistenerservice;1"].getService(Ci.nsIEventListenerService),
  _hasKeyListener: function _hasKeyListener(aElement) {
    let els = this._els;
    let listeners = els.getListenerInfoFor(aElement, {});
    for (let i = 0; i < listeners.length; i++) {
      let listener = listeners[i];
      if (["keyup", "keydown", "keypress"].indexOf(listener.type) != -1
          && !listener.inSystemEventGroup) {
        return true;
      }
    }
    return false;
  },

  focusSync: false,
  handleEvent: function formHelperHandleEvent(aEvent) {
    
    
    let shouldIgnoreFocus = (aEvent.type == "focus" && !this._open && !this.focusSync);
    if ((!this._open && aEvent.type != "focus") || shouldIgnoreFocus)
      return;

    let currentElement = this.currentElement;
    switch (aEvent.type) {
      case "pagehide":
      case "pageshow":
        
        
        
        if (gFocusManager.focusedElement != currentElement)
          this.close();
        break;

      case "focus":
        let focusedElement = gFocusManager.getFocusedElementForWindow(content, true, {}) || aEvent.target;

        
        
        
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

        let focusedIndex = this._getIndexForElement(focusedElement);
        if (focusedIndex != -1 && this.currentIndex != focusedIndex)
          this.currentIndex = focusedIndex;
        break;

      case "text":
        if (this._isAutocomplete(aEvent.target))
          sendAsyncMessage("FormAssist:AutoComplete", this._getJSON());
        break;

      
      
      case "keypress":
        
        
        if (!currentElement)
          return;

        let formExceptions = { button: true, checkbox: true, file: true, image: true, radio: true, reset: true, submit: true };
        if (this._isSelectElement(currentElement) || formExceptions[currentElement.type] ||
            currentElement instanceof HTMLButtonElement || (currentElement.getAttribute("role") == "button" && currentElement.hasAttribute("tabindex"))) {
          switch (aEvent.keyCode) {
            case aEvent.DOM_VK_RIGHT:
              this._executeDelayed(function(self) {
                self.currentIndex++;
              });
              aEvent.stopPropagation();
              aEvent.preventDefault();
              break;

            case aEvent.DOM_VK_LEFT:
              this._executeDelayed(function(self) {
                self.currentIndex--;
              });
              aEvent.stopPropagation();
              aEvent.preventDefault();
              break;
          }
        }
        break;

      case "keyup":
        
        
        if (!currentElement)
          return;

        switch (aEvent.keyCode) {
          case aEvent.DOM_VK_DOWN:
            if (currentElement instanceof HTMLInputElement && !this._isAutocomplete(currentElement)) {
              if (this._hasKeyListener(currentElement))
                return;
            } else if (currentElement instanceof HTMLTextAreaElement) {
              let existSelection = currentElement.selectionEnd - currentElement.selectionStart;
              let isEnd = (currentElement.textLength == currentElement.selectionEnd);
              if (!isEnd || existSelection)
                return;
            } else if (getListForElement(currentElement)) {
              this.currentIndex = this.currentIndex;
              return;
            }

            this.currentIndex++;
            break;

          case aEvent.DOM_VK_UP:
            if (currentElement instanceof HTMLInputElement && !this._isAutocomplete(currentElement)) {
              if (this._hasKeyListener(currentElement))
                return;
            } else if (currentElement instanceof HTMLTextAreaElement) {
              let existSelection = currentElement.selectionEnd - currentElement.selectionStart;
              let isStart = (currentElement.selectionEnd == 0);
              if (!isStart || existSelection)
                return;
            } else if (this._isSelectElement(currentElement)) {
              this.currentIndex = this.currentIndex;
              return;
            }

            this.currentIndex--;
            break;

          case aEvent.DOM_VK_RETURN:
            if (!this._isVisibleElement(currentElement))
              this.close();
            break;

          case aEvent.DOM_VK_ESCAPE:
          case aEvent.DOM_VK_TAB:
            break;

          default:
            if (this._isAutocomplete(aEvent.target))
              sendAsyncMessage("FormAssist:AutoComplete", this._getJSON());
            else if (currentElement && this._isSelectElement(currentElement))
              this.currentIndex = this.currentIndex;
            break;
        }

        let caretRect = this._getCaretRect();
        if (!caretRect.isEmpty())
          sendAsyncMessage("FormAssist:Update", { caretRect: caretRect });
    }
  },

  _executeDelayed: function formHelperExecuteSoon(aCallback) {
    let self = this;
    let timer = new Util.Timeout(function() {
      aCallback(self);
    });
    timer.once(0);
  },

  _filterEditables: function formHelperFilterEditables(aNodes) {
    let result = [];
    for (let i = 0; i < aNodes.length; i++) {
      let node = aNodes[i];

      
      if (this._isEditable(node)) {
        let editableElement = this._getTopLevelEditable(node);
        if (result.indexOf(editableElement) == -1)
          result.push(editableElement);
      }
      else {
        result.push(node);
      }
    }
    return result;
  },

  _isEditable: function formHelperIsEditable(aElement) {
    let canEdit = false;

    if (aElement.isContentEditable || aElement.designMode == "on") {
      canEdit = true;
    } else if (aElement instanceof HTMLIFrameElement && (aElement.contentDocument.body.isContentEditable || aElement.contentDocument.designMode == "on")) {
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
      let autocomplete = aElement.getAttribute("autocomplete");
      let allowedValues = ["off", "false", "disabled"];
      if (allowedValues.indexOf(autocomplete) == -1)
        return true;
    }

    return false;
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
    let style = aElement.ownerDocument.defaultView.getComputedStyle(aElement, null);
    if (!style)
      return false;

    let isVisible = (style.getPropertyValue("visibility") != "hidden");
    let isOpaque = (style.getPropertyValue("opacity") != 0);

    let rect = aElement.getBoundingClientRect();
    return isVisible && isOpaque && (rect.height != 0 || rect.width != 0);
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

  
  _getRect: function _formHelperGetRect() {
    const kDistanceMax = 100;
    let element = this.currentElement;
    let elRect = getBoundingContentRect(element);

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
    return elRect;
  },

  _getLabels: function formHelperGetLabels() {
    let associatedLabels = [];

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

  _getAllElements: function getAllElements(aElement) {
    
    
    let document = aElement.ownerDocument;
    if (!document)
      return;

    let documents = Util.getAllDocuments(document);

    let elements = this._elements;
    for (let i = 0; i < documents.length; i++) {
      let selector = "input, button, select, textarea, [role=button], iframe, [contenteditable=true]";
      let nodes = documents[i].querySelectorAll(selector);
      nodes = this._filterRadioButtons(nodes);

      for (let j = 0; j < nodes.length; j++) {
        let node = nodes[j];
        if (!this._isNavigableElement(node) || !this._isVisibleElement(node))
          continue;

        elements.push(node);
      }
    }
    this._elements = this._filterEditables(elements);

    function orderByTabIndex(a, b) {
      
      
      
      if (a.tabIndex == 0 || b.tabIndex == 0)
        return b.tabIndex;

      return a.tabIndex > b.tabIndex;
    }
    this._elements = this._elements.sort(orderByTabIndex);

    
    let currentIndex = this._getIndexForElement(aElement);
    return currentIndex;
  },

  _getIndexForElement: function(aElement) {
    let currentIndex = -1;
    let elements = this._elements;
    for (let i = 0; i < elements.length; i++) {
      if (elements[i] == aElement)
        return i;
    }
    return -1;
  },

  _getJSON: function() {
    let element = this.currentElement;
    let list = getListForElement(element);
    let labels = this._getLabels();
    return {
      current: {
        id: element.id,
        name: element.name,
        title: labels.length ? labels[0].title : "",
        value: element.value,
        maxLength: element.maxLength,
        type: (element.getAttribute("type") || "").toLowerCase(),
        choices: list,
        isAutocomplete: this._isAutocomplete(this.currentElement),
        rect: this._getRect(),
        caretRect: this._getCaretRect()
      },
      hasPrevious: !!this._elements[this._currentIndex - 1],
      hasNext: !!this._elements[this._currentIndex + 1]
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
};


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

  select: function(aIndex, aSelected, aClearAll) {
    let options = this._control.options;
    options[aIndex].selected = aSelected;

    if (aClearAll) {
      for (let i = 0; i < options.length; i++) {
        if (i != aIndex)
          options.item(i).selected = false;
      }
    }
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

  select: function(aIndex, aSelected, aClearAll) {
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

