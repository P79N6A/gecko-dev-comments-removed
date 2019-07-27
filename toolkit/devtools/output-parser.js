



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {colorUtils} = require("devtools/css-color");
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

const HTML_NS = "http://www.w3.org/1999/xhtml";

const MAX_ITERATIONS = 100;
const REGEX_QUOTES = /^".*?"|^".*|^'.*?'|^'.*/;
const REGEX_WHITESPACE = /^\s+/;
const REGEX_FIRST_WORD_OR_CHAR = /^\w+|^./;
const REGEX_CSS_PROPERTY_VALUE = /(^[^;]+)/;
const REGEX_CUBIC_BEZIER = /^linear|^ease-in-out|^ease-in|^ease-out|^ease|^cubic-bezier\(([0-9.\- ]+,){3}[0-9.\- ]+\)/;











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

    
    
    options.expectCubicBezier = ["transition", "transition-timing-function",
      "animation", "animation-timing-function"].indexOf(name) !== -1;

    if (this._cssPropertySupportsValue(name, value)) {
      return this._parse(value, options);
    }
    this._appendTextNode(value);

    return this._toDOM();
  },

  










  parseHTMLAttribute: function(value, options={}) {
    options.isHTMLAttribute = true;
    options = this._mergeOptions(options);

    return this._parse(value, options);
  },

  





  _matchBackgroundUrl: function(text) {
    let startToken = "url(";
    if (text.indexOf(startToken) !== 0) {
      return null;
    }

    let uri = text.substring(startToken.length).trim();
    let quote = uri.substring(0, 1);
    if (quote === "'" || quote === '"') {
      uri = uri.substring(1, uri.search(new RegExp(quote + "\\s*\\)")));
    } else {
      uri = uri.substring(0, uri.indexOf(")"));
      quote = "";
    }
    let end = startToken + quote + uri;
    text = text.substring(0, text.indexOf(")", end.length) + 1);

    return [text, uri.trim()];
  },

  










  _parse: function(text, options={}) {
    text = text.trim();
    this.parsed.length = 0;
    let i = 0;

    while (text.length > 0) {
      let matched = null;

      
      
      
      i++;
      if (i > MAX_ITERATIONS) {
        this._appendTextNode(text);
        text = "";
        break;
      }

      matched = text.match(REGEX_QUOTES);
      if (matched) {
        let match = matched[0];

        text = this._trimMatchFromStart(text, match);
        this._appendTextNode(match);
        continue;
      }

      matched = text.match(REGEX_WHITESPACE);
      if (matched) {
        let match = matched[0];

        text = this._trimMatchFromStart(text, match);
        this._appendTextNode(match);
        continue;
      }

      matched = this._matchBackgroundUrl(text);
      if (matched) {
        let [match, url] = matched;
        text = this._trimMatchFromStart(text, match);

        this._appendURL(match, url, options);
        continue;
      }

      if (options.expectCubicBezier) {
        matched = text.match(REGEX_CUBIC_BEZIER);
        if (matched) {
          let match = matched[0];
          text = this._trimMatchFromStart(text, match);

          this._appendCubicBezier(match, options);
          continue;
        }
      }

      matched = text.match(REGEX_ALL_CSS_PROPERTIES);
      if (matched) {
        let [match] = matched;

        text = this._trimMatchFromStart(text, match);
        this._appendTextNode(match);

        if (options.isHTMLAttribute) {
          [text] = this._appendColorOnMatch(text, options);
        }
        continue;
      }

      if (!options.isHTMLAttribute) {
        let dirty;

        [text, dirty] = this._appendColorOnMatch(text, options);

        if (dirty) {
          continue;
        }
      }

      
      
      matched = text.match(REGEX_FIRST_WORD_OR_CHAR);
      if (matched) {
        let match = matched[0];

        text = this._trimMatchFromStart(text, match);
        this._appendTextNode(match);
      }
    }

    return this._toDOM();
  },

  










  _trimMatchFromStart: function(text, match) {
    return text.substr(match.length);
  },

  












  _appendColorOnMatch: function(text, options) {
    let dirty;
    let matched = text.match(REGEX_ALL_COLORS);

    if (matched) {
      let match = matched[0];
      if (this._appendColor(match, options)) {
        text = this._trimMatchFromStart(text, match);
        dirty = true;
      }
    } else {
      dirty = false;
    }

    return [text, dirty];
  },

  








  _appendCubicBezier: function(bezier, options) {
    let container = this._createNode("span", {
       "data-bezier": bezier
    });

    if (options.bezierSwatchClass) {
      let swatch = this._createNode("span", {
        class: options.bezierSwatchClass
      });
      container.appendChild(swatch);
    }

    let value = this._createNode("span", {
      class: options.bezierClass
    }, bezier);

    container.appendChild(value);
    this.parsed.push(container);
  },

  







  _cssPropertySupportsValue: function(name, value) {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;

    value = value.replace("!important", "");

    let div = doc.createElement("div");
    div.style.setProperty(name, value);

    return !!div.style.getPropertyValue(name);
  },

  




  _isValidColor: function(colorObj) {
    return colorObj.valid &&
      (!colorObj.specialValue || colorObj.specialValue === "transparent");
  },

  











  _appendColor: function(color, options={}) {
    let colorObj = new colorUtils.CssColor(color);

    if (this._isValidColor(colorObj)) {
      let container = this._createNode("span", {
         "data-color": color
      });

      if (options.colorSwatchClass) {
        let swatch = this._createNode("span", {
          class: options.colorSwatchClass,
          style: "background-color:" + color
        });
        container.appendChild(swatch);
      }

      if (options.defaultColorType) {
        color = colorObj.toString();
        container.dataset["color"]Â = color;
      }

      let value = this._createNode("span", {
        class: options.colorClass
      }, color);

      container.appendChild(value);
      this.parsed.push(container);
      return true;
    }
    return false;
  },

   










  _appendURL: function(match, url, options={}) {
    if (options.urlClass) {
      this._appendTextNode("url(\"");

      let href = url;
      if (options.baseURI) {
        href = options.baseURI.resolve(url);
      }

      this._appendNode("a",  {
        target: "_blank",
        class: options.urlClass,
        href: href
      }, url);

      this._appendTextNode("\")");
    } else {
      this._appendTextNode("url(\"" + url + "\")");
    }
  },

  











  _createNode: function(tagName, attributes, value="") {
    let win = Services.appShell.hiddenDOMWindow;
    let doc = win.document;
    let node = doc.createElementNS(HTML_NS, tagName);
    let attrs = Object.getOwnPropertyNames(attributes);

    for (let attr of attrs) {
      if (attributes[attr]) {
        node.setAttribute(attr, attributes[attr]);
      }
    }

    if (value) {
      let textNode = doc.createTextNode(value);
      node.appendChild(textNode);
    }

    return node;
  },

  










  _appendNode: function(tagName, attributes, value="") {
    let node = this._createNode(tagName, attributes, value);
    this.parsed.push(node);
  },

  






  _appendTextNode: function(text) {
    let lastItem = this.parsed[this.parsed.length - 1];
    if (typeof lastItem === "string") {
      this.parsed[this.parsed.length - 1] = lastItem + text;
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
      colorClass: "",
      bezierSwatchClass: "",
      bezierClass: "",
      isHTMLAttribute: false,
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
  }
};
