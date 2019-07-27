





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

XPCOMUtils.defineLazyServiceGetter(Services, "threadManager",
                                   "@mozilla.org/thread-manager;1",
                                   "nsIThreadManager");

XPCOMUtils.defineLazyGetter(this, "domWindowUtils", function () {
  return content.QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIDOMWindowUtils);
});

const RESIZE_SCROLL_DELAY = 20;





const MAX_BLOCKED_COUNT = 20;

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
    addMessageListener("Forms:SetComposition", this);
    addMessageListener("Forms:EndComposition", this);
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
  _selectionPrivate: null,

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
      this.focusedElement.removeEventListener('compositionend', this);
      if (this._observer) {
        this._observer.disconnect();
        this._observer = null;
      }
      if (this._selectionPrivate) {
        this._selectionPrivate.removeSelectionListener(this);
        this._selectionPrivate = null;
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
      element.addEventListener('compositionend', this);
      if (isContentEditable(element)) {
        this._documentEncoder = getDocumentEncoder(element);
      }
      this._editor = getPlaintextEditor(element);
      if (this._editor) {
        
        
        this._editor.addEditorObserver(this);

        let selection = this._editor.selection;
        if (selection) {
          this._selectionPrivate = selection.QueryInterface(Ci.nsISelectionPrivate);
          this._selectionPrivate.addSelectionListener(this);
        }
      }

      
      let MutationObserver = element.ownerDocument.defaultView.MutationObserver;
      this._observer = new MutationObserver(function(mutations) {
        var del = [].some.call(mutations, function(m) {
          return [].some.call(m.removedNodes, function(n) {
            return n.contains(element);
          });
        });
        if (del && element === self.focusedElement) {
          self.hideKeyboard();
          self.selectionStart = -1;
          self.selectionEnd = -1;
        }
      });

      this._observer.observe(element.ownerDocument.body, {
        childList: true,
        subtree: true
      });
    }

    this.focusedElement = element;
  },

  notifySelectionChanged: function(aDocument, aSelection, aReason) {
    this.updateSelection();
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

        
        if (target instanceof HTMLHtmlElement) {
          target = target.document.body;
        } else if (target instanceof HTMLDocument) {
          target = target.body;
        } else if (target instanceof HTMLIFrameElement) {
          target = target.contentDocument ? target.contentDocument.body
                                          : null;
        }

        if (!target) {
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
        
      case "submit":
        if (this.focusedElement) {
          this.focusedElement.blur();
        }
        break;

      case "blur":
        if (this.focusedElement) {
          this.hideKeyboard();
          this.selectionStart = -1;
          this.selectionEnd = -1;
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
              scrollSelectionOrElementIntoView(this.focusedElement);
            }
          }.bind(this), RESIZE_SCROLL_DELAY);
        }
        break;

      case "keydown":
        if (!this.focusedElement) {
          break;
        }

        CompositionManager.endComposition('');
        break;

      case "keyup":
        if (!this.focusedElement) {
          break;
        }

        CompositionManager.endComposition('');
        break;

      case "compositionend":
        if (!this.focusedElement) {
          break;
        }

        CompositionManager.onCompositionEnd();
        break;
    }
  },

  waitForNextTick: function(callback) {
    var tm = Services.threadManager;
    tm.mainThread.dispatch({
      run: callback,
    }, Components.interfaces.nsIThread.DISPATCH_NORMAL);
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
        CompositionManager.endComposition('');

        target.value = json.value;

        let event = target.ownerDocument.createEvent('HTMLEvents');
        event.initEvent('input', true, false);
        target.dispatchEvent(event);
        break;
      }

      case "Forms:Input:SendKey":
        CompositionManager.endComposition('');

        let flags = domWindowUtils.KEY_FLAG_NOT_SYNTHESIZED_FOR_TESTS;
        this._editing = true;
        let doKeypress = domWindowUtils.sendKeyEvent('keydown', json.keyCode,
                                                     json.charCode, json.modifiers, flags);
        if (doKeypress) {
          domWindowUtils.sendKeyEvent('keypress', json.keyCode,
                                      json.charCode, json.modifiers, flags);
        }

        if(!json.repeat) {
          domWindowUtils.sendKeyEvent('keyup', json.keyCode,
                                      json.charCode, json.modifiers, flags);
        }

        this._editing = false;

        if (json.requestId && doKeypress) {
          sendAsyncMessage("Forms:SendKey:Result:OK", {
            requestId: json.requestId,
            selectioninfo: this.getSelectionInfo()
          });
        }
        else if (json.requestId && !doKeypress) {
          sendAsyncMessage("Forms:SendKey:Result:Error", {
            requestId: json.requestId,
            error: "Keydown event got canceled"
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
        if (this.focusedElement) {
          this.focusedElement.blur();
        }

        break;
      }

      case "Forms:SetSelectionRange":  {
        CompositionManager.endComposition('');

        let start = json.selectionStart;
        let end =  json.selectionEnd;

        if (!setSelectionRange(target, start, end)) {
          if (json.requestId) {
            sendAsyncMessage("Forms:SetSelectionRange:Result:Error", {
              requestId: json.requestId,
              error: "failed"
            });
          }
          break;
        }

        if (json.requestId) {
          sendAsyncMessage("Forms:SetSelectionRange:Result:OK", {
            requestId: json.requestId,
            selectioninfo: this.getSelectionInfo()
          });
        }
        break;
      }

      case "Forms:ReplaceSurroundingText": {
        CompositionManager.endComposition('');

        let selectionRange = getSelectionRange(target);
        if (!replaceSurroundingText(target,
                                    json.text,
                                    selectionRange[0],
                                    selectionRange[1],
                                    json.offset,
                                    json.length)) {
          if (json.requestId) {
            sendAsyncMessage("Forms:ReplaceSurroundingText:Result:Error", {
              requestId: json.requestId,
              error: "failed"
            });
          }
          break;
        }

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

      case "Forms:SetComposition": {
        CompositionManager.setComposition(target, json.text, json.cursor,
                                          json.clauses);
        sendAsyncMessage("Forms:SetComposition:Result:OK", {
          requestId: json.requestId,
          selectioninfo: this.getSelectionInfo()
        });
        break;
      }

      case "Forms:EndComposition": {
        CompositionManager.endComposition(json.text);
        sendAsyncMessage("Forms:EndComposition:Result:OK", {
          requestId: json.requestId,
          selectioninfo: this.getSelectionInfo()
        });
        break;
      }
    }
    this._editing = false;

  },

  showKeyboard: function fa_showKeyboard(target) {
    if (this.focusedElement === target)
      return;

    if (target instanceof HTMLOptionElement)
      target = target.parentNode;

    this.setFocusedElement(target);

    let count = this._focusCounter;
    this.waitForNextTick(function fa_showKeyboardSync() {
      if (count !== this._focusCounter) {
        return;
      }

      let kbOpened = this.sendKeyboardState(target);
      if (this.isTextInputElement(target))
        this.isKeyboardOpened = kbOpened;
    }.bind(this));
  },

  hideKeyboard: function fa_hideKeyboard() {
    this.setFocusedElement(null);

    let count = this._focusCounter;

    
    
    
    this.waitForNextTick(function fa_hideKeyboardSync() {
      if (count !== this._focusCounter ||
          !this.isKeyboardOpened) {
        return;
      }

      this.isKeyboardOpened = false;
      sendAsyncMessage("Forms:Input", { "type": "blur" });
    }.bind(this));
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
      while (element && !isContentEditable(element))
        element = element.parentNode;

      return element;
    }

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

  _selectionTimeout: null,

  
  updateSelection: function fa_updateSelection() {
    
    
    
    
    
    
    
    if (this._selectionTimeout) {
      content.clearTimeout(this._selectionTimeout);
    }
    this._selectionTimeout = content.setTimeout(function() {
      if (!this.focusedElement) {
        return;
      }
      let selectionInfo = this.getSelectionInfo();
      if (selectionInfo.changed) {
        sendAsyncMessage("Forms:SelectionChange", selectionInfo);
      }
    }.bind(this), 0);
  }
};

