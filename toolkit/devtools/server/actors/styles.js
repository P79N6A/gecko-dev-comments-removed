



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const Services = require("Services");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;
const events = require("sdk/event/core");
const object = require("sdk/util/object");
const {Class} = require("sdk/core/heritage");
const {LongStringActor} = require("devtools/server/actors/string");



require("devtools/server/actors/stylesheets");

loader.lazyGetter(this, "CssLogic", () => require("devtools/styleinspector/css-logic").CssLogic);
loader.lazyGetter(this, "DOMUtils", () => Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils));




const ELEMENT_STYLE = 100;
exports.ELEMENT_STYLE = ELEMENT_STYLE;

const PSEUDO_ELEMENTS = [":first-line", ":first-letter", ":before", ":after", ":-moz-selection"];
exports.PSEUDO_ELEMENTS = PSEUDO_ELEMENTS;



const PSEUDO_ELEMENTS_TO_READ = PSEUDO_ELEMENTS.filter(pseudo => {
  return pseudo !== ":before" && pseudo !== ":after";
});

const XHTML_NS = "http://www.w3.org/1999/xhtml";
const FONT_PREVIEW_TEXT = "Abc";
const FONT_PREVIEW_FONT_SIZE = 40;
const FONT_PREVIEW_FILLSTYLE = "black";
const NORMAL_FONT_WEIGHT = 400;
const BOLD_FONT_WEIGHT = 700;


types.addActorType("domnode");


types.addActorType("domstylerule");





types.addLifetime("walker", "walker");






types.addDictType("appliedstyle", {
  rule: "domstylerule#actorid",
  inherited: "nullable:domnode#actorid",
  keyframes: "nullable:domstylerule#actorid"
});

types.addDictType("matchedselector", {
  rule: "domstylerule#actorid",
  selector: "string",
  value: "string",
  status: "number"
});

types.addDictType("appliedStylesReturn", {
  entries: "array:appliedstyle",
  rules: "array:domstylerule",
  sheets: "array:stylesheet"
});

types.addDictType("fontpreview", {
  data: "nullable:longstring",
  size: "json"
});

types.addDictType("fontface", {
  name: "string",
  CSSFamilyName: "string",
  rule: "nullable:domstylerule",
  srcIndex: "number",
  URI: "string",
  format: "string",
  preview: "nullable:fontpreview",
  localName: "string",
  metadata: "string"
});





