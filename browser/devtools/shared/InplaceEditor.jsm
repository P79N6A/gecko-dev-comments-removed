























"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const HTML_NS = "http://www.w3.org/1999/xhtml";

const FOCUS_FORWARD = Ci.nsIFocusManager.MOVEFOCUS_FORWARD;
const FOCUS_BACKWARD = Ci.nsIFocusManager.MOVEFOCUS_BACKWARD;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["editableItem",
                         "editableField",
                         "getInplaceEditorForSpan",
                         "InplaceEditor"];



































function editableField(aOptions)
{
  return editableItem(aOptions, function(aElement, aEvent) {
    new InplaceEditor(aOptions, aEvent);
  });
}














this.editableItem = function editableItem(aOptions, aCallback)
{
  let trigger = aOptions.trigger || "click"
  let element = aOptions.element;
  element.addEventListener(trigger, function(evt) {
    let win = this.ownerDocument.defaultView;
    let selection = win.getSelection();
    if (trigger != "click" || selection.isCollapsed) {
      aCallback(element, evt);
    }
    evt.stopPropagation();
  }, false);

  
  
  element.addEventListener("keypress", function(evt) {
    if (evt.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN ||
        evt.charCode === Ci.nsIDOMKeyEvent.DOM_VK_SPACE) {
      aCallback(element);
    }
  }, true);

  
  
  
  
  element.addEventListener("mousedown", function(evt) {
    let cleanup = function() {
      element.style.removeProperty("outline-style");
      element.removeEventListener("mouseup", cleanup, false);
      element.removeEventListener("mouseout", cleanup, false);
    };
    element.style.setProperty("outline-style", "none");
    element.addEventListener("mouseup", cleanup, false);
    element.addEventListener("mouseout", cleanup, false);
  }, false);

  
  
  element._editable = true;
}







this.getInplaceEditorForSpan = function getInplaceEditorForSpan(aSpan)
{
  return aSpan.inplaceEditor;
};

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
  this.stopOnReturn = !!aOptions.stopOnReturn;

  this._onBlur = this._onBlur.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
  this._onInput = this._onInput.bind(this);
  this._onKeyup = this._onKeyup.bind(this);

  this._createInput();
  this._autosize();

  
  
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

  this.input.addEventListener("blur", this._onBlur, false);
  this.input.addEventListener("keypress", this._onKeyPress, false);
  this.input.addEventListener("input", this._onInput, false);
  this.input.addEventListener("mousedown", function(aEvt) {
                                             aEvt.stopPropagation();
                                           }, false);

  this.warning = aOptions.warning;
  this.validate = aOptions.validate;

  if (this.warning && this.validate) {
    this.input.addEventListener("keyup", this._onKeyup, false);
  }

  if (aOptions.start) {
    aOptions.start(this, aEvent);
  }
}

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

    if (this.destroy) {
      this.destroy();
    }

    this.elt.parentNode.removeChild(this.input);
    this.input = null;

    delete this.elt.inplaceEditor;
    delete this.elt;
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
    this._measurement.parentNode.removeChild(this._measurement);
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
    const reSplitCSS = /(url\("?[^"\)]+"?\)?)|(rgba?\([^)]*\)?)|(hsla?\([^)]*\)?)|(#[\dA-Fa-f]+)|(-?\d+(\.\d+)?(%|[a-z]{1,4})?)|"([^"]*)"?|'([^']*)'?|([^,\s\/!\(\)]+)|(!(.*)?)/;
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

  


  _apply: function InplaceEditor_apply(aEvent)
  {
    if (this._applied) {
      return;
    }

    this._applied = true;

    if (this.done) {
      let val = this.input.value.trim();
      return this.done(this.cancelled ? this.initial : val, !this.cancelled);
    }

    return null;
  },

  


  _onBlur: function InplaceEditor_onBlur(aEvent, aDoNotClear)
  {
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

    if (increment && this._incrementValue(increment) ) {
      this._updateSize();
      prevent = true;
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
        this.cancelled = true;
        direction = FOCUS_BACKWARD;
      }
      if (this.stopOnReturn && aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN) {
        direction = null;
      }

      let input = this.input;

      this._apply();

      if (direction !== null && focusManager.focusedElement === input) {
        
        
        let next = moveFocus(this.doc.defaultView, direction);

        
        
        if (next && next.ownerDocument === this.doc && next._editable) {
          next.click();
        }
      }

      this._clear();
    } else if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      
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
    
    this.warning.hidden = this.validate(this.input.value);
    this._applied = false;
    this._onBlur(null, true);
  },

  


  _onInput: function InplaceEditor_onInput(aEvent)
  {
    
    if (this.warning && this.validate) {
      this.warning.hidden = this.validate(this.input.value);
    }

    
    if (this._measurement) {
      this._updateSize();
    }

    
    if (this.change) {
      this.change(this.input.value.trim());
    }
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