FormAssistant.init();

function isContentEditable(element) {
  if (!element) {
    return false;
  }

  if (element.isContentEditable || element.designMode == "on")
    return true;

  return element.ownerDocument && element.ownerDocument.designMode == "on";
}

function isPlainTextField(element) {
  if (!element) {
    return false;
  }

  return element instanceof HTMLTextAreaElement ||
         (element instanceof HTMLInputElement &&
          element.mozIsTextField(false));
}

function getJSON(element, focusCounter) {
  
  
  
  
  element = element.ownerNumberControl || element;

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
              Ci.nsIDocumentEncoder.OutputDropInvisibleBreak |
              
              Ci.nsIDocumentEncoder.OutputDontRemoveLineEndingSpaces |
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
    if (sel && sel.rangeCount > 0) {
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
    
    
    return false;
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
    return true;
  } else {
    
    let win = element.ownerDocument.defaultView;
    let sel = win.getSelection();

    
    sel.collapse(element, 0);
    for (let i = 0; i < start; i++) {
      sel.modify("move", "forward", "character");
    }

    
    
    let oldStart = getContentEditableSelectionStart(element, sel);
    let counter = 0;
    while (oldStart < start) {
      sel.modify("move", "forward", "character");
      let newStart = getContentEditableSelectionStart(element, sel);
      if (oldStart == newStart) {
        counter++;
        if (counter > MAX_BLOCKED_COUNT) {
          return false;
        }
      } else {
        counter = 0;
        oldStart = newStart;
      }
    }

    
    for (let i = start; i < end; i++) {
      sel.modify("extend", "forward", "character");
    }

    
    
    counter = 0;
    let selectionLength = end - start;
    let oldSelectionLength = getContentEditableSelectionLength(element, sel);
    while (oldSelectionLength  < selectionLength) {
      sel.modify("extend", "forward", "character");
      let newSelectionLength = getContentEditableSelectionLength(element, sel);
      if (oldSelectionLength == newSelectionLength ) {
        counter++;
        if (counter > MAX_BLOCKED_COUNT) {
          return false;
        }
      } else {
        counter = 0;
        oldSelectionLength = newSelectionLength;
      }
    }
    return true;
  }
}