var PageStyleActor = protocol.ActorClass({
  typeName: "pagestyle",

  







  initialize: function(inspector) {
    protocol.Actor.prototype.initialize.call(this, null);
    this.inspector = inspector;
    if (!this.inspector.walker) {
      throw Error("The inspector's WalkerActor must be created before " +
                   "creating a PageStyleActor.");
    }
    this.walker = inspector.walker;
    this.cssLogic = new CssLogic;

    
    this.refMap = new Map;

    this.onFrameUnload = this.onFrameUnload.bind(this);
    events.on(this.inspector.tabActor, "will-navigate", this.onFrameUnload);
  },

  get conn() this.inspector.conn,

  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    return {
      actor: this.actorID,
      traits: {
        
        
        
        
        getAppliedCreatesStyleCache: true
      }
    };
  },

  



  _styleRef: function(item) {
    if (this.refMap.has(item)) {
      return this.refMap.get(item);
    }
    let actor = StyleRuleActor(this, item);
    this.manage(actor);
    this.refMap.set(item, actor);

    return actor;
  },

  






  _sheetRef: function(sheet) {
    let tabActor = this.inspector.tabActor;
    let actor = tabActor.createStyleSheetActor(sheet);
    return actor;
  },

  























  getComputed: method(function(node, options) {
    let ret = Object.create(null);

    this.cssLogic.sourceFilter = options.filter || CssLogic.FILTER.UA;
    this.cssLogic.highlight(node.rawNode);
    let computed = this.cssLogic.computedStyle || [];

    Array.prototype.forEach.call(computed, name => {
      let matched = undefined;
      ret[name] = {
        value: computed.getPropertyValue(name),
        priority: computed.getPropertyPriority(name) || undefined
      };
    });

    if (options.markMatched || options.onlyMatched) {
      let matched = this.cssLogic.hasMatchedSelectors(Object.keys(ret));
      for (let key in ret) {
        if (matched[key]) {
          ret[key].matched = options.markMatched ? true : undefined
        } else if (options.onlyMatched) {
          delete ret[key];
        }
      }
    }

    return ret;
  }, {
    request: {
      node: Arg(0, "domnode"),
      markMatched: Option(1, "boolean"),
      onlyMatched: Option(1, "boolean"),
      filter: Option(1, "string"),
    },
    response: {
      computed: RetVal("json")
    }
  }),

  










  getAllUsedFontFaces: method(function(options) {
    let windows = this.inspector.tabActor.windows;
    let fontsList = [];
    for(let win of windows){
      fontsList = [...fontsList,
                   ...this.getUsedFontFaces(win.document.body, options)];
    }
    return fontsList;
  },
  {
    request: {
      includePreviews: Option(0, "boolean"),
      previewText: Option(0, "string"),
      previewFontSize: Option(0, "string"),
      previewFillStyle: Option(0, "string")
    },
    response: {
      fontFaces: RetVal("array:fontface")
    }
  }),

  












  getUsedFontFaces: method(function(node, options) {
    
    let actualNode = node.rawNode || node;
    let contentDocument = actualNode.ownerDocument;
    
    let rng = contentDocument.createRange();
    rng.selectNodeContents(actualNode);
    let fonts = DOMUtils.getUsedFontFaces(rng);
    let fontsArray = [];

    for (let i = 0; i < fonts.length; i++) {
      let font = fonts.item(i);
      let fontFace = {
        name: font.name,
        CSSFamilyName: font.CSSFamilyName,
        srcIndex: font.srcIndex,
        URI: font.URI,
        format: font.format,
        localName: font.localName,
        metadata: font.metadata
      }

      
      if (font.rule) {
        fontFace.rule = StyleRuleActor(this, font.rule);
        fontFace.ruleText = font.rule.cssText;
      }

      
      let weight = NORMAL_FONT_WEIGHT, style = "";
      if (font.rule) {
        weight = font.rule.style.getPropertyValue("font-weight")
                 || NORMAL_FONT_WEIGHT;
        if (weight == "bold") {
          weight = BOLD_FONT_WEIGHT;
        } else if (weight == "normal") {
          weight = NORMAL_FONT_WEIGHT;
        }
        style = font.rule.style.getPropertyValue("font-style") || "";
      }
      fontFace.weight = weight;
      fontFace.style = style;

      if (options.includePreviews) {
        let opts = {
          previewText: options.previewText,
          previewFontSize: options.previewFontSize,
          fontStyle: weight + " " + style,
          fillStyle: options.previewFillStyle
        }
        let { dataURL, size } = getFontPreviewData(font.CSSFamilyName,
                                                   contentDocument, opts);
        fontFace.preview = {
          data: LongStringActor(this.conn, dataURL),
          size: size
        };
      }
      fontsArray.push(fontFace);
    }

    
    fontsArray.sort(function(a, b) {
      return a.weight > b.weight ? 1 : -1;
    });
    fontsArray.sort(function(a, b) {
      if (a.CSSFamilyName == b.CSSFamilyName) {
        return 0;
      }
      return a.CSSFamilyName > b.CSSFamilyName ? 1 : -1;
    });
    fontsArray.sort(function(a, b) {
      if ((a.rule && b.rule) || (!a.rule && !b.rule)) {
        return 0;
      }
      return !a.rule && b.rule ? 1 : -1;
    });

    return fontsArray;
  }, {
    request: {
      node: Arg(0, "domnode"),
      includePreviews: Option(1, "boolean"),
      previewText: Option(1, "string"),
      previewFontSize: Option(1, "string"),
      previewFillStyle: Option(1, "string")
    },
    response: {
      fontFaces: RetVal("array:fontface")
    }
  }),

  



































  getMatchedSelectors: method(function(node, property, options) {
    this.cssLogic.sourceFilter = options.filter || CssLogic.FILTER.UA;
    this.cssLogic.highlight(node.rawNode);

    let walker = node.parent();

    let rules = new Set;
    let sheets = new Set;

    let matched = [];
    let propInfo = this.cssLogic.getPropertyInfo(property);
    for (let selectorInfo of propInfo.matchedSelectors) {
      let cssRule = selectorInfo.selector.cssRule;
      let domRule = cssRule.sourceElement || cssRule.domRule;

      let rule = this._styleRef(domRule);
      rules.add(rule);

      matched.push({
        rule: rule,
        sourceText: this.getSelectorSource(selectorInfo, node.rawNode),
        selector: selectorInfo.selector.text,
        name: selectorInfo.property,
        value: selectorInfo.value,
        status: selectorInfo.status
      });
    }

    this.expandSets(rules, sheets);

    return {
      matched: matched,
      rules: [...rules],
      sheets: [...sheets],
    };
  }, {
    request: {
      node: Arg(0, "domnode"),
      property: Arg(1, "string"),
      filter: Option(2, "string")
    },
    response: RetVal(types.addDictType("matchedselectorresponse", {
      rules: "array:domstylerule",
      sheets: "array:stylesheet",
      matched: "array:matchedselector"
    }))
  }),

  
  
  getSelectorSource: function(selectorInfo, relativeTo) {
    let result = selectorInfo.selector.text;
    if (selectorInfo.elementStyle) {
      let source = selectorInfo.sourceElement;
      if (source === relativeTo) {
        result = "this";
      } else {
        result = CssLogic.getShortName(source);
      }
      result += ".style"
    }
    return result;
  },

  











  getApplied: method(function(node, options) {
    if (!node) {
      return {entries: [], rules: [], sheets: []};
    }

    this.cssLogic.highlight(node.rawNode);
    let entries = [];
    entries = entries.concat(this._getAllElementRules(node, undefined, options));
    return this.getAppliedProps(node, entries, options);
  }, {
    request: {
      node: Arg(0, "domnode"),
      inherited: Option(1, "boolean"),
      matchedSelectors: Option(1, "boolean"),
      filter: Option(1, "string")
    },
    response: RetVal("appliedStylesReturn")
  }),

  _hasInheritedProps: function(style) {
    return Array.prototype.some.call(style, prop => {
      return DOMUtils.isInheritedProperty(prop);
    });
  },

  













  _getAllElementRules: function(node, inherited, options) {
    let {bindingElement, pseudo} = CssLogic.getBindingElementAndPseudo(node.rawNode);
    let rules = [];

    if (!bindingElement || !bindingElement.style) {
      return rules;
    }

    let elementStyle = this._styleRef(bindingElement);
    let showElementStyles = !inherited && !pseudo;
    let showInheritedStyles = inherited && this._hasInheritedProps(bindingElement.style);

    
    if (showElementStyles) {
      rules.push({
        rule: elementStyle,
      });
    }

    
    if (showInheritedStyles) {
      rules.push({
        rule: elementStyle,
        inherited: inherited
      });
    }

    
    
    
    this._getElementRules(bindingElement, pseudo, inherited, options).forEach((rule) => {
      
      
      
      rule.pseudoElement = null;
      rules.push(rule);
    });

    
    
    if (showElementStyles) {
      for (let pseudo of PSEUDO_ELEMENTS_TO_READ) {
        this._getElementRules(bindingElement, pseudo, inherited, options).forEach((rule) => {
          rules.push(rule);
        });
      }
    }

    return rules;
  },

  









  _getElementRules: function (node, pseudo, inherited, options) {
    let domRules = DOMUtils.getCSSStyleRules(node, pseudo);
    if (!domRules) {
      return [];
    }

    let rules = [];

    
    
    for (let i = domRules.Count() - 1; i >= 0; i--) {
      let domRule = domRules.GetElementAt(i);

      let isSystem = !CssLogic.isContentStylesheet(domRule.parentStyleSheet);

      if (isSystem && options.filter != CssLogic.FILTER.UA) {
        continue;
      }

      if (inherited) {
        
        
        let hasInherited = [...domRule.style].some(
          prop => DOMUtils.isInheritedProperty(prop)
        );
        if (!hasInherited) {
          continue;
        }
      }

      let ruleActor = this._styleRef(domRule);
      rules.push({
        rule: ruleActor,
        inherited: inherited,
        isSystem: isSystem,
        pseudoElement: pseudo
      });
    }
    return rules;
  },


  




















  getAppliedProps: function(node, entries, options) {
    if (options.inherited) {
      let parent = this.walker.parentNode(node);
      while (parent && parent.rawNode.nodeType != Ci.nsIDOMNode.DOCUMENT_NODE) {
        entries = entries.concat(this._getAllElementRules(parent, parent, options));
        parent = this.walker.parentNode(parent);
      }
    }

    if (options.matchedSelectors) {
      for (let entry of entries) {
        if (entry.rule.type === ELEMENT_STYLE) {
          continue;
        }

        let domRule = entry.rule.rawRule;
        let selectors = CssLogic.getSelectors(domRule);
        let element = entry.inherited ? entry.inherited.rawNode : node.rawNode;

        let {bindingElement,pseudo} = CssLogic.getBindingElementAndPseudo(element);
        entry.matchedSelectors = [];
        for (let i = 0; i < selectors.length; i++) {
          if (DOMUtils.selectorMatchesElement(bindingElement, domRule, i, pseudo)) {
            entry.matchedSelectors.push(selectors[i]);
          }
        }
      }
    }

    
    let computedStyle = this.cssLogic.computedStyle;
    if (computedStyle) {
      let animationNames = computedStyle.animationName.split(",");
      animationNames = animationNames.map(name => name.trim());

      if (animationNames) {
        
        
        for (let keyframesRule of this.cssLogic.keyframesRules) {
          if (animationNames.indexOf(keyframesRule.name) > -1) {
            for (let rule of keyframesRule.cssRules) {
              entries.push({
                rule: this._styleRef(rule),
                keyframes: this._styleRef(keyframesRule)
              });
            }
          }
        }
      }
    }

    let rules = new Set;
    let sheets = new Set;
    entries.forEach(entry => rules.add(entry.rule));
    this.expandSets(rules, sheets);

    return {
      entries: entries,
      rules: [...rules],
      sheets: [...sheets]
    }
  },

  


  expandSets: function(ruleSet, sheetSet) {
    
    for (let rule of ruleSet) {
      if (rule.rawRule.parentRule) {
        let parent = this._styleRef(rule.rawRule.parentRule);
        if (!ruleSet.has(parent)) {
          ruleSet.add(parent);
        }
      }
      if (rule.rawRule.parentStyleSheet) {
        let parent = this._sheetRef(rule.rawRule.parentStyleSheet);
        if (!sheetSet.has(parent)) {
          sheetSet.add(parent);
        }
      }
    }

    for (let sheet of sheetSet) {
      if (sheet.rawSheet.parentStyleSheet) {
        let parent = this._sheetRef(sheet.rawSheet.parentStyleSheet);
        if (!sheetSet.has(parent)) {
          sheetSet.add(parent);
        }
      }
    }
  },

  getLayout: method(function(node, options) {
    this.cssLogic.highlight(node.rawNode);

    let layout = {};

    
    

    let clientRect = node.rawNode.getBoundingClientRect();
    layout.width = parseFloat(clientRect.width.toPrecision(6));
    layout.height = parseFloat(clientRect.height.toPrecision(6));

    
    let style = CssLogic.getComputedStyle(node.rawNode);
    for (let prop of [
      "position",
      "margin-top",
      "margin-right",
      "margin-bottom",
      "margin-left",
      "padding-top",
      "padding-right",
      "padding-bottom",
      "padding-left",
      "border-top-width",
      "border-right-width",
      "border-bottom-width",
      "border-left-width"
    ]) {
      layout[prop] = style.getPropertyValue(prop);
    }

    if (options.autoMargins) {
      layout.autoMargins = this.processMargins(this.cssLogic);
    }

    for (let i in this.map) {
      let property = this.map[i].property;
      this.map[i].value = parseFloat(style.getPropertyValue(property));
    }


    if (options.margins) {
      layout.margins = this.processMargins(cssLogic);
    }

    return layout;
  }, {
    request: {
      node: Arg(0, "domnode"),
      autoMargins: Option(1, "boolean")
    },
    response: RetVal("json")
  }),

  


  processMargins: function(cssLogic) {
    let margins = {};

    for (let prop of ["top", "bottom", "left", "right"]) {
      let info = cssLogic.getPropertyInfo("margin-" + prop);
      let selectors = info.matchedSelectors;
      if (selectors && selectors.length > 0 && selectors[0].value == "auto") {
        margins[prop] = "auto";
      }
    }

    return margins;
  },

  


  onFrameUnload: function() {
    this._styleElement = null;
  },

  



  get styleElement() {
    if (!this._styleElement) {
      let document = this.inspector.window.document;
      let style = document.createElementNS("http://www.w3.org/1999/xhtml", "style");
      style.setAttribute("type", "text/css");
      document.documentElement.appendChild(style);
      this._styleElement = style;
    }

    return this._styleElement;
  },

  




  addNewRule: method(function(node) {
    let style = this.styleElement;
    let sheet = style.sheet;
    let rawNode = node.rawNode;

    let selector;
    if (rawNode.id) {
      selector = "#" + rawNode.id;
    } else if (rawNode.className) {
      selector = "." + rawNode.className.split(" ").join(".");
    } else {
      selector = rawNode.tagName.toLowerCase();
    }

    let index = sheet.insertRule(selector + " {}", sheet.cssRules.length);
    let ruleActor = this._styleRef(sheet.cssRules[index]);
    return this.getAppliedProps(node, [{ rule: ruleActor }],
      { matchedSelectors: true });
  }, {
    request: {
      node: Arg(0, "domnode")
    },
    response: RetVal("appliedStylesReturn")
  }),

});
exports.PageStyleActor = PageStyleActor;




