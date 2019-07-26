



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {colorUtils} = require("devtools/css-color");
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

const REGEX_QUOTES = /^".*?"|^".*/;
const REGEX_URL = /^url\(["']?(.+?)(?::(\d+))?["']?\)/;
const REGEX_WHITESPACE = /^\s+/;
const REGEX_FIRST_WORD_OR_CHAR = /^\w+|^./;
const REGEX_CSS_PROPERTY_VALUE = /(^[^;]+)/;











const REGEX_ALL_COLORS = /^#[0-9a-fA-F]{3}\b|^#[0-9a-fA-F]{6}\b|^hsl\(.*?\)|^hsla\(.*?\)|^rgba?\(.*?\)|^[a-zA-Z-]+/;

loader.lazyGetter(this, "DOMUtils", function () {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});






loader.lazyGetter(this, "REGEX_ALL_CSS_PROPERTIES", function () {
  let names = DOMUtils.getCSSPropertyNames();
    let pattern = "^(";

    for (let i = 0; i < names.length; i++) {
      if (i > 0) {
        pattern += "|";
      }
      pattern += names[i];
    }
    pattern += ")\\s*:\\s*";

    return new RegExp(pattern);
});


















function OutputParser() {
  this.parsed = [];
}

exports.OutputParser = OutputParser;

OutputParser.prototype = {
  












  parseCssProperty: function(name, value, options={}) {
    options = this._mergeOptions(options);
    options.cssPropertyName = name;

    
    
    options.colors = this._cssPropertySupportsValue(name, "papayawhip");

    return this._parse(value, options);
  },

  











  parseHTMLAttribute: function(value, options={}) {
    options = this._mergeOptions(options);
    options.colors = false;

    return this._parse(value, options);
  },

  











  _parse: function(text, options={}) {
    text = text.trim();
    this.parsed.length = 0;
    let dirty = false;
    let matched = null;
    let nameValueSupported = false;

    let trimMatchFromStart = function(match) {
      text = text.substr(match.length);
      dirty = true;
      matched = null;
    };

    while (text.length > 0) {
      matched = text.match(REGEX_QUOTES);
      if (matched) {
        let match = matched[0];
        trimMatchFromStart(match);
        this._appendTextNode(match);
      }

      matched = text.match(REGEX_WHITESPACE);
      if (matched) {
        let match = matched[0];
        trimMatchFromStart(match);
        this._appendTextNode(match);
      }

      matched = text.match(REGEX_URL);
      if (matched) {
        let [match, url, line] = matched;
        trimMatchFromStart(match);
        this._appendURL(match, url, line, options);
      }

      
      
      matched = text.match(REGEX_ALL_CSS_PROPERTIES);
      if (matched) {
        let [match, propertyName] = matched;
        trimMatchFromStart(match);
        this._appendTextNode(match);

        matched = text.match(REGEX_CSS_PROPERTY_VALUE);
        if (matched) {
          let [, value] = matched;
          nameValueSupported = this._cssPropertySupportsValue(propertyName, value);
        }
      }

      
      
      
      
      if (options.cssPropertyName || (!options.cssPropertyName && nameValueSupported)) {
        matched = text.match(REGEX_ALL_COLORS);
        if (matched) {
          let match = matched[0];
          trimMatchFromStart(match);
          this._appendColor(match, options);
        }

        nameValueSupported = false;
      }

      if (!dirty) {
        
        
        matched = text.match(REGEX_FIRST_WORD_OR_CHAR);
        if (matched) {
          let match = matched[0];
          trimMatchFromStart(match);
          this._appendTextNode(match);
          nameValueSupported = false;
        }
      }

      dirty = false;
    }

    return this._toDOM();
  },

  







  _cssPropertySupportsValue: function(propertyName, propertyValue) {
    let autoCompleteValues = DOMUtils.getCSSValuesForProperty(propertyName);

    
    
    if (autoCompleteValues.indexOf("papayawhip") !== -1) {
      return this._isColorValid(propertyValue);
    }

    
    return autoCompleteValues.indexOf(propertyValue) !== -1;
  },

  








  _appendColor: function(color, options={}) {
    if (options.colors && this._isColorValid(color)) {
      if (options.colorSwatchClass) {
        this._appendNode("span", {
          class: options.colorSwatchClass,
          style: "background-color:" + color
        });
      }
      if (options.defaultColorType) {
        color = new colorUtils.CssColor(color).toString();
      }
    }
    this._appendTextNode(color);
  },

  












  _appendURL: function(match, url, line, options={}) {
    this._appendTextNode(match);
  },

  










  _appendNode: function(tagName, attributes, value="") {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;
    let node = doc.createElement(tagName);
    let attrs = Object.getOwnPropertyNames(attributes);

    for (let attr of attrs) {
      node.setAttribute(attr, attributes[attr]);
    }

    if (value) {
      let textNode = content.document.createTextNode(value);
      node.appendChild(textNode);
    }

    this.parsed.push(node);
  },

  






  _appendTextNode: function(text) {
    let lastItem = this.parsed[this.parsed.length - 1];

    if (typeof lastItem !== "undefined" && lastItem.nodeName === "#text") {
      lastItem.nodeValue += text;
    } else {
      let win = Services.appShell.hiddenDOMWindow;
      let doc = win.document;
      let textNode = doc.createTextNode(text);
      this.parsed.push(textNode);
    }
  },

  





  _toDOM: function() {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;
    let frag = doc.createDocumentFragment();

    for (let item of this.parsed) {
      frag.appendChild(item);
    }

    this.parsed.length = 0;
    return frag;
  },

  





  _isColorValid: function(color) {
    return new colorUtils.CssColor(color).valid;
  },

  
















  _mergeOptions: function(overrides) {
    let defaults = {
      colors: true,
      defaultColorType: true,
      colorSwatchClass: "",
      cssPropertyName: ""
    };

    for (let item in overrides) {
      defaults[item] = overrides[item];
    }
    return defaults;
  },
};