function scrollSelectionOrElementIntoView(element) {
  let editor = getPlaintextEditor(element);
  if (editor) {
    editor.selectionController.scrollSelectionIntoView(
      Ci.nsISelectionController.SELECTION_NORMAL,
      Ci.nsISelectionController.SELECTION_FOCUS_REGION,
      Ci.nsISelectionController.SCROLL_SYNCHRONOUS);
  } else {
      element.scrollIntoView(false);
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

function replaceSurroundingText(element, text, selectionStart, selectionEnd,
                                offset, length) {
  let editor = FormAssistant.editor;
  if (!editor) {
    return false;
  }

  
  let start = selectionStart + offset;
  if (start < 0) {
    start = 0;
  }
  if (length < 0) {
    length = 0;
  }
  let end = start + length;

  if (selectionStart != start || selectionEnd != end) {
    
    if (!setSelectionRange(element, start, end)) {
      return false;
    }
  }

  if (start != end) {
    
    editor.deleteSelection(Ci.nsIEditor.ePrevious, Ci.nsIEditor.eStrip);
  }

  if (text) {
    
    
    text = text.replace(/\r/g, '\n');
    
    editor.insertText(text);
  }
  return true;
}

let CompositionManager =  {
  _isStarted: false,
  _text: '',
  _clauseAttrMap: {
    'raw-input':
      Ci.nsICompositionStringSynthesizer.ATTR_RAWINPUT,
    'selected-raw-text':
      Ci.nsICompositionStringSynthesizer.ATTR_SELECTEDRAWTEXT,
    'converted-text':
      Ci.nsICompositionStringSynthesizer.ATTR_CONVERTEDTEXT,
    'selected-converted-text':
      Ci.nsICompositionStringSynthesizer.ATTR_SELECTEDCONVERTEDTEXT
  },

  setComposition: function cm_setComposition(element, text, cursor, clauses) {
    
    if (!element) {
      return;
    }
    let len = text.length;
    if (cursor > len) {
      cursor = len;
    }
    let clauseLens = [];
    let clauseAttrs = [];
    if (clauses) {
      let remainingLength = len;
      for (let i = 0; i < clauses.length; i++) {
        if (clauses[i]) {
          let clauseLength = clauses[i].length || 0;
          
          
          if (clauseLength > remainingLength) {
            clauseLength = remainingLength;
          }
          remainingLength -= clauseLength;
          clauseLens.push(clauseLength);
          clauseAttrs.push(this._clauseAttrMap[clauses[i].selectionType] ||
                           Ci.nsICompositionStringSynthesizer.ATTR_RAWINPUT);
        }
      }
      
      
      if (remainingLength > 0) {
        clauseLens[clauseLens.length - 1] += remainingLength;
      }
    } else {
      clauseLens.push(len);
      clauseAttrs.push(Ci.nsICompositionStringSynthesizer.ATTR_RAWINPUT);
    }

    
    if (!this._isStarted) {
      this._isStarted = true;
      domWindowUtils.sendCompositionEvent('compositionstart', '', '');
      this._text = '';
    }

    
    if (this._text !== text) {
      this._text = text;
      domWindowUtils.sendCompositionEvent('compositionupdate', text, '');
    }
    let compositionString = domWindowUtils.createCompositionStringSynthesizer();
    compositionString.setString(text);
    for (var i = 0; i < clauseLens.length; i++) {
      compositionString.appendClause(clauseLens[i], clauseAttrs[i]);
    }
    if (cursor >= 0) {
      compositionString.setCaret(cursor, 0);
    }
    compositionString.dispatchEvent();
  },

  endComposition: function cm_endComposition(text) {
    if (!this._isStarted) {
      return;
    }
    
    if (this._text !== text) {
      domWindowUtils.sendCompositionEvent('compositionupdate', text, '');
    }
    let compositionString = domWindowUtils.createCompositionStringSynthesizer();
    compositionString.setString(text);
    
    
    compositionString.setCaret(text.length, 0);
    compositionString.dispatchEvent();
    domWindowUtils.sendCompositionEvent('compositionend', text, '');
    this._text = '';
    this._isStarted = false;
  },

  
  onCompositionEnd: function cm_onCompositionEnd() {
    if (!this._isStarted) {
      return;
    }

    this._text = '';
    this._isStarted = false;
  }
};