var PageStyleFront = protocol.FrontClass(PageStyleActor, {
  initialize: function(conn, form, ctx, detail) {
    protocol.Front.prototype.initialize.call(this, conn, form, ctx, detail);
    this.inspector = this.parent();
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this._form = form;
  },

  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },

  get walker() {
    return this.inspector.walker;
  },

  getMatchedSelectors: protocol.custom(function(node, property, options) {
    return this._getMatchedSelectors(node, property, options).then(ret => {
      return ret.matched;
    });
  }, {
    impl: "_getMatchedSelectors"
  }),

  getApplied: protocol.custom(Task.async(function*(node, options={}) {
    
    
    
    
    if (!this._form.traits || !this._form.traits.getAppliedCreatesStyleCache) {
      yield this.getLayout(node);
    }
    let ret = yield this._getApplied(node, options);
    return ret.entries;
  }), {
    impl: "_getApplied"
  }),

  addNewRule: protocol.custom(function(node) {
    return this._addNewRule(node).then(ret => {
      return ret.entries[0];
    });
  }, {
    impl: "_addNewRule"
  })
});









var StyleRuleActor = protocol.ActorClass({
  typeName: "domstylerule",
  initialize: function(pageStyle, item) {
    protocol.Actor.prototype.initialize.call(this, null);
    this.pageStyle = pageStyle;
    this.rawStyle = item.style;

    if (item instanceof (Ci.nsIDOMCSSRule)) {
      this.type = item.type;
      this.rawRule = item;
      if ((this.rawRule instanceof Ci.nsIDOMCSSStyleRule ||
           this.rawRule instanceof Ci.nsIDOMMozCSSKeyframeRule) &&
           this.rawRule.parentStyleSheet) {
        this.line = DOMUtils.getRuleLine(this.rawRule);
        this.column = DOMUtils.getRuleColumn(this.rawRule);
      }
    } else {
      
      this.type = ELEMENT_STYLE;
      this.rawNode = item;
      this.rawRule = {
        style: item.style,
        toString: function() "[element rule " + this.style + "]"
      }
    }
  },

  get conn() this.pageStyle.conn,

  
  
  get marshallPool() this.pageStyle,

  getDocument: function(sheet) {
    let document;

    if (sheet.ownerNode instanceof Ci.nsIDOMHTMLDocument) {
      document = sheet.ownerNode;
    } else {
      document = sheet.ownerNode.ownerDocument;
    }

    return document;
  },

  toString: function() "[StyleRuleActor for " + this.rawRule + "]",

  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let form = {
      actor: this.actorID,
      type: this.type,
      line: this.line || undefined,
      column: this.column
    };

    if (this.rawRule.parentRule) {
      form.parentRule = this.pageStyle._styleRef(this.rawRule.parentRule).actorID;

      
      
      
      
      if (this.rawRule.parentRule.type === Ci.nsIDOMCSSRule.MEDIA_RULE) {
        form.media = [];
        for (let i = 0, n = this.rawRule.parentRule.media.length; i < n; i++) {
          form.media.push(this.rawRule.parentRule.media.item(i));
        }
      }
    }
    if (this.rawRule.parentStyleSheet) {
      form.parentStyleSheet = this.pageStyle._sheetRef(this.rawRule.parentStyleSheet).actorID;
    }

    switch (this.type) {
      case Ci.nsIDOMCSSRule.STYLE_RULE:
        form.selectors = CssLogic.getSelectors(this.rawRule);
        form.cssText = this.rawStyle.cssText || "";
        break;
      case ELEMENT_STYLE:
        
        
        
        let doc = this.rawNode.ownerDocument;
        form.href = doc.location ? doc.location.href : "";
        form.cssText = this.rawStyle.cssText || "";
        break;
      case Ci.nsIDOMCSSRule.CHARSET_RULE:
        form.encoding = this.rawRule.encoding;
        break;
      case Ci.nsIDOMCSSRule.IMPORT_RULE:
        form.href = this.rawRule.href;
        break;
      case Ci.nsIDOMCSSRule.KEYFRAMES_RULE:
        form.cssText = this.rawRule.cssText;
        form.name = this.rawRule.name;
        break;
      case Ci.nsIDOMCSSRule.KEYFRAME_RULE:
        form.cssText = this.rawStyle.cssText || "";
        form.keyText = this.rawRule.keyText || "";
        break;
    }

    return form;
  },

  















  modifyProperties: method(function(modifications) {
    let validProps = new Map();

    
    

    let document;
    if (this.rawNode) {
      document = this.rawNode.ownerDocument;
    } else {
      let parentStyleSheet = this.rawRule.parentStyleSheet;
      while (parentStyleSheet.ownerRule &&
          parentStyleSheet.ownerRule instanceof Ci.nsIDOMCSSImportRule) {
        parentStyleSheet = parentStyleSheet.ownerRule.parentStyleSheet;
      }

      document = this.getDocument(parentStyleSheet);
    }

    let tempElement = document.createElementNS(XHTML_NS, "div");

    for (let mod of modifications) {
      if (mod.type === "set") {
        tempElement.style.setProperty(mod.name, mod.value, mod.priority || "");
        this.rawStyle.setProperty(mod.name,
          tempElement.style.getPropertyValue(mod.name), mod.priority || "");
      } else if (mod.type === "remove") {
        this.rawStyle.removeProperty(mod.name);
      }
    }

    return this;
  }, {
    request: { modifications: Arg(0, "array:json") },
    response: { rule: RetVal("domstylerule") }
  }),

  








  modifySelector: method(function(value) {
    if (this.type === ELEMENT_STYLE) {
      return false;
    }

    let rule = this.rawRule;
    let parentStyleSheet = rule.parentStyleSheet;
    let document = this.getDocument(parentStyleSheet);
    
    let [selector, pseudoProp] = value.split(/(:{1,2}.+$)/);
    let selectorElement;

    try {
      selectorElement = document.querySelector(selector);
    } catch (e) {
      return false;
    }

    
    
    if (selectorElement && rule.selectorText !== value) {
      let cssRules = parentStyleSheet.cssRules;
      let cssText = rule.cssText;
      let selectorText = rule.selectorText;

      
      let i = 0;
      for (let cssRule of cssRules) {
        if (rule === cssRule) {
          parentStyleSheet.deleteRule(i);
          break;
        }

        i++;
      }

      
      let ruleText = cssText.slice(selectorText.length).trim();
      parentStyleSheet.insertRule(value + " " + ruleText, i);

      return true;
    } else {
      return false;
    }
  }, {
    request: { selector: Arg(0, "string") },
    response: { isModified: RetVal("boolean") },
  }),
});




