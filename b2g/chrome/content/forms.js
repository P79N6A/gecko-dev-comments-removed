





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
    addEventListener("beforeunload", this, true, false);
    addEventListener("input", this, true, false);
    addEventListener("keydown", this, true, false);
    addEventListener("keyup", this, true, false);
    addMessageListener("Forms:Select:Choice", this);
    addMessageListener("Forms:Input:Value", this);
    addMessageListener("Forms:Select:Blur", this);
    addMessageListener("Forms:SetSelectionRange", this);
    addMessageListener("Forms:ReplaceSurroundingText", this);
    addMessageListener("Forms:GetText", this);
    addMessageListener("Forms:Input:SendKey", this);
    addMessageListener("Forms:GetContext", this);
  },

  ignoredInputTypes: new Set([
    'button', 'file', 'checkbox', 'radio', 'reset', 'submit', 'image',
    'range'
  ]),

  isKeyboardOpened: false,
  selectionStart: -1,
  selectionEnd: -1,
  textBeforeCursor: "",
  textAfterCursor: "",
  scrollIntoViewTimeout: null,
  _focusedElement: null,
  _focusCounter: 0, 
  _observer: null,
  _documentEncoder: null,
  _editor: null,
  _editing: false,
  _ignoreEditActionOnce: false,

  get focusedElement() {
    if (this._focusedElement && Cu.isDeadWrapper(this._focusedElement))
      this._focusedElement = null;

    return this._focusedElement;
  },

  set focusedElement(val) {
    this._focusCounter++;
    this._focusedElement = val;
  },

  setFocusedElement: function fa_setFocusedElement(element) {
    let self = this;

    if (element === this.focusedElement)
      return;

    if (this.focusedElement) {
      this.focusedElement.removeEventListener('mousedown', this);
      this.focusedElement.removeEventListener('mouseup', this);
      if (this._observer) {
        this._observer.disconnect();
        this._observer = null;
      }
      if (!element) {
        this.focusedElement.blur();
      }
    }

    this._documentEncoder = null;
    if (this._editor) {
      
      
      
      
      try {
        this._editor.removeEditorObserver(this);
      } catch (e) {}
      this._editor = null;
    }

    if (element) {
      element.addEventListener('mousedown', this);
      element.addEventListener('mouseup', this);
      if (isContentEditable(element)) {
        this._documentEncoder = getDocumentEncoder(element);
      }
      this._editor = getPlaintextEditor(element);
      if (this._editor) {
        
        
        this._editor.addEditorObserver(this);
      }

      
      let MutationObserver = element.ownerDocument.defaultView.MutationObserver;
      this._observer = new MutationObserver(function(mutations) {
        var del = [].some.call(mutations, function(m) {
          return [].some.call(m.removedNodes, function(n) {
            return n === element;
          });
        });
        if (del && element === this.focusedElement) {
          
          self.handleEvent({ target: element, type: "blur" });
        }
      });

      this._observer.observe(element.parentNode, {
        childList: true
      });
    }

    this.focusedElement = element;
  },

  get documentEncoder() {
    return this._documentEncoder;
  },

  
  get editor() {
    return this._editor;
  },

  
  
  EditAction: function fa_editAction() {
    if (this._editing) {
      return;
    } else if (this._ignoreEditActionOnce) {
      this._ignoreEditActionOnce = false;
      return;
    }
    this.sendKeyboardState(this.focusedElement);
  },

  handleEvent: function fa_handleEvent(evt) {
    let target = evt.target;

    let range = null;
    switch (evt.type) {
      case "focus":
        if (!target) {
          break;
        }

        if (target instanceof HTMLDocument ||
            
            target instanceof HTMLBodyElement ||
            target == content) {
          break;
        }

        if (isContentEditable(target)) {
          this.showKeyboard(this.getTopLevelEditable(target));
          this.updateSelection();
          break;
        }

        if (this.isFocusableElement(target)) {
          this.showKeyboard(target);
          this.updateSelection();
        }
        break;

      case "pagehide":
      case "beforeunload":
        
        
        if (target && target != content.document) {
          break;
        }
        
      case "blur":
      case "submit":
        if (this.focusedElement) {
          this.hideKeyboard();
          this.selectionStart = -1;
          this.selectionEnd = -1;
        }
        break;

      case 'mousedown':
         if (!this.focusedElement) {
          break;
        }

        
        
        this.updateSelection();
        break;

      case 'mouseup':
        if (!this.focusedElement) {
          break;
        }

        
        
        
        
        range = getSelectionRange(this.focusedElement);
        if (range[0] !== this.selectionStart ||
            range[1] !== this.selectionEnd) {
          this.sendKeyboardState(this.focusedElement);
          this.updateSelection();
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

      case "input":
        if (this.focusedElement) {
          
          this.updateSelection();
        }
        break;

      case "keydown":
        if (!this.focusedElement) {
          break;
        }

        
        this._ignoreEditActionOnce = true;

        
        
        content.setTimeout(function() {
          this.updateSelection();
        }.bind(this), 0);
        break;

      case "keyup":
        if (!this.focusedElement) {
          break;
        }

        this._ignoreEditActionOnce = false;
        break;
    }
  },

  receiveMessage: function fa_receiveMessage(msg) {
    let target = this.focusedElement;
    let json = msg.json;

    
    if ('contextId' in json &&
        json.contextId !== this._focusCounter &&
        json.requestId) {
      
      sendAsyncMessage("Forms:SequenceError", {
        requestId: json.requestId,
        error: "Expected contextId " + this._focusCounter +
               " but was " + json.contextId
      });
      return;
    }

    if (!target) {
      switch (msg.name) {
      case "Forms:GetText":
        sendAsyncMessage("Forms:GetText:Result:Error", {
          requestId: json.requestId,
          error: "No focused element"
        });
        break;
      }
      return;
    }

    this._editing = true;
    switch (msg.name) {
      case "Forms:Input:Value": {
        target.value = json.value;

        let event = target.ownerDocument.createEvent('HTMLEvents');
        event.initEvent('input', true, false);
        target.dispatchEvent(event);
        break;
      }

      case "Forms:Input:SendKey":
        ["keydown", "keypress", "keyup"].forEach(function(type) {
          domWindowUtils.sendKeyEvent(type, json.keyCode, json.charCode,
            json.modifiers);
        });

        if (json.requestId) {
          sendAsyncMessage("Forms:SendKey:Result:OK", {
            requestId: json.requestId
          });
        }
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
          let event = target.ownerDocument.createEvent('HTMLEvents');
          event.initEvent('change', true, true);
          target.dispatchEvent(event);
        }
        break;

      case "Forms:Select:Blur": {
        this.setFocusedElement(null);
        break;
      }

      case "Forms:SetSelectionRange":  {
        let start = json.selectionStart;
        let end =  json.selectionEnd;
        setSelectionRange(target, start, end);
        this.updateSelection();

        if (json.requestId) {
          sendAsyncMessage("Forms:SetSelectionRange:Result:OK", {
            requestId: json.requestId,
            selectioninfo: this.getSelectionInfo()
          });
        }
        break;
      }

      case "Forms:ReplaceSurroundingText": {
        let text = json.text;
        let beforeLength = json.beforeLength;
        let afterLength = json.afterLength;
        let selectionRange = getSelectionRange(target);

        replaceSurroundingText(target, text, selectionRange[0], beforeLength,
                               afterLength);

        if (json.requestId) {
          sendAsyncMessage("Forms:ReplaceSurroundingText:Result:OK", {
            requestId: json.requestId,
            selectioninfo: this.getSelectionInfo()
          });
        }
        break;
      }

      case "Forms:GetText": {
        let value = isContentEditable(target) ? getContentEditableText(target)
                                              : target.value;

        if (json.offset && json.length) {
          value = value.substr(json.offset, json.length);
        }
        else if (json.offset) {
          value = value.substr(json.offset);
        }

        sendAsyncMessage("Forms:GetText:Result:OK", {
          requestId: json.requestId,
          text: value
        });
        break;
      }

      case "Forms:GetContext": {
        let obj = getJSON(target, this._focusCounter);
        sendAsyncMessage("Forms:GetContext:Result:OK", obj);
        break;
      }
    }
    this._editing = false;

  },

  showKeyboard: function fa_showKeyboard(target) {
    if (this.isKeyboardOpened)
      return;

    if (target instanceof HTMLOptionElement)
      target = target.parentNode;

    this.setFocusedElement(target);

    let kbOpened = this.sendKeyboardState(target);
    if (this.isTextInputElement(target))
      this.isKeyboardOpened = kbOpened;
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

    sendAsyncMessage("Forms:Input", getJSON(element, this._focusCounter));
    return true;
  },

  getSelectionInfo: function fa_getSelectionInfo() {
    let element = this.focusedElement;
    let range =  getSelectionRange(element);

    let text = isContentEditable(element) ? getContentEditableText(element)
                                          : element.value;

    let textAround = getTextAroundCursor(text, range);

    let changed = this.selectionStart !== range[0] ||
      this.selectionEnd !== range[1] ||
      this.textBeforeCursor !== textAround.before ||
      this.textAfterCursor !== textAround.after;

    this.selectionStart = range[0];
    this.selectionEnd = range[1];
    this.textBeforeCursor = textAround.before;
    this.textAfterCursor = textAround.after;

    return {
      selectionStart: range[0],
      selectionEnd: range[1],
      textBeforeCursor: textAround.before,
      textAfterCursor: textAround.after,
      changed: changed
    };
  },

  
  updateSelection: function fa_updateSelection() {
    if (!this.focusedElement) {
      return;
    }
    let selectionInfo = this.getSelectionInfo();
    if (selectionInfo.changed) {
      sendAsyncMessage("Forms:SelectionChange", this.getSelectionInfo());
    }
  }
};

