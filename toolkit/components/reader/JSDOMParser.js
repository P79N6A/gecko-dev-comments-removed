
































(function (global) {

  function error(m) {
    dump("JSDOMParser error: " + m + "\n");
  }

  

  var entityTable = {
    "lt": "<",
    "gt": ">",
    "amp": "&",
    "quot": '"',
    "apos": "'",
  };

  var reverseEntityTable = {
    "<": "&lt;",
    ">": "&gt;",
    "&": "&amp;",
    '"': "&quot;",
    "'": "&apos;",
  };

  function encodeHTML(s) {
    return s.replace(/[&<>'"]/g, function(x) {
      return reverseEntityTable[x];
    });
  }

  function decodeHTML(str) {
    return str.replace(/&(quot|amp|apos|lt|gt);/g, function(match, tag) {
      return entityTable[tag];
    }).replace(/&#(?:x([0-9a-z]{1,4})|([0-9]{1,4}));/gi, function(match, hex, numStr) {
      var num = parseInt(hex || numStr, hex ? 16 : 10); 
      return String.fromCharCode(num);
    });
  }

  
  var styleMap = {
    "alignmentBaseline": "alignment-baseline",
    "background": "background",
    "backgroundAttachment": "background-attachment",
    "backgroundClip": "background-clip",
    "backgroundColor": "background-color",
    "backgroundImage": "background-image",
    "backgroundOrigin": "background-origin",
    "backgroundPosition": "background-position",
    "backgroundPositionX": "background-position-x",
    "backgroundPositionY": "background-position-y",
    "backgroundRepeat": "background-repeat",
    "backgroundRepeatX": "background-repeat-x",
    "backgroundRepeatY": "background-repeat-y",
    "backgroundSize": "background-size",
    "baselineShift": "baseline-shift",
    "border": "border",
    "borderBottom": "border-bottom",
    "borderBottomColor": "border-bottom-color",
    "borderBottomLeftRadius": "border-bottom-left-radius",
    "borderBottomRightRadius": "border-bottom-right-radius",
    "borderBottomStyle": "border-bottom-style",
    "borderBottomWidth": "border-bottom-width",
    "borderCollapse": "border-collapse",
    "borderColor": "border-color",
    "borderImage": "border-image",
    "borderImageOutset": "border-image-outset",
    "borderImageRepeat": "border-image-repeat",
    "borderImageSlice": "border-image-slice",
    "borderImageSource": "border-image-source",
    "borderImageWidth": "border-image-width",
    "borderLeft": "border-left",
    "borderLeftColor": "border-left-color",
    "borderLeftStyle": "border-left-style",
    "borderLeftWidth": "border-left-width",
    "borderRadius": "border-radius",
    "borderRight": "border-right",
    "borderRightColor": "border-right-color",
    "borderRightStyle": "border-right-style",
    "borderRightWidth": "border-right-width",
    "borderSpacing": "border-spacing",
    "borderStyle": "border-style",
    "borderTop": "border-top",
    "borderTopColor": "border-top-color",
    "borderTopLeftRadius": "border-top-left-radius",
    "borderTopRightRadius": "border-top-right-radius",
    "borderTopStyle": "border-top-style",
    "borderTopWidth": "border-top-width",
    "borderWidth": "border-width",
    "bottom": "bottom",
    "boxShadow": "box-shadow",
    "boxSizing": "box-sizing",
    "captionSide": "caption-side",
    "clear": "clear",
    "clip": "clip",
    "clipPath": "clip-path",
    "clipRule": "clip-rule",
    "color": "color",
    "colorInterpolation": "color-interpolation",
    "colorInterpolationFilters": "color-interpolation-filters",
    "colorProfile": "color-profile",
    "colorRendering": "color-rendering",
    "content": "content",
    "counterIncrement": "counter-increment",
    "counterReset": "counter-reset",
    "cursor": "cursor",
    "direction": "direction",
    "display": "display",
    "dominantBaseline": "dominant-baseline",
    "emptyCells": "empty-cells",
    "enableBackground": "enable-background",
    "fill": "fill",
    "fillOpacity": "fill-opacity",
    "fillRule": "fill-rule",
    "filter": "filter",
    "cssFloat": "float",
    "floodColor": "flood-color",
    "floodOpacity": "flood-opacity",
    "font": "font",
    "fontFamily": "font-family",
    "fontSize": "font-size",
    "fontStretch": "font-stretch",
    "fontStyle": "font-style",
    "fontVariant": "font-variant",
    "fontWeight": "font-weight",
    "glyphOrientationHorizontal": "glyph-orientation-horizontal",
    "glyphOrientationVertical": "glyph-orientation-vertical",
    "height": "height",
    "imageRendering": "image-rendering",
    "kerning": "kerning",
    "left": "left",
    "letterSpacing": "letter-spacing",
    "lightingColor": "lighting-color",
    "lineHeight": "line-height",
    "listStyle": "list-style",
    "listStyleImage": "list-style-image",
    "listStylePosition": "list-style-position",
    "listStyleType": "list-style-type",
    "margin": "margin",
    "marginBottom": "margin-bottom",
    "marginLeft": "margin-left",
    "marginRight": "margin-right",
    "marginTop": "margin-top",
    "marker": "marker",
    "markerEnd": "marker-end",
    "markerMid": "marker-mid",
    "markerStart": "marker-start",
    "mask": "mask",
    "maxHeight": "max-height",
    "maxWidth": "max-width",
    "minHeight": "min-height",
    "minWidth": "min-width",
    "opacity": "opacity",
    "orphans": "orphans",
    "outline": "outline",
    "outlineColor": "outline-color",
    "outlineOffset": "outline-offset",
    "outlineStyle": "outline-style",
    "outlineWidth": "outline-width",
    "overflow": "overflow",
    "overflowX": "overflow-x",
    "overflowY": "overflow-y",
    "padding": "padding",
    "paddingBottom": "padding-bottom",
    "paddingLeft": "padding-left",
    "paddingRight": "padding-right",
    "paddingTop": "padding-top",
    "page": "page",
    "pageBreakAfter": "page-break-after",
    "pageBreakBefore": "page-break-before",
    "pageBreakInside": "page-break-inside",
    "pointerEvents": "pointer-events",
    "position": "position",
    "quotes": "quotes",
    "resize": "resize",
    "right": "right",
    "shapeRendering": "shape-rendering",
    "size": "size",
    "speak": "speak",
    "src": "src",
    "stopColor": "stop-color",
    "stopOpacity": "stop-opacity",
    "stroke": "stroke",
    "strokeDasharray": "stroke-dasharray",
    "strokeDashoffset": "stroke-dashoffset",
    "strokeLinecap": "stroke-linecap",
    "strokeLinejoin": "stroke-linejoin",
    "strokeMiterlimit": "stroke-miterlimit",
    "strokeOpacity": "stroke-opacity",
    "strokeWidth": "stroke-width",
    "tableLayout": "table-layout",
    "textAlign": "text-align",
    "textAnchor": "text-anchor",
    "textDecoration": "text-decoration",
    "textIndent": "text-indent",
    "textLineThrough": "text-line-through",
    "textLineThroughColor": "text-line-through-color",
    "textLineThroughMode": "text-line-through-mode",
    "textLineThroughStyle": "text-line-through-style",
    "textLineThroughWidth": "text-line-through-width",
    "textOverflow": "text-overflow",
    "textOverline": "text-overline",
    "textOverlineColor": "text-overline-color",
    "textOverlineMode": "text-overline-mode",
    "textOverlineStyle": "text-overline-style",
    "textOverlineWidth": "text-overline-width",
    "textRendering": "text-rendering",
    "textShadow": "text-shadow",
    "textTransform": "text-transform",
    "textUnderline": "text-underline",
    "textUnderlineColor": "text-underline-color",
    "textUnderlineMode": "text-underline-mode",
    "textUnderlineStyle": "text-underline-style",
    "textUnderlineWidth": "text-underline-width",
    "top": "top",
    "unicodeBidi": "unicode-bidi",
    "unicodeRange": "unicode-range",
    "vectorEffect": "vector-effect",
    "verticalAlign": "vertical-align",
    "visibility": "visibility",
    "whiteSpace": "white-space",
    "widows": "widows",
    "width": "width",
    "wordBreak": "word-break",
    "wordSpacing": "word-spacing",
    "wordWrap": "word-wrap",
    "writingMode": "writing-mode",
    "zIndex": "z-index",
    "zoom": "zoom",
  };

  
  var voidElems = {
    "area": true,
    "base": true,
    "br": true,
    "col": true,
    "command": true,
    "embed": true,
    "hr": true,
    "img": true,
    "input": true,
    "link": true,
    "meta": true,
    "param": true,
    "source": true,
  };

  var whitespace = [" ", "\t", "\n", "\r"];

  
  var nodeTypes = {
    ELEMENT_NODE: 1,
    ATTRIBUTE_NODE: 2,
    TEXT_NODE: 3,
    CDATA_SECTION_NODE: 4,
    ENTITY_REFERENCE_NODE: 5,
    ENTITY_NODE: 6,
    PROCESSING_INSTRUCTION_NODE: 7,
    COMMENT_NODE: 8,
    DOCUMENT_NODE: 9,
    DOCUMENT_TYPE_NODE: 10,
    DOCUMENT_FRAGMENT_NODE: 11,
    NOTATION_NODE: 12
  };

  function getElementsByTagName(tag) {
    tag = tag.toUpperCase();
    var elems = [];
    var allTags = (tag === "*");
    function getElems(node) {
      var length = node.children.length;
      for (var i = 0; i < length; i++) {
        var child = node.children[i];
        if (allTags || (child.tagName === tag))
          elems.push(child);
        getElems(child);
      }
    }
    getElems(this);
    return elems;
  }

  var Node = function () {};

  Node.prototype = {
    attributes: null,
    childNodes: null,
    localName: null,
    nodeName: null,
    parentNode: null,
    textContent: null,
    nextSibling: null,
    previousSibling: null,

    get firstChild() {
      return this.childNodes[0] || null;
    },

    get firstElementChild() {
      return this.children[0] || null;
    },

    get lastChild() {
      return this.childNodes[this.childNodes.length - 1] || null;
    },

    get lastElementChild() {
      return this.children[this.children.length - 1] || null;
    },

    appendChild: function (child) {
      if (child.parentNode) {
        child.parentNode.removeChild(child);
      }

      var last = this.lastChild;
      if (last)
        last.nextSibling = child;
      child.previousSibling = last;

      if (child.nodeType === Node.ELEMENT_NODE) {
        child.previousElementSibling = this.children[this.children.length - 1] || null;
        this.children.push(child);
        child.previousElementSibling && (child.previousElementSibling.nextElementSibling = child);
      }
      this.childNodes.push(child);
      child.parentNode = this;
    },

    removeChild: function (child) {
      var childNodes = this.childNodes;
      var childIndex = childNodes.indexOf(child);
      if (childIndex === -1) {
        throw "removeChild: node not found";
      } else {
        child.parentNode = null;
        var prev = child.previousSibling;
        var next = child.nextSibling;
        if (prev)
          prev.nextSibling = next;
        if (next)
          next.previousSibling = prev;

        if (child.nodeType === Node.ELEMENT_NODE) {
          prev = child.previousElementSibling;
          next = child.nextElementSibling;
          if (prev)
            prev.nextElementSibling = next;
          if (next)
            next.previousElementSibling = prev;
          this.children.splice(this.children.indexOf(child), 1);
        }

        child.previousSibling = child.nextSibling = null;
        child.previousElementSibling = child.nextElementSibling = null;

        return childNodes.splice(childIndex, 1)[0];
      }
    },

    replaceChild: function (newNode, oldNode) {
      var childNodes = this.childNodes;
      var childIndex = childNodes.indexOf(oldNode);
      if (childIndex === -1) {
        throw "replaceChild: node not found";
      } else {
        
        if (newNode.parentNode)
          newNode.parentNode.removeChild(newNode);

        childNodes[childIndex] = newNode;

        
        newNode.nextSibling = oldNode.nextSibling;
        newNode.previousSibling = oldNode.previousSibling;
        if (newNode.nextSibling)
          newNode.nextSibling.previousSibling = newNode;
        if (newNode.previousSibling)
          newNode.previousSibling.nextSibling = newNode;

        newNode.parentNode = this;

        
        
        if (newNode.nodeType === Node.ELEMENT_NODE) {
          if (oldNode.nodeType === Node.ELEMENT_NODE) {
            
            newNode.previousElementSibling = oldNode.previousElementSibling;
            newNode.nextElementSibling = oldNode.nextElementSibling;
            if (newNode.previousElementSibling)
              newNode.previousElementSibling.nextElementSibling = newNode;
            if (newNode.nextElementSibling)
              newNode.nextElementSibling.previousElementSibling = newNode;
            this.children[this.children.indexOf(oldNode)] = newNode;
          } else {
            
            newNode.previousElementSibling = (function() {
              for (var i = childIndex - 1; i >= 0; i--) {
                if (childNodes[i].nodeType === Node.ELEMENT_NODE)
                  return childNodes[i];
              }
              return null;
            })();
            if (newNode.previousElementSibling) {
              newNode.nextElementSibling = newNode.previousElementSibling.nextElementSibling;
            } else {
              newNode.nextElementSibling = (function() {
                for (var i = childIndex + 1; i < childNodes.length; i++) {
                  if (childNodes[i].nodeType === Node.ELEMENT_NODE)
                    return childNodes[i];
                }
                return null;
              })();
            }
            if (newNode.previousElementSibling)
              newNode.previousElementSibling.nextElementSibling = newNode;
            if (newNode.nextElementSibling)
              newNode.nextElementSibling.previousElementSibling = newNode;

            if (newNode.nextElementSibling)
              this.children.splice(this.children.indexOf(newNode.nextElementSibling), 0, newNode);
            else
              this.children.push(newNode);
          }
        } else {
          
          
          if (oldNode.nodeType === Node.ELEMENT_NODE) {
            if (oldNode.previousElementSibling)
              oldNode.previousElementSibling.nextElementSibling = oldNode.nextElementSibling;
            if (oldNode.nextElementSibling)
              oldNode.nextElementSibling.previousElementSibling = oldNode.previousElementSibling;
            this.children.splice(this.children.indexOf(oldNode), 1);
          }
          
          
        }


        oldNode.parentNode = null;
        oldNode.previousSibling = null;
        oldNode.nextSibling = null;
        if (oldNode.nodeType === Node.ELEMENT_NODE) {
          oldNode.previousElementSibling = null;
          oldNode.nextElementSibling = null;
        }
        return oldNode;
      }
    },

    __JSDOMParser__: true,
  };

  for (var i in nodeTypes) {
    Node[i] = Node.prototype[i] = nodeTypes[i];
  }

  var Attribute = function (name, value) {
    this.name = name;
    this._value = value;
  };

  Attribute.prototype = {
    get value() {
      return this._value;
    },
    setValue: function(newValue) {
      this._value = newValue;
      delete this._decodedValue;
    },
    setDecodedValue: function(newValue) {
      this._value = encodeHTML(newValue);
      this._decodedValue = newValue;
    },
    getDecodedValue: function() {
      if (typeof this._decodedValue === "undefined") {
        this._decodedValue = (this._value && decodeHTML(this._value)) || "";
      }
      return this._decodedValue;
    },
  };

  var Comment = function () {
    this.childNodes = [];
  };

  Comment.prototype = {
    __proto__: Node.prototype,

    nodeName: "#comment",
    nodeType: Node.COMMENT_NODE
  };

  var Text = function () {
    this.childNodes = [];
  };

  Text.prototype = {
    __proto__: Node.prototype,

    nodeName: "#text",
    nodeType: Node.TEXT_NODE,
    get textContent() {
      if (typeof this._textContent === "undefined") {
        this._textContent = decodeHTML(this._innerHTML || "");
      }
      return this._textContent;
    },
    get innerHTML() {
      if (typeof this._innerHTML === "undefined") {
        this._innerHTML = encodeHTML(this._textContent || "");
      }
      return this._innerHTML;
    },

    set innerHTML(newHTML) {
      this._innerHTML = newHTML;
      delete this._textContent;
    },
    set textContent(newText) {
      this._textContent = newText;
      delete this._innerHTML;
    },
  }

  var Document = function () {
    this.styleSheets = [];
    this.childNodes = [];
    this.children = [];
  };

  Document.prototype = {
    __proto__: Node.prototype,

    nodeName: "#document",
    nodeType: Node.DOCUMENT_NODE,
    title: "",

    getElementsByTagName: getElementsByTagName,

    getElementById: function (id) {
      function getElem(node) {
        var length = node.children.length;
        if (node.id === id)
          return node;
        for (var i = 0; i < length; i++) {
          var el = getElem(node.children[i]);
          if (el)
            return el;
        }
        return null;
      }
      return getElem(this);
    },

    createElement: function (tag) {
      var node = new Element(tag);
      return node;
    }
  };

  var Element = function (tag) {
    this.attributes = [];
    this.childNodes = [];
    this.children = [];
    this.nextElementSibling = this.previousElementSibling = null;
    this.localName = tag.toLowerCase();
    this.tagName = tag.toUpperCase();
    this.style = new Style(this);
  };

  Element.prototype = {
    __proto__: Node.prototype,

    nodeType: Node.ELEMENT_NODE,

    getElementsByTagName: getElementsByTagName,

    get className() {
      return this.getAttribute("class") || "";
    },

    set className(str) {
      this.setAttribute("class", str);
    },

    get id() {
      return this.getAttribute("id") || "";
    },

    set id(str) {
      this.setAttribute("id", str);
    },

    get href() {
      return this.getAttribute("href") || "";
    },

    set href(str) {
      this.setAttribute("href", str);
    },

    get src() {
      return this.getAttribute("src") || "";
    },

    set src(str) {
      this.setAttribute("src", str);
    },

    get nodeName() {
      return this.tagName;
    },

    get innerHTML() {
      function getHTML(node) {
        var i = 0;
        for (i = 0; i < node.childNodes.length; i++) {
          var child = node.childNodes[i];
          if (child.localName) {
            arr.push("<" + child.localName);

            
            for (var j = 0; j < child.attributes.length; j++) {
              var attr = child.attributes[j];
              
              var val = attr.value;
              var quote = (val.indexOf('"') === -1 ? '"' : "'");
              arr.push(" " + attr.name + '=' + quote + val + quote);
            }

            if (child.localName in voidElems) {
              
              arr.push(">");
            } else {
              
              arr.push(">");
              getHTML(child);
              arr.push("</" + child.localName + ">");
            }
          } else {
            
            arr.push(child.innerHTML);
          }
        }
      }

      
      
      var arr = [];
      getHTML(this);
      return arr.join("");
    },

    set innerHTML(html) {
      var parser = new JSDOMParser();
      var node = parser.parse(html);
      for (var i = this.childNodes.length; --i >= 0;) {
        this.childNodes[i].parentNode = null;
      }
      this.childNodes = node.childNodes;
      this.children = node.children;
      for (var i = this.childNodes.length; --i >= 0;) {
        this.childNodes[i].parentNode = this;
      }
    },

    set textContent(text) {
      
      for (var i = this.childNodes.length; --i >= 0;) {
        this.childNodes[i].parentNode = null;
      }

      var node = new Text();
      this.childNodes = [ node ];
      this.children = [];
      node.textContent = text;
      node.parentNode = this;
    },

    get textContent() {
      function getText(node) {
        var nodes = node.childNodes;
        for (var i = 0; i < nodes.length; i++) {
          var child = nodes[i];
          if (child.nodeType === 3) {
            text.push(child.textContent);
          } else {
            getText(child);
          }
        }
      }

      
      
      var text = [];
      getText(this);
      return text.join("");
    },

    getAttribute: function (name) {
      for (var i = this.attributes.length; --i >= 0;) {
        var attr = this.attributes[i];
        if (attr.name === name)
          return attr.getDecodedValue();
      }
      return undefined;
    },

    setAttribute: function (name, value) {
      for (var i = this.attributes.length; --i >= 0;) {
        var attr = this.attributes[i];
        if (attr.name === name) {
          attr.setDecodedValue(value);
          return;
        }
      }
      this.attributes.push(new Attribute(name, encodeHTML(value)));
    },

    removeAttribute: function (name) {
      for (var i = this.attributes.length; --i >= 0;) {
        var attr = this.attributes[i];
        if (attr.name === name) {
          this.attributes.splice(i, 1);
          break;
        }
      }
    }
  };

  var Style = function (node) {
    this.node = node;
  };

  
  
  
  
  
  Style.prototype = {
    getStyle: function (styleName) {
      var attr = this.node.getAttribute("style");
      if (!attr)
        return undefined;

      var styles = attr.split(";");
      for (var i = 0; i < styles.length; i++) {
        var style = styles[i].split(":");
        var name = style[0].trim();
        if (name === styleName)
          return style[1].trim();
      }

      return undefined;
    },

    setStyle: function (styleName, styleValue) {
      var value = this.node.getAttribute("style") || "";
      var index = 0;
      do {
        var next = value.indexOf(";", index) + 1;
        var length = next - index - 1;
        var style = (length > 0 ? value.substr(index, length) : value.substr(index));
        if (style.substr(0, style.indexOf(":")).trim() === styleName) {
          value = value.substr(0, index).trim() + (next ? " " + value.substr(next).trim() : "");
          break;
        }
        index = next;
      } while (index);

      value += " " + styleName + ": " + styleValue + ";";
      this.node.setAttribute("style", value.trim());
    }
  };

  
  
  for (var jsName in styleMap) {
    (function (cssName) {
      Style.prototype.__defineGetter__(jsName, function () {
        return this.getStyle(cssName);
      });
      Style.prototype.__defineSetter__(jsName, function (value) {
        this.setStyle(cssName, value);
      });
    }) (styleMap[jsName]);
  }

  var JSDOMParser = function () {
    this.currentChar = 0;

    
    
    
    
    
    
    this.strBuf = [];

    
    
    
    this.retPair = [];
  };

  JSDOMParser.prototype = {
    


    peekNext: function () {
      return this.html[this.currentChar];
    },

    


    nextChar: function () {
      return this.html[this.currentChar++];
    },

    



    readString: function (quote) {
      var str;
      var n = this.html.indexOf(quote, this.currentChar);
      if (n === -1) {
        this.currentChar = this.html.length;
        str = null;
      } else {
        str = this.html.substring(this.currentChar, n);
        this.currentChar = n + 1;
      }

      return str;
    },

    



    readAttribute: function (node) {
      var name = "";

      var n = this.html.indexOf("=", this.currentChar);
      if (n === -1) {
        this.currentChar = this.html.length;
      } else {
        
        name = this.html.substring(this.currentChar, n);
        this.currentChar = n + 1;
      }

      if (!name)
        return;

      
      var c = this.nextChar();
      if (c !== '"' && c !== "'") {
        error("Error reading attribute " + name + ", expecting '\"'");
        return;
      }

      
      var value = this.readString(c);

      node.attributes.push(new Attribute(name, value));

      return;
    },

    







    makeElementNode: function (retPair) {
      var c = this.nextChar();

      
      var strBuf = this.strBuf;
      strBuf.length = 0;
      while (whitespace.indexOf(c) == -1 && c !== ">" && c !== "/") {
        if (c === undefined)
          return false;
        strBuf.push(c);
        c = this.nextChar();
      }
      var tag = strBuf.join('');

      if (!tag)
        return false;

      var node = new Element(tag);

      
      while (c !== "/" && c !== ">") {
        if (c === undefined)
          return false;
        while (whitespace.indexOf(this.html[this.currentChar++]) != -1);
        this.currentChar--;
        c = this.nextChar();
        if (c !== "/" && c !== ">") {
          --this.currentChar;
          this.readAttribute(node);
        }
      }

      
      var closed = tag in voidElems;
      if (c === "/") {
        closed = true;
        c = this.nextChar();
        if (c !== ">") {
          error("expected '>' to close " + tag);
          return false;
        }
      }

      retPair[0] = node;
      retPair[1] = closed;
      return true
    },

    





    match: function (str) {
      var strlen = str.length;
      if (this.html.substr(this.currentChar, strlen).toLowerCase() === str.toLowerCase()) {
        this.currentChar += strlen;
        return true;
      }
      return false;
    },

    



    discardTo: function (str) {
      var index = this.html.indexOf(str, this.currentChar) + str.length;
      if (index === -1)
        this.currentChar = this.html.length;
      this.currentChar = index;
    },

    


    readChildren: function (node) {
      var child;
      while ((child = this.readNode())) {
        
        if (child.nodeType !== 8) {
          node.appendChild(child);
        }
      }
    },

    readScript: function (node) {
      while (this.currentChar < this.html.length) {
        var c = this.nextChar();
        var nextC = this.peekNext();
        if (c === "<") {
          if (nextC === "!" || nextC === "?") {
            
            this.currentChar++;
            node.appendChild(this.discardNextComment());
            continue;
          }
          if (nextC === "/" && this.html.substr(this.currentChar, 8 ).toLowerCase() == "/script>") {
            
            this.currentChar--;
            
            return;
          }
        }
        
        
        

        var haveTextNode = node.lastChild && node.lastChild.nodeType === Node.TEXT_NODE;
        var textNode = haveTextNode ? node.lastChild : new Text();
        var n = this.html.indexOf("<", this.currentChar);
        
        
        this.currentChar--;
        if (n === -1) {
          textNode.innerHTML += this.html.substring(this.currentChar, this.html.length);
          this.currentChar = this.html.length;
        } else {
          textNode.innerHTML += this.html.substring(this.currentChar, n);
          this.currentChar = n;
        }
        if (!haveTextNode)
          node.appendChild(textNode);
      }
    },

    discardNextComment: function() {
      if (this.match("--")) {
        this.discardTo("-->");
      } else {
        var c = this.nextChar();
        while (c !== ">") {
          if (c === undefined)
            return null;
          if (c === '"' || c === "'")
            this.readString(c);
          c = this.nextChar();
        }
      }
      return new Comment();
    },


    





    readNode: function () {
      var c = this.nextChar();

      if (c === undefined)
        return null;

      
      if (c !== "<") {
        --this.currentChar;
        var node = new Text();
        var n = this.html.indexOf("<", this.currentChar);
        if (n === -1) {
          node.innerHTML = this.html.substring(this.currentChar, this.html.length);
          this.currentChar = this.html.length;
        } else {
          node.innerHTML = this.html.substring(this.currentChar, n);
          this.currentChar = n;
        }
        return node;
      }

      c = this.peekNext();

      
      
      
      
      if (c === "!" || c === "?") {
        
        this.currentChar++;
        return this.discardNextComment();
      }

      
      
      if (c === "/") {
        --this.currentChar;
        return null;
      }

      
      var result = this.makeElementNode(this.retPair);
      if (!result)
        return null;

      var node = this.retPair[0];
      var closed = this.retPair[1];
      var localName = node.localName;

      
      if (!closed) {
        if (localName == "script") {
          this.readScript(node);
        } else {
          this.readChildren(node);
        }
        var closingTag = "</" + localName + ">";
        if (!this.match(closingTag)) {
          error("expected '" + closingTag + "'");
          return null;
        }
      }

      
      
      
      if (localName === "title" && !this.doc.title) {
        this.doc.title = node.textContent.trim();
      } else if (localName === "head") {
        this.doc.head = node;
      } else if (localName === "body") {
        this.doc.body = node;
      } else if (localName === "html") {
        this.doc.documentElement = node;
      }

      return node;
    },

    


    parse: function (html) {
      this.html = html;
      var doc = this.doc = new Document();
      this.readChildren(doc);

      
      
      if (doc.documentElement) {
        for (var i = doc.childNodes.length; --i >= 0;) {
          var child = doc.childNodes[i];
          if (child !== doc.documentElement) {
            doc.removeChild(child);
          }
        }
      }

      return doc;
    }
  };

  
  global.Node = Node;
  global.Comment = Comment;
  global.Document = Document;
  global.Element = Element;
  global.Text = Text;

  
  global.JSDOMParser = JSDOMParser;

}) (this);