var StyleRuleFront = protocol.FrontClass(StyleRuleActor, {
  initialize: function(client, form, ctx, detail) {
    protocol.Front.prototype.initialize.call(this, client, form, ctx, detail);
  },

  destroy: function() {
    protocol.Front.prototype.destroy.call(this);
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this.actorID = form.actor;
    this._form = form;
    if (this._mediaText) {
      this._mediaText = null;
    }
  },

  


  startModifyingProperties: function() {
    return new RuleModificationList(this);
  },

  get type() this._form.type,
  get line() this._form.line || -1,
  get column() this._form.column || -1,
  get cssText() {
    return this._form.cssText;
  },
  get keyText() {
    return this._form.keyText;
  },
  get name() {
    return this._form.name;
  },
  get selectors() {
    return this._form.selectors;
  },
  get media() {
    return this._form.media;
  },
  get mediaText() {
    if (!this._form.media) {
      return null;
    }
    if (this._mediaText) {
      return this._mediaText;
    }
    this._mediaText = this.media.join(", ");
    return this._mediaText;
  },

  get parentRule() {
    return this.conn.getActor(this._form.parentRule);
  },

  get parentStyleSheet() {
    return this.conn.getActor(this._form.parentStyleSheet);
  },

  get element() {
    return this.conn.getActor(this._form.element);
  },

  get href() {
    if (this._form.href) {
      return this._form.href;
    }
    let sheet = this.parentStyleSheet;
    return sheet ? sheet.href : "";
  },

  get nodeHref() {
    let sheet = this.parentStyleSheet;
    return sheet ? sheet.nodeHref : "";
  },

  get location()
  {
    return {
      source: this.parentStyleSheet,
      href: this.href,
      line: this.line,
      column: this.column
    };
  },

  getOriginalLocation: function()
  {
    if (this._originalLocation) {
      return promise.resolve(this._originalLocation);
    }
    let parentSheet = this.parentStyleSheet;
    if (!parentSheet) {
      
      
      return promise.resolve(this.location);
    }
    return parentSheet.getOriginalLocation(this.line, this.column)
      .then(({ fromSourceMap, source, line, column }) => {
        let location = {
          href: source,
          line: line,
          column: column,
          mediaText: this.mediaText
        };
        if (fromSourceMap === false) {
          location.source = this.parentStyleSheet;
        }
        if (!source) {
          location.href = this.href;
        }
        this._originalLocation = location;
        return location;
      });
  }
});





