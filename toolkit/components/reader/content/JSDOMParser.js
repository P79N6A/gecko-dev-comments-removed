

























(function (global) {

  function error(m) {
    dump("JSDOMParser error: " + m);
  }

  
  let styleMap = {
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

  
  let voidElems = {
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

  
  let nodeTypes = {
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
    let elems = [];
    let allTags = (tag === "*");
    function getElems(node) {
      let length = node.childNodes.length;
      for (let i = 0; i < length; i++) {
        let child = node.childNodes[i];
        if (child.nodeType !== 1)
          continue;
        if (allTags || (child.tagName === tag))
          elems.push(child);
        getElems(child);
      }
    }
    getElems(this);
    return elems;
  }

  let Node = function () {};

  Node.prototype = {
    attributes: null,
    childNodes: null,
    localName: null,
    nodeName: null,
    parentNode: null,
    textContent: null,

    get firstChild() {
      return this.childNodes[0] || null;
    },

    get nextSibling() {
      if (this.parentNode) {
        let childNodes = this.parentNode.childNodes;
        return childNodes[childNodes.indexOf(this) + 1] || null;
      }

      return null;
    },

    appendChild: function (child) {
      if (child.parentNode) {
        child.parentNode.removeChild(child);
      }

      this.childNodes.push(child);
      child.parentNode = this;
    },

    removeChild: function (child) {
      let childNodes = this.childNodes;
      let childIndex = childNodes.indexOf(child);
      if (childIndex === -1) {
        throw "removeChild: node not found";
      } else {
        child.parentNode = null;
        return childNodes.splice(childIndex, 1)[0];
      }
    },

    replaceChild: function (newNode, oldNode) {
      let childNodes = this.childNodes;
      let childIndex = childNodes.indexOf(oldNode);
      if (childIndex === -1) {
        throw "replaceChild: node not found";
      } else {
        if (newNode.parentNode)
          newNode.parentNode.removeChild(newNode);

        childNodes[childIndex] = newNode;
        newNode.parentNode = this;
        oldNode.parentNode = null;
        return oldNode;
      }
    }
  };

  for (let i in nodeTypes) {
    Node[i] = Node.prototype[i] = nodeTypes[i];
  }

  let Attribute = function (name, value) {
    this.name = name;
    this.value = value;
  };

  let Comment = function () {
    this.childNodes = [];
  };

  Comment.prototype = {
    __proto__: Node.prototype,

    nodeName: "#comment",
    nodeType: Node.COMMENT_NODE
  };

  let Text = function () {
    this.childNodes = [];
  };

  Text.prototype = {
    __proto__: Node.prototype,

    nodeName: "#text",
    nodeType: Node.TEXT_NODE,
    textContent: ""
  }

  let Document = function () {
    this.styleSheets = [];
    this.childNodes = [];
  };

  Document.prototype = {
    __proto__: Node.prototype,

    nodeName: "#document",
    nodeType: Node.DOCUMENT_NODE,
    title: "",

    getElementsByTagName: getElementsByTagName,

    getElementById: function (id) {
      function getElem(node) {
        let length = node.childNodes.length;
        if (node.id === id)
          return node;
        for (let i = 0; i < length; i++) {
          let el = getElem(node.childNodes[i]);
          if (el)
            return el;
        }
        return null;
      }
      return getElem(this);
    },

    createElement: function (tag) {
      let node = new Element(tag);
      return node;
    }
  };

  let Element = function (tag) {
    this.attributes = [];
    this.childNodes = [];
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
        let i = 0;
        for (i = 0; i < node.childNodes.length; i++) {
          let child = node.childNodes[i];
          if (child.localName) {
            arr.push("<" + child.localName);

            
            for (let j = 0; j < child.attributes.length; j++) {
              let attr = child.attributes[j];
              let quote = (attr.value.indexOf('"') === -1 ? '"' : "'");
              arr.push(" " + attr.name + '=' + quote + attr.value + quote);
            }

            if (child.localName in voidElems) {
              
              arr.push("/>");
            } else {
              
              arr.push(">");
              getHTML(child);
              arr.push("</" + child.localName + ">");
            }
          } else {
            arr.push(child.textContent);
          }
        }
      }

      
      
      let arr = [];
      getHTML(this);
      return arr.join("");
    },

    set innerHTML(html) {
      let parser = new JSDOMParser();
      let node = parser.parse(html);
      for (let i = this.childNodes.length; --i >= 0;) {
        this.childNodes[i].parentNode = null;
      }
      this.childNodes = node.childNodes;
      for (let i = this.childNodes.length; --i >= 0;) {
        this.childNodes[i].parentNode = this;
      }
    },

    set textContent(text) {
      
      for (let i = this.childNodes.length; --i >= 0;) {
        this.childNodes[i].parentNode = null;
      }

      let node = new Text();
      this.childNodes = [ node ];
      node.textContent = text;
      node.parentNode = this;
    },

    get textContent() {
      function getText(node) {
        let nodes = node.childNodes;
        for (let i = 0; i < nodes.length; i++) {
          let child = nodes[i];
          if (child.nodeType === 3) {
            text.push(child.textContent);
          } else {
            getText(child);
          }
        }
      }

      
      
      let text = [];
      getText(this);
      return text.join("");
    },

    getAttribute: function (name) {
      for (let i = this.attributes.length; --i >= 0;) {
        let attr = this.attributes[i];
        if (attr.name === name)
          return attr.value;
      }
      return undefined;
    },

    setAttribute: function (name, value) {
      for (let i = this.attributes.length; --i >= 0;) {
        let attr = this.attributes[i];
        if (attr.name === name) {
          attr.value = value;
          return;
        }
      }
      this.attributes.push(new Attribute(name, value));
    },

    removeAttribute: function (name) {
      for (let i = this.attributes.length; --i >= 0;) {
        let attr = this.attributes[i];
        if (attr.name === name) {
          this.attributes.splice(i, 1);
          break;
        }
      }
    }
  };

  let Style = function (node) {
    this.node = node;
  };

  
  
  
  
  
  Style.prototype = {
    getStyle: function (styleName) {
      let attr = this.node.getAttribute("style");
      if (!attr)
        return undefined;

      let styles = attr.split(";");
      for (let i = 0; i < styles.length; i++) {
        let style = styles[i].split(":");
        let name = style[0].trim();
        if (name === styleName)
          return style[1].trim();
      }

      return undefined;
    },

    setStyle: function (styleName, styleValue) {
      let value = this.node.getAttribute("style") || "";
      let index = 0;
      do {
        let next = value.indexOf(";", index) + 1;
        let length = next - index - 1;
        let style = (length > 0 ? value.substr(index, length) : value.substr(index));
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

  
  
  for (let jsName in styleMap) {
    (function (cssName) {
      Style.prototype.__defineGetter__(jsName, function () {
        return this.getStyle(cssName);
      });
      Style.prototype.__defineSetter__(jsName, function (value) {
        this.setStyle(cssName, value);
      });
    }) (styleMap[jsName]);
  }

  let JSDOMParser = function () {
    this.currentChar = 0;
  };

  JSDOMParser.prototype = {
    


    peekNext: function () {
      return this.html[this.currentChar];
    },

    


    nextChar: function () {
      return this.html[this.currentChar++];
    },

    



    readString: function (quote) {
      let str;
      let n = this.html.indexOf(quote, this.currentChar);
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
      let name = "";

      let n = this.html.indexOf("=", this.currentChar);
      if (n === -1) {
        this.currentChar = this.html.length;
      } else {
        
        name = this.html.substring(this.currentChar, n);
        this.currentChar = n + 1;
      }

      if (!name)
        return;

      
      let c = this.nextChar();
      if (c !== '"' && c !== "'") {
        error("expecting '\"'");
        return;
      }

      
      let value = this.readString(c);

      if (!value)
        return;

      node.attributes.push(new Attribute(name, value));

      return;
    },

    







    makeElementNode: function () {
      let c = this.nextChar();

      
      let tag = "";
      while (c !== " " && c !== ">" && c !== "/") {
        if (c === undefined)
          return null;
        tag += c;
        c = this.nextChar();
      }

      if (!tag)
        return null;

      let node = new Element(tag);

      
      while (c !== "/" && c !== ">") {
        if (c === undefined)
          return null;
        while (this.match(" "));
        c = this.nextChar();
        if (c !== "/" && c !== ">") {
          --this.currentChar;
          this.readAttribute(node);
        }
      }

      
      let closed = tag in voidElems;
      if (c === "/") {
        closed = true;
        c = this.nextChar();
        if (c !== ">") {
          error("expected '>'");
          return null;
        }
      }

      return [node, closed];
    },

    





    match: function (str) {
      let strlen = str.length;
      if (this.html.substr(this.currentChar, strlen) === str) {
        this.currentChar += strlen;
        return true;
      }
      return false;
    },

    



    discardTo: function (str) {
      let index = this.html.indexOf(str, this.currentChar) + str.length;
      if (index === -1)
        this.currentChar = this.html.length;
      this.currentChar = index;
    },

    


    readChildren: function (node) {
      let child;
      while ((child = this.readNode())) {
        
        if (child.nodeType !== 8) {
          node.childNodes.push(child);
          child.parentNode = node;
        }
      }
    },

    





    readNode: function () {
      let c = this.nextChar();
 
      if (c === undefined)
        return null;

      
      if (c !== "<") {
        --this.currentChar;
        let node = new Text();
        let n = this.html.indexOf("<", this.currentChar);
        if (n === -1) {
          node.textContent = this.html.substring(this.currentChar, this.html.length);
          this.currentChar = this.html.length;
        } else {
          node.textContent = this.html.substring(this.currentChar, n);
          this.currentChar = n;
        }
        return node;
      }

      c = this.peekNext();

      
      
      
      
      if (c === "!" || c === "?") {
        this.currentChar++;
        if (this.match("--")) {
          this.discardTo("-->");
        } else {
          let c = this.nextChar();
          while (c !== ">") {
            if (c === undefined)
              return null;
            if (c === '"' || c === "'")
              this.readString(c);
            c = this.nextChar();
          }
        }
        return new Comment();
      }

      
      
      if (c === "/") {
        --this.currentChar;
        return null;
      }

      
      let result = this.makeElementNode();
      if (result === null)
        return null;

      let [node, closed] = result;
      let localName = node.localName;

      
      if (!closed) {
        this.readChildren(node);
        let closingTag = "</" + localName + ">";
        if (!this.match(closingTag)) {
          error("expected '" + closingTag + "'");
          return null;
        }
      }

      if (localName === "title") {
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
      let doc = this.doc = new Document();
      this.readChildren(doc);

      
      
      if (doc.documentElement) {
        for (let i = doc.childNodes.length; --i >= 0;) {
          let child = doc.childNodes[i];
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
