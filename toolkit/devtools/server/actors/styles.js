



"use strict";

const {Cc, Ci, Cu} = require("chrome");
const Services = require("Services");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const protocol = require("devtools/server/protocol");
const {Arg, Option, method, RetVal, types} = protocol;
const events = require("sdk/event/core");
const object = require("sdk/util/object");
const { Class } = require("sdk/core/heritage");
const { StyleSheetActor } = require("devtools/server/actors/stylesheets");

loader.lazyGetter(this, "CssLogic", () => require("devtools/styleinspector/css-logic").CssLogic);
loader.lazyGetter(this, "DOMUtils", () => Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils));




const ELEMENT_STYLE = 100;
exports.ELEMENT_STYLE = ELEMENT_STYLE;

const PSEUDO_ELEMENTS = [":first-line", ":first-letter", ":before", ":after", ":-moz-selection"];
exports.PSEUDO_ELEMENTS = PSEUDO_ELEMENTS;


types.addActorType("domnode");





types.addLifetime("walker", "walker");






types.addDictType("appliedstyle", {
  rule: "domstylerule#actorid",
  inherited: "nullable:domnode#actorid"
});

types.addDictType("matchedselector", {
  rule: "domstylerule#actorid",
  selector: "string",
  value: "string",
  status: "number"
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
  },

  get conn() this.inspector.conn,

  



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
    if (this.refMap.has(sheet)) {
      return this.refMap.get(sheet);
    }
    let actor = new StyleSheetActor(sheet, this, this.walker.rootWin);
    this.manage(actor);
    this.refMap.set(sheet, actor);

    return actor;
  },

  























  getComputed: method(function(node, options) {
    let ret = Object.create(null);

    this.cssLogic.sourceFilter = options.filter || CssLogic.FILTER.UA;
    this.cssLogic.highlight(node.rawNode);
    let computed = this.cssLogic._computedStyle || [];

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
    let entries = [];

    this.addElementRules(node.rawNode, undefined, options, entries);

    if (options.inherited) {
      let parent = this.walker.parentNode(node);
      while (parent && parent.rawNode.nodeType != Ci.nsIDOMNode.DOCUMENT_NODE) {
        this.addElementRules(parent.rawNode, parent, options, entries);
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
        entry.matchedSelectors = [];
        for (let i = 0; i < selectors.length; i++) {
          if (DOMUtils.selectorMatchesElement(element, domRule, i)) {
            entry.matchedSelectors.push(selectors[i]);
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
  }, {
    request: {
      node: Arg(0, "domnode"),
      inherited: Option(1, "boolean"),
      matchedSelectors: Option(1, "boolean"),
      filter: Option(1, "string")
    },
    response: RetVal(types.addDictType("appliedStylesReturn", {
      entries: "array:appliedstyle",
      rules: "array:domstylerule",
      sheets: "array:stylesheet"
    }))
  }),

  _hasInheritedProps: function(style) {
    return Array.prototype.some.call(style, prop => {
      return DOMUtils.isInheritedProperty(prop);
    });
  },

  



  addElementRules: function(element, inherited, options, rules)
  {
    if (!element.style) {
      return;
    }

    let elementStyle = this._styleRef(element);

    if (!inherited || this._hasInheritedProps(element.style)) {
      rules.push({
        rule: elementStyle,
        inherited: inherited,
      });
    }

    let pseudoElements = inherited ? [null] : [null, ...PSEUDO_ELEMENTS];
    for (let pseudo of pseudoElements) {

      
      let domRules = DOMUtils.getCSSStyleRules(element, pseudo);

      if (!domRules) {
        continue;
      }

      
      
      for (let i = domRules.Count() - 1; i >= 0; i--) {
        let domRule = domRules.GetElementAt(i);

        let isSystem = !CssLogic.isContentStylesheet(domRule.parentStyleSheet);

        if (isSystem && options.filter != CssLogic.FILTER.UA) {
          continue;
        }

        if (inherited) {
          
          
          let hasInherited = Array.prototype.some.call(domRule.style, prop => {
            return DOMUtils.isInheritedProperty(prop);
          });
          if (!hasInherited) {
            continue;
          }
        }

        let ruleActor = this._styleRef(domRule);
        rules.push({
          rule: ruleActor,
          inherited: inherited,
          pseudoElement: pseudo,
          isSystem: isSystem
        });
      }

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
    layout.width = Math.round(clientRect.width);
    layout.height = Math.round(clientRect.height);

    
    let style = node.rawNode.ownerDocument.defaultView.getComputedStyle(node.rawNode);
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
      this.map[i].value = parseInt(style.getPropertyValue(property));
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

});
exports.PageStyleActor = PageStyleActor;




var PageStyleFront = protocol.FrontClass(PageStyleActor, {
  initialize: function(conn, form, ctx, detail) {
    protocol.Front.prototype.initialize.call(this, conn, form, ctx, detail);
    this.inspector = this.parent();
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

  getApplied: protocol.custom(function(node, options={}) {
    return this._getApplied(node, options).then(ret => {
      return ret.entries;
    });
  }, {
    impl: "_getApplied"
  })
});


types.addActorType("domstylerule");









var StyleRuleActor = protocol.ActorClass({
  typeName: "domstylerule",
  initialize: function(pageStyle, item) {
    protocol.Actor.prototype.initialize.call(this, null);
    this.pageStyle = pageStyle;
    this.rawStyle = item.style;

    if (item instanceof (Ci.nsIDOMCSSRule)) {
      this.type = item.type;
      this.rawRule = item;
      if (this.rawRule instanceof Ci.nsIDOMCSSStyleRule && this.rawRule.parentStyleSheet) {
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
        
        
        
        form.href = this.rawNode.ownerDocument.location.href;
        form.cssText = this.rawStyle.cssText || "";
        break;
      case Ci.nsIDOMCSSRule.CHARSET_RULE:
        form.encoding = this.rawRule.encoding;
        break;
      case Ci.nsIDOMCSSRule.IMPORT_RULE:
        form.href = this.rawRule.href;
        break;
      case Ci.nsIDOMCSSRule.MEDIA_RULE:
        form.media = [];
        for (let i = 0, n = this.rawRule.media.length; i < n; i++) {
          form.media.push(this.rawRule.media.item(i));
        }
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

    let tempElement = document.createElement("div");

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

      
      let i = 0;
      for (let cssRule of cssRules) {
        if (rule === cssRule) {
          parentStyleSheet.deleteRule(i);
          break;
        }

        i++;
      }

      
      let ruleText = rule.cssText.slice(rule.selectorText.length).trim();
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
    return sheet.href;
  },

  get nodeHref() {
    let sheet = this.parentStyleSheet;
    return sheet ? sheet.nodeHref : "";
  },

  get location()
  {
    return {
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
      .then(({ source, line, column }) => {
        let location = {
          href: source,
          line: line,
          column: column
        }
        if (!source) {
          location.href = this.href;
        }
        this._originalLocation = location;
        return location;
      })
  },

  
  _rawStyle: function() {
    if (!this.conn._transport._serverConnection) {
      console.warn("Tried to use rawNode on a remote connection.");
      return null;
    }
    let actor = this.conn._transport._serverConnection.getActor(this.actorID);
    if (!actor) {
      return null;
    }
    return actor.rawStyle;
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