var RuleModificationList = Class({
  initialize: function(rule) {
    this.rule = rule;
    this.modifications = [];
  },

  apply: function() {
    return this.rule.modifyProperties(this.modifications);
  },
  setProperty: function(name, value, priority) {
    this.modifications.push({
      type: "set",
      name: name,
      value: value,
      priority: priority
    });
  },
  removeProperty: function(name) {
    this.modifications.push({
      type: "remove",
      name: name
    });
  }
});














function getFontPreviewData(font, doc, options) {
  options = options || {};
  let previewText = options.previewText || FONT_PREVIEW_TEXT;
  let previewFontSize = options.previewFontSize || FONT_PREVIEW_FONT_SIZE;
  let fillStyle = options.fillStyle || FONT_PREVIEW_FILLSTYLE;
  let fontStyle = options.fontStyle || "";

  let canvas = doc.createElementNS(XHTML_NS, "canvas");
  let ctx = canvas.getContext("2d");
  let fontValue = fontStyle + " " + previewFontSize + "px " + font + ", serif";

  
  ctx.font = fontValue;
  ctx.fillStyle = fillStyle;
  let textWidth = ctx.measureText(previewText).width;
  let offset = 4; 
  canvas.width = textWidth * 2 + offset * 2;
  canvas.height = previewFontSize * 3;

  
  ctx.font = fontValue;
  ctx.fillStyle = fillStyle;

  
  ctx.textBaseline = "top";
  ctx.scale(2, 2);
  ctx.fillText(previewText, offset, Math.round(previewFontSize / 3));

  let dataURL = canvas.toDataURL("image/png");

  return {
    dataURL: dataURL,
    size: textWidth + offset * 2
  };
}

exports.getFontPreviewData = getFontPreviewData;
