























"use strict";

const {Ci, Cu, Cc} = require("chrome");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const CONTENT_TYPES = {
  PLAIN_TEXT: 0,
  CSS_VALUE: 1,
  CSS_MIXED: 2,
  CSS_PROPERTY: 3,
};
const MAX_POPUP_ENTRIES = 10;

const FOCUS_FORWARD = Ci.nsIFocusManager.MOVEFOCUS_FORWARD;
const FOCUS_BACKWARD = Ci.nsIFocusManager.MOVEFOCUS_BACKWARD;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");











































function editableField(aOptions)
{
  return editableItem(aOptions, function(aElement, aEvent) {
    if (!aOptions.element.inplaceEditor) {
      new InplaceEditor(aOptions, aEvent);
    }
  });
}

exports.editableField = editableField;















function editableItem(aOptions, aCallback)
{
  let trigger = aOptions.trigger || "click"
  let element = aOptions.element;
  element.addEventListener(trigger, function(evt) {
    if (evt.target.nodeName !== "a") {
      let win = this.ownerDocument.defaultView;
      let selection = win.getSelection();
      if (trigger != "click" || selection.isCollapsed) {
        aCallback(element, evt);
      }
      evt.stopPropagation();
    }
  }, false);

  
  
  element.addEventListener("keypress", function(evt) {
    if (evt.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN ||
        evt.charCode === Ci.nsIDOMKeyEvent.DOM_VK_SPACE) {
      aCallback(element);
    }
  }, true);

  
  
  
  
  element.addEventListener("mousedown", function(evt) {
    if (evt.target.nodeName !== "a") {
      let cleanup = function() {
        element.style.removeProperty("outline-style");
        element.removeEventListener("mouseup", cleanup, false);
        element.removeEventListener("mouseout", cleanup, false);
      };
      element.style.setProperty("outline-style", "none");
      element.addEventListener("mouseup", cleanup, false);
      element.addEventListener("mouseout", cleanup, false);
    }
  }, false);

  
  
  element._editable = true;

  
  element._trigger = trigger;

  return function turnOnEditMode() {
    aCallback(element);
  }
}

exports.editableItem = this.editableItem;








function getInplaceEditorForSpan(aSpan)
{
  return aSpan.inplaceEditor;
};
exports.getInplaceEditorForSpan = getInplaceEditorForSpan;

function InplaceEditor(aOptions, aEvent)
{
  this.elt = aOptions.element;
  let doc = this.elt.ownerDocument;
  this.doc = doc;
  this.elt.inplaceEditor = this;

  this.change = aOptions.change;
  this.done = aOptions.done;
  this.destroy = aOptions.destroy;
  this.initial = aOptions.initial ? aOptions.initial : this.elt.textContent;
  this.multiline = aOptions.multiline || false;
  this.stopOnShiftTab = !!aOptions.stopOnShiftTab;
  this.stopOnTab = !!aOptions.stopOnTab;
  this.stopOnReturn = !!aOptions.stopOnReturn;
  this.contentType = aOptions.contentType || CONTENT_TYPES.PLAIN_TEXT;
  this.property = aOptions.property;
  this.popup = aOptions.popup;

  this._onBlur = this._onBlur.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
  this._onInput = this._onInput.bind(this);
  this._onKeyup = this._onKeyup.bind(this);

  this._createInput();
  this._autosize();
  this.inputCharWidth = this._getInputCharWidth();

  
  
  this._advanceCharCodes = {};
  let advanceChars = aOptions.advanceChars || '';
  for (let i = 0; i < advanceChars.length; i++) {
    this._advanceCharCodes[advanceChars.charCodeAt(i)] = true;
  }

  
  this.originalDisplay = this.elt.style.display;
  this.elt.style.display = "none";
  this.elt.parentNode.insertBefore(this.input, this.elt);

  if (typeof(aOptions.selectAll) == "undefined" || aOptions.selectAll) {
    this.input.select();
  }
  this.input.focus();

  if (this.contentType == CONTENT_TYPES.CSS_VALUE && this.input.value == "") {
    this._maybeSuggestCompletion(true);
  }

  this.input.addEventListener("blur", this._onBlur, false);
  this.input.addEventListener("keypress", this._onKeyPress, false);
  this.input.addEventListener("input", this._onInput, false);

  this.input.addEventListener("dblclick",
    (e) => { e.stopPropagation(); }, false);
  this.input.addEventListener("mousedown",
    (e) => { e.stopPropagation(); }, false);

  this.validate = aOptions.validate;

  if (this.validate) {
    this.input.addEventListener("keyup", this._onKeyup, false);
  }

  if (aOptions.start) {
    aOptions.start(this, aEvent);
  }

  EventEmitter.decorate(this);
}

