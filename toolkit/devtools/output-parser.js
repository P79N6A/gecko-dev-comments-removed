



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {colorUtils} = require("devtools/css-color");
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

const HTML_NS = "http://www.w3.org/1999/xhtml";

const MAX_ITERATIONS = 100;
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

    if (this._cssPropertySupportsValue(name, value)) {
      return this._parse(value, options);
    }
    this._appendTextNode(value);

    return this._toDOM();
  },

  










  parseHTMLAttribute: function(value, options={}) {
    options = this._mergeOptions(options);

    return this._parse(value, options);
  },

  










  _parse: function(text, options={}) {
    text = text.trim();
    this.parsed.length = 0;
    let dirty = false;
    let matched = null;
    let i = 0;

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
        let [match, url] = matched;
        trimMatchFromStart(match);
        this._appendURL(match, url, options);
      }

      matched = text.match(REGEX_ALL_CSS_PROPERTIES);
      if (matched) {
        let [match] = matched;
        trimMatchFromStart(match);
        this._appendTextNode(match);

        dirty = true;
      }

      matched = text.match(REGEX_ALL_COLORS);
      if (matched) {
        let match = matched[0];
        if (this._appendColor(match, options)) {
          trimMatchFromStart(match);
        }
      }

      if (!dirty) {
        
        
        matched = text.match(REGEX_FIRST_WORD_OR_CHAR);
        if (matched) {
          let match = matched[0];
          trimMatchFromStart(match);
          this._appendTextNode(match);
        }
      }

      dirty = false;

      
      
      i++;
      if (i > MAX_ITERATIONS) {
        trimMatchFromStart(text);
        this._appendTextNode(text);
      }
    }

    return this._toDOM();
  },

  







  _cssPropertySupportsValue: function(name, value) {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;

    name = name.replace(/-\w{1}/g, function(match) {
      return match.charAt(1).toUpperCase();
    });

    let div = doc.createElement("div");
    div.style[name] = value;

    return !!div.style[name];
  },

  











  _appendColor: function(color, options={}) {
    let colorObj = new colorUtils.CssColor(color);

    if (colorObj.valid && !colorObj.specialValue) {
      if (options.colorSwatchClass) {
        this._appendNode("span", {
          class: options.colorSwatchClass,
          style: "background-color:" + color
        });
      }
      if (options.defaultColorType) {
        color = colorObj.toString();
      }
      this._appendTextNode(color);
      return true;
    }
    return false;
  },

   










  _appendURL: function(match, url, options={}) {
    if (options.urlClass) {
      
      
      this._appendTextNode("url('");

      let href = url;
      if (options.baseURI) {
        href = options.baseURI.resolve(url);
      }

      this._appendNode("a",  {
        target: "_blank",
        class: options.urlClass,
        href: href
      }, url);

      this._appendTextNode("')");
    } else {
      this._appendTextNode("url('" + url + "')");
    }
  },

  










  _appendNode: function(tagName, attributes, value="") {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;
    let node = doc.createElementNS(HTML_NS, tagName);
    let attrs = Object.getOwnPropertyNames(attributes);

    for (let attr of attrs) {
      node.setAttribute(attr, attributes[attr]);
    }

    if (value) {
      let textNode = doc.createTextNode(value);
      node.appendChild(textNode);
    }

    this.parsed.push(node);
  },

  






  _appendTextNode: function(text) {
    let lastItem = this.parsed[this.parsed.length - 1];
    if (typeof lastItem === "string") {
      this.parsed[this.parsed.length - 1] = lastItem + text
    } else {
      this.parsed.push(text);
    }
  },

  





  _toDOM: function() {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;
    let frag = doc.createDocumentFragment();

    for (let item of this.parsed) {
      if (typeof item === "string") {
        frag.appendChild(doc.createTextNode(item));
      } else {
        frag.appendChild(item);
      }
    }

    this.parsed.length = 0;
    return frag;
  },

  















  _mergeOptions: function(overrides) {
    let defaults = {
      defaultColorType: true,
      colorSwatchClass: "",
      urlClass: "",
      baseURI: ""
    };

    if (typeof overrides.baseURI === "string") {
      overrides.baseURI = Services.io.newURI(overrides.baseURI, null, null);
    }

    for (let item in overrides) {
      defaults[item] = overrides[item];
    }
    return defaults;
  },
};
