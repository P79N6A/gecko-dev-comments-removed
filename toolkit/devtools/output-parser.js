





"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {colorUtils} = require("devtools/css-color");
const {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

const HTML_NS = "http://www.w3.org/1999/xhtml";

const MAX_ITERATIONS = 100;

const BEZIER_KEYWORDS = ["linear", "ease-in-out", "ease-in", "ease-out",
                         "ease"];


const COLOR_TAKING_FUNCTIONS = ["linear-gradient",
                                "-moz-linear-gradient",
                                "repeating-linear-gradient",
                                "-moz-repeating-linear-gradient",
                                "radial-gradient",
                                "-moz-radial-gradient",
                                "repeating-radial-gradient",
                                "-moz-repeating-radial-gradient",
                                "drop-shadow"];

loader.lazyGetter(this, "DOMUtils", function() {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

















function OutputParser() {
  this.parsed = [];
  this.colorSwatches = new WeakMap();
  this._onSwatchMouseDown = this._onSwatchMouseDown.bind(this);
}

exports.OutputParser = OutputParser;

OutputParser.prototype = {
  












  parseCssProperty: function(name, value, options={}) {
    options = this._mergeOptions(options);

    options.expectCubicBezier =
      safeCssPropertySupportsType(name, DOMUtils.TYPE_TIMING_FUNCTION);
    options.expectFilter = name === "filter";
    options.supportsColor =
      safeCssPropertySupportsType(name, DOMUtils.TYPE_COLOR) ||
      safeCssPropertySupportsType(name, DOMUtils.TYPE_GRADIENT);

    if (this._cssPropertySupportsValue(name, value)) {
      return this._parse(value, options);
    }
    this._appendTextNode(value);

    return this._toDOM();
  },

  














  _collectFunctionText: function(initialToken, text, tokenStream) {
    let result = text.substring(initialToken.startOffset,
                                initialToken.endOffset);
    let depth = 1;
    while (depth > 0) {
      let token = tokenStream.nextToken();
      if (!token) {
        break;
      }
      if (token.tokenType === "comment") {
        continue;
      }
      result += text.substring(token.startOffset, token.endOffset);
      if (token.tokenType === "symbol") {
        if (token.text === "(") {
          ++depth;
        } else if (token.text === ")") {
          --depth;
        }
      } else if (token.tokenType === "function") {
        ++depth;
      }
    }
    return result;
  },

  










  _parse: function(text, options={}) {
    text = text.trim();
    this.parsed.length = 0;

    let tokenStream = DOMUtils.getCSSLexer(text);
    let i = 0;
    let parenDepth = 0;
    let outerMostFunctionTakesColor = false;

    let colorOK = function() {
      return options.supportsColor ||
        (options.expectFilter && parenDepth === 1 &&
         outerMostFunctionTakesColor);
    };

    while (true) {
      let token = tokenStream.nextToken();
      if (!token) {
        break;
      }
      if (token.tokenType === "comment") {
        continue;
      }

      
      
      
      i++;
      if (i > MAX_ITERATIONS) {
        this._appendTextNode(text.substring(token.startOffset,
                                            token.endOffset));
        continue;
      }

      switch (token.tokenType) {
        case "function": {
          if (COLOR_TAKING_FUNCTIONS.indexOf(token.text) >= 0) {
            
            
            
            
            this._appendTextNode(text.substring(token.startOffset,
                                                token.endOffset));
            if (parenDepth === 0) {
              outerMostFunctionTakesColor = true;
            }
            ++parenDepth;
          } else {
            let functionText = this._collectFunctionText(token, text,
                                                         tokenStream);

            if (options.expectCubicBezier && token.text === "cubic-bezier") {
              this._appendCubicBezier(functionText, options);
            } else if (colorOK() && DOMUtils.isValidCSSColor(functionText)) {
              this._appendColor(functionText, options);
            } else {
              this._appendTextNode(functionText);
            }
          }
          break;
        }

        case "ident":
          if (options.expectCubicBezier &&
              BEZIER_KEYWORDS.indexOf(token.text) >= 0) {
            this._appendCubicBezier(token.text, options);
          } else if (colorOK() && DOMUtils.isValidCSSColor(token.text)) {
            this._appendColor(token.text, options);
          } else {
            this._appendTextNode(text.substring(token.startOffset,
                                                token.endOffset));
          }
          break;

        case "id":
        case "hash": {
          let original = text.substring(token.startOffset, token.endOffset);
          if (colorOK() && DOMUtils.isValidCSSColor(original)) {
            this._appendColor(original, options);
          } else {
            this._appendTextNode(original);
          }
          break;
        }

        case "url":
        case "bad_url":
          this._appendURL(text.substring(token.startOffset, token.endOffset),
                          token.text, options);
          break;

        case "symbol":
          if (token.text === "(") {
            ++parenDepth;
          } else if (token.token === ")") {
            --parenDepth;
          }
          
        default:
          this._appendTextNode(text.substring(token.startOffset,
                                              token.endOffset));
          break;
      }
    }

    let result = this._toDOM();

    if (options.expectFilter && !options.filterSwatch) {
      result = this._wrapFilter(text, options, result);
    }

    return result;
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
    return DOMUtils.cssPropertyIsValid(name, value);
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
        this.colorSwatches.set(swatch, colorObj);
        swatch.addEventListener("mousedown", this._onSwatchMouseDown, false);
        container.appendChild(swatch);
      }

      if (options.defaultColorType) {
        color = colorObj.toString();
        container.dataset.colorÂ = color;
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

  












  _wrapFilter: function(filters, options, nodes) {
    let container = this._createNode("span", {
      "data-filters": filters
    });

    if (options.filterSwatchClass) {
      let swatch = this._createNode("span", {
        class: options.filterSwatchClass
      });
      container.appendChild(swatch);
    }

    let value = this._createNode("span", {
      class: options.filterClass
    });
    value.appendChild(nodes);
    container.appendChild(value);

    return container;
  },

  _onSwatchMouseDown: function(event) {
    
    event.preventDefault();

    if (!event.shiftKey) {
      return;
    }

    let swatch = event.target;
    let color = this.colorSwatches.get(swatch);
    let val = color.nextColorUnit();

    swatch.nextElementSibling.textContent = val;
  },

   










  _appendURL: function(match, url, options={}) {
    if (options.urlClass) {
      this._appendTextNode("url(\"");

      let href = url;
      if (options.baseURI) {
        href = options.baseURI.resolve(url);
      }

      this._appendNode("a", {
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
      supportsColor: false,
      urlClass: "",
      baseURI: "",
      filterSwatch: false
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










function safeCssPropertySupportsType(name, type) {
  try {
    return DOMUtils.cssPropertySupportsType(name, type);
  } catch(e) {
    return false;
  }
}