exports.InplaceEditor = InplaceEditor;

InplaceEditor.CONTENT_TYPES = CONTENT_TYPES;

InplaceEditor.prototype = {
  _createInput: function InplaceEditor_createEditor()
  {
    this.input =
      this.doc.createElementNS(HTML_NS, this.multiline ? "textarea" : "input");
    this.input.inplaceEditor = this;
    this.input.classList.add("styleinspector-propertyeditor");
    this.input.value = this.initial;

    copyTextStyles(this.elt, this.input);
  },

  


  _clear: function InplaceEditor_clear()
  {
    if (!this.input) {
      
      return;
    }

    this.input.removeEventListener("blur", this._onBlur, false);
    this.input.removeEventListener("keypress", this._onKeyPress, false);
    this.input.removeEventListener("keyup", this._onKeyup, false);
    this.input.removeEventListener("oninput", this._onInput, false);
    this._stopAutosize();

    this.elt.style.display = this.originalDisplay;
    this.elt.focus();

    this.input.remove();
    this.input = null;

    delete this.elt.inplaceEditor;
    delete this.elt;

    if (this.destroy) {
      this.destroy();
    }
  },

  



  _autosize: function InplaceEditor_autosize()
  {
    
    

    
    
    
    
    this._measurement =
      this.doc.createElementNS(HTML_NS, this.multiline ? "pre" : "span");
    this._measurement.className = "autosizer";
    this.elt.parentNode.appendChild(this._measurement);
    let style = this._measurement.style;
    style.visibility = "hidden";
    style.position = "absolute";
    style.top = "0";
    style.left = "0";
    copyTextStyles(this.input, this._measurement);
    this._updateSize();
  },

  


  _stopAutosize: function InplaceEditor_stopAutosize()
  {
    if (!this._measurement) {
      return;
    }
    this._measurement.remove();
    delete this._measurement;
  },

  


  _updateSize: function InplaceEditor_updateSize()
  {
    
    
    
    this._measurement.textContent = this.input.value.replace(/ /g, '\u00a0');

    
    
    
    let width = this._measurement.offsetWidth + 10;

    if (this.multiline) {
      
      
      
      width += 15;
      this._measurement.textContent += "M";
      this.input.style.height = this._measurement.offsetHeight + "px";
    }

    this.input.style.width = width + "px";
  },

  



  _getInputCharWidth: function InplaceEditor_getInputCharWidth()
  {
    
    
    this._measurement.textContent = "x";
    return this._measurement.offsetWidth;
  },

   






  _incrementValue: function InplaceEditor_incrementValue(increment)
  {
    let value = this.input.value;
    let selectionStart = this.input.selectionStart;
    let selectionEnd = this.input.selectionEnd;

    let newValue = this._incrementCSSValue(value, increment, selectionStart,
                                           selectionEnd);

    if (!newValue) {
      return false;
    }

    this.input.value = newValue.value;
    this.input.setSelectionRange(newValue.start, newValue.end);
    this._doValidation();

    
    if (this.change) {
      this.change(this.input.value.trim());
    }

    return true;
  },

  












  _incrementCSSValue: function InplaceEditor_incrementCSSValue(value, increment,
                                                               selStart, selEnd)
  {
    let range = this._parseCSSValue(value, selStart);
    let type = (range && range.type) || "";
    let rawValue = (range ? value.substring(range.start, range.end) : "");
    let incrementedValue = null, selection;

    if (type === "num") {
      let newValue = this._incrementRawValue(rawValue, increment);
      if (newValue !== null) {
        incrementedValue = newValue;
        selection = [0, incrementedValue.length];
      }
    } else if (type === "hex") {
      let exprOffset = selStart - range.start;
      let exprOffsetEnd = selEnd - range.start;
      let newValue = this._incHexColor(rawValue, increment, exprOffset,
                                       exprOffsetEnd);
      if (newValue) {
        incrementedValue = newValue.value;
        selection = newValue.selection;
      }
    } else {
      let info;
      if (type === "rgb" || type === "hsl") {
        info = {};
        let part = value.substring(range.start, selStart).split(",").length - 1;
        if (part === 3) { 
          info.minValue = 0;
          info.maxValue = 1;
        } else if (type === "rgb") {
          info.minValue = 0;
          info.maxValue = 255;
        } else if (part !== 0) { 
          info.minValue = 0;
          info.maxValue = 100;

          
          
          if (value.charAt(selStart - 1) === "%") {
            --selStart;
          }
        }
      }
      return this._incrementGenericValue(value, increment, selStart, selEnd, info);
    }

    if (incrementedValue === null) {
      return;
    }

    let preRawValue = value.substr(0, range.start);
    let postRawValue = value.substr(range.end);

    return {
      value: preRawValue + incrementedValue + postRawValue,
      start: range.start + selection[0],
      end: range.start + selection[1]
    };
  },

  








   _parseCSSValue: function InplaceEditor_parseCSSValue(value, offset)
  {
    const reSplitCSS = /(url\("?[^"\)]+"?\)?)|(rgba?\([^)]*\)?)|(hsla?\([^)]*\)?)|(#[\dA-Fa-f]+)|(-?\d*\.?\d+(%|[a-z]{1,4})?)|"([^"]*)"?|'([^']*)'?|([^,\s\/!\(\)]+)|(!(.*)?)/;
    let start = 0;
    let m;

    
    while ((m = reSplitCSS.exec(value)) &&
          (m.index + m[0].length < offset)) {
      value = value.substr(m.index + m[0].length);
      start += m.index + m[0].length;
      offset -= m.index + m[0].length;
    }

    if (!m) {
      return;
    }

    let type;
    if (m[1]) {
      type = "url";
    } else if (m[2]) {
      type = "rgb";
    } else if (m[3]) {
      type = "hsl";
    } else if (m[4]) {
      type = "hex";
    } else if (m[5]) {
      type = "num";
    }

    return {
      value: m[0],
      start: start + m.index,
      end: start + m.index + m[0].length,
      type: type
    };
  },

  















  _incrementGenericValue:
  function InplaceEditor_incrementGenericValue(value, increment, offset,
                                               offsetEnd, info)
  {
    
    let start, end;
    
    if (/^-?[0-9.]/.test(value.substring(offset, offsetEnd)) &&
      !(/\d/.test(value.charAt(offset - 1) + value.charAt(offsetEnd)))) {
      
      
      
      start = offset;
      end = offsetEnd;
    } else {
      
      
      let pattern = "[" + (info ? "0-9." : "0-9") + "]*";
      let before = new RegExp(pattern + "$").exec(value.substr(0, offset))[0].length;
      let after = new RegExp("^" + pattern).exec(value.substr(offset))[0].length;

      start = offset - before;
      end = offset + after;

      
      
      if (value.charAt(start - 1) === "-" &&
         (start - 1 === 0 || /[ (:,='"]/.test(value.charAt(start - 2)))) {
        --start;
      }
    }

    if (start !== end)
    {
      
      
      if (value.charAt(end) === "%") {
        ++end;
      }

      let first = value.substr(0, start);
      let mid = value.substring(start, end);
      let last = value.substr(end);

      mid = this._incrementRawValue(mid, increment, info);

      if (mid !== null) {
        return {
          value: first + mid + last,
          start: start,
          end: start + mid.length
        };
      }
    }
  },

  










  _incrementRawValue:
  function InplaceEditor_incrementRawValue(rawValue, increment, info)
  {
    let num = parseFloat(rawValue);

    if (isNaN(num)) {
      return null;
    }

    let number = /\d+(\.\d+)?/.exec(rawValue);
    let units = rawValue.substr(number.index + number[0].length);

    
    let newValue = Math.round((num + increment) * 1000) / 1000;

    if (info && "minValue" in info) {
      newValue = Math.max(newValue, info.minValue);
    }
    if (info && "maxValue" in info) {
      newValue = Math.min(newValue, info.maxValue);
    }

    newValue = newValue.toString();

    return newValue + units;
  },

  












  _incHexColor:
  function InplaceEditor_incHexColor(rawValue, increment, offset, offsetEnd)
  {
    
    if (offsetEnd > rawValue.length && offset >= rawValue.length) {
      return;
    }
    if (offset < 1 && offsetEnd <= 1) {
      return;
    }
    
    rawValue = rawValue.substr(1);
    --offset;
    --offsetEnd;

    
    offset = Math.max(offset, 0);
    offsetEnd = Math.min(offsetEnd, rawValue.length);
    offsetEnd = Math.max(offsetEnd, offset);

    
    if (rawValue.length === 3) {
      rawValue = rawValue.charAt(0) + rawValue.charAt(0) +
                 rawValue.charAt(1) + rawValue.charAt(1) +
                 rawValue.charAt(2) + rawValue.charAt(2);
      offset *= 2;
      offsetEnd *= 2;
    }

    if (rawValue.length !== 6) {
      return;
    }

    
    if (offset === offsetEnd) {
      if (offset === 0) {
        offsetEnd = 1;
      } else {
        offset = offsetEnd - 1;
      }
    }

    
    offset -= offset % 2;
    offsetEnd += offsetEnd % 2;

    
    if (-1 < increment && increment < 1) {
      increment = (increment < 0 ? -1 : 1);
    }
    if (Math.abs(increment) === 10) {
      increment = (increment < 0 ? -16 : 16);
    }

    let isUpper = (rawValue.toUpperCase() === rawValue);

    for (let pos = offset; pos < offsetEnd; pos += 2) {
      
      let mid = rawValue.substr(pos, 2);
      let value = parseInt(mid, 16);

      if (isNaN(value)) {
        return;
      }

      mid = Math.min(Math.max(value + increment, 0), 255).toString(16);

      while (mid.length < 2) {
        mid = "0" + mid;
      }
      if (isUpper) {
        mid = mid.toUpperCase();
      }

      rawValue = rawValue.substr(0, pos) + mid + rawValue.substr(pos + 2);
    }

    return {
      value: "#" + rawValue,
      selection: [offset + 1, offsetEnd + 1]
    };
  },

  








  _cycleCSSSuggestion:
  function InplaceEditor_cycleCSSSuggestion(aReverse, aNoSelect)
  {
    
    let {label, preLabel} = this.popup.selectedItem || {label: "", preLabel: ""};
    if (aReverse) {
      this.popup.selectPreviousItem();
    } else {
      this.popup.selectNextItem();
    }
    this._selectedIndex = this.popup.selectedIndex;
    let input = this.input;
    let pre = "";
    if (input.selectionStart < input.selectionEnd) {
      pre = input.value.slice(0, input.selectionStart);
    }
    else {
      pre = input.value.slice(0, input.selectionStart - label.length +
                                 preLabel.length);
    }
    let post = input.value.slice(input.selectionEnd, input.value.length);
    let item = this.popup.selectedItem;
    let toComplete = item.label.slice(item.preLabel.length);
    input.value = pre + toComplete + post;
    if (!aNoSelect) {
      input.setSelectionRange(pre.length, pre.length + toComplete.length);
    }
    else {
      input.setSelectionRange(pre.length + toComplete.length,
                              pre.length + toComplete.length);
    }
    this._updateSize();
    
    this.emit("after-suggest");
  },

  


  _apply: function InplaceEditor_apply(aEvent, direction)
  {
    if (this._applied) {
      return;
    }

    this._applied = true;

    if (this.done) {
      let val = this.input.value.trim();
      return this.done(this.cancelled ? this.initial : val, !this.cancelled, direction);
    }

    return null;
  },

  


  _onBlur: function InplaceEditor_onBlur(aEvent, aDoNotClear)
  {
    if (aEvent && this.popup && this.popup.isOpen &&
        this.popup.selectedIndex >= 0) {
      let label, preLabel;
      if (this._selectedIndex === undefined) {
        ({label, preLabel}) = this.popup.getItemAtIndex(this.popup.selectedIndex);
      }
      else {
        ({label, preLabel}) = this.popup.getItemAtIndex(this._selectedIndex);
      }
      let input = this.input;
      let pre = "";
      if (input.selectionStart < input.selectionEnd) {
        pre = input.value.slice(0, input.selectionStart);
      }
      else {
        pre = input.value.slice(0, input.selectionStart - label.length +
                                   preLabel.length);
      }
      let post = input.value.slice(input.selectionEnd, input.value.length);
      let item = this.popup.selectedItem;
      this._selectedIndex = this.popup.selectedIndex;
      let toComplete = item.label.slice(item.preLabel.length);
      input.value = pre + toComplete + post;
      input.setSelectionRange(pre.length + toComplete.length,
                              pre.length + toComplete.length);
      this._updateSize();
      
      
      let onPopupHidden = () => {
        this.popup._panel.removeEventListener("popuphidden", onPopupHidden);
        this.doc.defaultView.setTimeout(()=> {
          input.focus();
          this.emit("after-suggest");
        }, 0);
      };
      this.popup._panel.addEventListener("popuphidden", onPopupHidden);
      this.popup.hidePopup();
      
      
      if (this.contentType != CONTENT_TYPES.CSS_MIXED) {
        this._apply();
      }
      return;
    }
    this._apply();
    if (!aDoNotClear) {
      this._clear();
    }
  },

  


  _onKeyPress: function InplaceEditor_onKeyPress(aEvent)
  {
    let prevent = false;

    const largeIncrement = 100;
    const mediumIncrement = 10;
    const smallIncrement = 0.1;

    let increment = 0;

    if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_UP
       || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP) {
      increment = 1;
    } else if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_DOWN
       || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN) {
      increment = -1;
    }

    if (aEvent.shiftKey && !aEvent.altKey) {
      if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP
           ||  aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN) {
        increment *= largeIncrement;
      } else {
        increment *= mediumIncrement;
      }
    } else if (aEvent.altKey && !aEvent.shiftKey) {
      increment *= smallIncrement;
    }

    
    if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_HOME
        || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_END
        || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP
        || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN) {
      this._preventSuggestions = true;
    }

    let cycling = false;
    if (increment && this._incrementValue(increment) ) {
      this._updateSize();
      prevent = true;
      cycling = true;
    } else if (increment && this.popup && this.popup.isOpen) {
      cycling = true;
      prevent = true;
      this._cycleCSSSuggestion(increment > 0);
      this._doValidation();
    }

    if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_BACK_SPACE ||
        aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_DELETE ||
        aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_LEFT ||
        aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RIGHT) {
      if (this.popup && this.popup.isOpen) {
        this.popup.hidePopup();
      }
    } else if (!cycling && !aEvent.metaKey && !aEvent.altKey && !aEvent.ctrlKey) {
      this._maybeSuggestCompletion();
    }

    if (this.multiline &&
        aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN &&
        aEvent.shiftKey) {
      prevent = false;
    } else if (aEvent.charCode in this._advanceCharCodes
       || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN
       || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_TAB) {
      prevent = true;

      let direction = FOCUS_FORWARD;
      if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_TAB &&
          aEvent.shiftKey) {
        if (this.stopOnShiftTab) {
          direction = null;
        } else {
          direction = FOCUS_BACKWARD;
        }
      }
      if ((this.stopOnReturn &&
           aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN) ||
          (this.stopOnTab && aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_TAB)) {
        direction = null;
      }

      
      this._preventSuggestions = true;
      
      
      if (this.contentType == CONTENT_TYPES.CSS_PROPERTY &&
          direction == FOCUS_FORWARD) {
        this._preventSuggestions = false;
      }

      let input = this.input;

      if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_TAB &&
          this.contentType == CONTENT_TYPES.CSS_MIXED) {
        if (this.popup && input.selectionStart < input.selectionEnd) {
          aEvent.preventDefault();
          input.setSelectionRange(input.selectionEnd, input.selectionEnd);
          this.emit("after-suggest");
          return;
        }
        else if (this.popup && this.popup.isOpen) {
          aEvent.preventDefault();
          this._cycleCSSSuggestion(aEvent.shiftKey, true);
          return;
        }
      }

      this._apply(aEvent, direction);

      
      if (this.popup && this.popup.isOpen) {
        this.popup.hidePopup();
      }

      if (direction !== null && focusManager.focusedElement === input) {
        
        
        let next = moveFocus(this.doc.defaultView, direction);

        
        
        if (next && next.ownerDocument === this.doc && next._editable) {
          let e = this.doc.createEvent('Event');
          e.initEvent(next._trigger, true, true);
          next.dispatchEvent(e);
        }
      }

      this._clear();
    } else if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      
      
      this._preventSuggestions = true;
      
      if (this.popup && this.popup.isOpen) {
        this.popup.hidePopup();
      }
      prevent = true;
      this.cancelled = true;
      this._apply();
      this._clear();
      aEvent.stopPropagation();
    } else if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_SPACE) {
      
      
      
      prevent = !this.input.value;
    }

    if (prevent) {
      aEvent.preventDefault();
    }
  },

  


  _onKeyup: function(aEvent) {
    this._applied = false;
  },

  


  _onInput: function InplaceEditor_onInput(aEvent)
  {
    
    this._doValidation();

    
    if (this._measurement) {
      this._updateSize();
    }

    
    if (this.change) {
      this.change(this.input.value.trim());
    }
  },

  


  _doValidation: function()
  {
    if (this.validate && this.input) {
      this.validate(this.input.value);
    }
  },

  





  _maybeSuggestCompletion: function(aNoAutoInsert) {
    
    if (!this.input) {
      return;
    }
    let preTimeoutQuery = this.input.value;
    
    
    
    this.doc.defaultView.setTimeout(() => {
      if (this._preventSuggestions) {
        this._preventSuggestions = false;
        return;
      }
      if (this.contentType == CONTENT_TYPES.PLAIN_TEXT) {
        return;
      }
      if (!this.input) {
        return;
      }
      let input = this.input;
      
      if (input.value.length - preTimeoutQuery.length > 1) {
        return;
      }
      let query = input.value.slice(0, input.selectionStart);
      let startCheckQuery = query;
      if (query == null) {
        return;
      }
      
      
      if (input.selectionStart == input.selectionEnd &&
          input.selectionStart < input.value.length &&
          input.value.slice(input.selectionStart)[0] != " ") {
        
        this.emit("after-suggest", "nothing to autocomplete");
        return;
      }
      let list = [];
      if (this.contentType == CONTENT_TYPES.CSS_PROPERTY) {
        list = CSSPropertyList;
      } else if (this.contentType == CONTENT_TYPES.CSS_VALUE) {
        
        let match = /([^\s,.\/]+$)/.exec(query);
        if (match) {
          startCheckQuery = match[0];
        } else {
          startCheckQuery = "";
        }

        list =
          ["!important", ...domUtils.getCSSValuesForProperty(this.property.name)];

        if (query == "") {
          
          list.splice(0, 1);
        }
      } else if (this.contentType == CONTENT_TYPES.CSS_MIXED &&
                 /^\s*style\s*=/.test(query)) {
        
        let match = query.match(/([:;"'=]?)\s*([^"';:=]+)?$/);
        if (match && match.length >= 2) {
          if (match[1] == ":") { 
            let propertyName =
              query.match(/[;"'=]\s*([^"';:= ]+)\s*:\s*[^"';:=]*$/)[1];
            list =
              ["!important;", ...domUtils.getCSSValuesForProperty(propertyName)];
            let matchLastQuery = /([^\s,.\/]+$)/.exec(match[2] || "");
            if (matchLastQuery) {
              startCheckQuery = matchLastQuery[0];
            } else {
              startCheckQuery = "";
            }
            if (!match[2]) {
              
              list.splice(0, 1);
            }
          } else if (match[1]) { 
            list = CSSPropertyList;
            startCheckQuery = match[2];
          }
          if (startCheckQuery == null) {
            
            this.emit("after-suggest", "nothing to autocomplete");
            return;
          }
        }
      }
      if (!aNoAutoInsert) {
        list.some(item => {
          if (startCheckQuery != null && item.startsWith(startCheckQuery)) {
            input.value = query + item.slice(startCheckQuery.length) +
                          input.value.slice(query.length);
            input.setSelectionRange(query.length, query.length + item.length -
                                                  startCheckQuery.length);
            this._updateSize();
            return true;
          }
        });
      }

      if (!this.popup) {
        
        this.emit("after-suggest", "no popup");
        return;
      }
      let finalList = [];
      let length = list.length;
      for (let i = 0, count = 0; i < length && count < MAX_POPUP_ENTRIES; i++) {
        if (startCheckQuery != null && list[i].startsWith(startCheckQuery)) {
          count++;
          finalList.push({
            preLabel: startCheckQuery,
            label: list[i]
          });
        }
        else if (count > 0) {
          
          
          break;
        }
        else if (startCheckQuery != null && list[i][0] > startCheckQuery[0]) {
          
          break;
        }
      }

      if (finalList.length > 1) {
        
        let x = (this.input.selectionStart - startCheckQuery.length) *
                this.inputCharWidth;
        this.popup.setItems(finalList);
        this.popup.openPopup(this.input, x);
        if (aNoAutoInsert) {
          this.popup.selectedIndex = -1;
        }
      } else {
        this.popup.hidePopup();
      }
      
      this.emit("after-suggest");
      this._doValidation();
    }, 0);
  }
};




function copyTextStyles(aFrom, aTo)
{
  let win = aFrom.ownerDocument.defaultView;
  let style = win.getComputedStyle(aFrom);
  aTo.style.fontFamily = style.getPropertyCSSValue("font-family").cssText;
  aTo.style.fontSize = style.getPropertyCSSValue("font-size").cssText;
  aTo.style.fontWeight = style.getPropertyCSSValue("font-weight").cssText;
  aTo.style.fontStyle = style.getPropertyCSSValue("font-style").cssText;
}




function moveFocus(aWin, aDirection)
{
  return focusManager.moveFocus(aWin, null, aDirection, 0);
}


XPCOMUtils.defineLazyGetter(this, "focusManager", function() {
  return Services.focus;
});

XPCOMUtils.defineLazyGetter(this, "CSSPropertyList", function() {
  return domUtils.getCSSPropertyNames(domUtils.INCLUDE_ALIASES).sort();
});

XPCOMUtils.defineLazyGetter(this, "domUtils", function() {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});