FormAssistant.init();

function isContentEditable(element) {
  if (!element) {
    return false;
  }

  if (element.isContentEditable || element.designMode == "on")
    return true;

  
  
  if (element instanceof HTMLIFrameElement &&
      element.contentDocument &&
      (element.contentDocument.body.isContentEditable ||
       element.contentDocument.designMode == "on"))
    return true;

  return element.ownerDocument && element.ownerDocument.designMode == "on";
}

function isPlainTextField(element) {
  if (!element) {
    return false;
  }

  return element instanceof HTMLInputElement ||
         element instanceof HTMLTextAreaElement;
}

function getJSON(element, focusCounter) {
  let type = element.type || "";
  let value = element.value || "";
  let max = element.max || "";
  let min = element.min || "";

  
  if (isContentEditable(element)) {
    type = "textarea";
    value = getContentEditableText(element);
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

  let range = getSelectionRange(element);
  let textAround = getTextAroundCursor(value, range);

  return {
    "contextId": focusCounter,

    "type": type.toLowerCase(),
    "choices": getListForElement(element),
    "value": value,
    "inputmode": inputmode,
    "selectionStart": range[0],
    "selectionEnd": range[1],
    "max": max,
    "min": min,
    "lang": element.lang || "",
    "textBeforeCursor": textAround.before,
    "textAfterCursor": textAround.after
  };
}

function getTextAroundCursor(value, range) {
  let textBeforeCursor = range[0] < 100 ?
    value.substr(0, range[0]) :
    value.substr(range[0] - 100, 100);

  let textAfterCursor = range[1] + 100 > value.length ?
    value.substr(range[0], value.length) :
    value.substr(range[0], range[1] - range[0] + 100);

  return {
    before: textBeforeCursor,
    after: textAfterCursor
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


function getDocumentEncoder(element) {
  let encoder = Cc["@mozilla.org/layout/documentEncoder;1?type=text/plain"]
                .createInstance(Ci.nsIDocumentEncoder);
  let flags = Ci.nsIDocumentEncoder.SkipInvisibleContent |
              Ci.nsIDocumentEncoder.OutputRaw |
              Ci.nsIDocumentEncoder.OutputLFLineBreak |
              Ci.nsIDocumentEncoder.OutputNonTextContentAsPlaceholder;
  encoder.init(element.ownerDocument, "text/plain", flags);
  return encoder;
}


function getContentEditableText(element) {
  if (!element || !isContentEditable(element)) {
    return null;
  }

  let doc = element.ownerDocument;
  let range = doc.createRange();
  range.selectNodeContents(element);
  let encoder = FormAssistant.documentEncoder;
  encoder.setRange(range);
  return encoder.encodeToString();
}

function getSelectionRange(element) {
  let start = 0;
  let end = 0;
  if (isPlainTextField(element)) {
    
    start = element.selectionStart;
    end = element.selectionEnd;
  } else if (isContentEditable(element)){
    
    let win = element.ownerDocument.defaultView;
    let sel = win.getSelection();
    if (sel) {
      start = getContentEditableSelectionStart(element, sel);
      end = start + getContentEditableSelectionLength(element, sel);
    } else {
      dump("Failed to get window.getSelection()\n");
    }
   }
   return [start, end];
 }

function getContentEditableSelectionStart(element, selection) {
  let doc = element.ownerDocument;
  let range = doc.createRange();
  range.setStart(element, 0);
  range.setEnd(selection.anchorNode, selection.anchorOffset);
  let encoder = FormAssistant.documentEncoder;
  encoder.setRange(range);
  return encoder.encodeToString().length;
}

function getContentEditableSelectionLength(element, selection) {
  let encoder = FormAssistant.documentEncoder;
  encoder.setRange(selection.getRangeAt(0));
  return encoder.encodeToString().length;
}

function setSelectionRange(element, start, end) {
  let isTextField = isPlainTextField(element);

  

  if (!isTextField && !isContentEditable(element)) {
    
    
    return;
  }

  let text = isTextField ? element.value : getContentEditableText(element);
  let length = text.length;
  if (start < 0) {
    start = 0;
  }
  if (end > length) {
    end = length;
  }
  if (start > end) {
    start = end;
  }

  if (isTextField) {
    
    element.setSelectionRange(start, end, "forward");
  } else {
    
    let win = element.ownerDocument.defaultView;
    let sel = win.getSelection();

    
    sel.collapse(element, 0);
    for (let i = 0; i < start; i++) {
      sel.modify("move", "forward", "character");
    }

    while (getContentEditableSelectionStart(element, sel) < start) {
      sel.modify("move", "forward", "character");
    }

    
    for (let i = start; i < end; i++) {
      sel.modify("extend", "forward", "character");
    }

    let selectionLength = end - start;
    while (getContentEditableSelectionLength(element, sel) < selectionLength) {
      sel.modify("extend", "forward", "character");
    }
  }
}


function getPlaintextEditor(element) {
  let editor = null;
  
  if (isPlainTextField(element)) {
    
    editor = element.QueryInterface(Ci.nsIDOMNSEditableElement).editor;
  } else if (isContentEditable(element)) {
    
    let win = element.ownerDocument.defaultView;
    let editingSession = win.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIWebNavigation)
                            .QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIEditingSession);
    if (editingSession) {
      editor = editingSession.getEditorForWindow(win);
    }
  }
  if (editor) {
    editor.QueryInterface(Ci.nsIPlaintextEditor);
  }
  return editor;
}

function replaceSurroundingText(element, text, selectionStart, beforeLength,
                                afterLength) {
  let editor = FormAssistant.editor;
  if (!editor) {
    return;
  }

  
  if (beforeLength < 0) {
    beforeLength = 0;
  }
  if (afterLength < 0) {
    afterLength = 0;
  }

  let start = selectionStart - beforeLength;
  let end = selectionStart + afterLength;

  if (beforeLength != 0 || afterLength != 0) {
    
    setSelectionRange(element, start, end);
  }

  if (start != end) {
    
    editor.deleteSelection(Ci.nsIEditor.ePrevious, Ci.nsIEditor.eStrip);
  }

  if (text) {
    
    
    text = text.replace(/\r/g, '\n');
    
    editor.insertText(text);
  }
}
