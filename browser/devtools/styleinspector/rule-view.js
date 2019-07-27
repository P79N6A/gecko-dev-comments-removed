





"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {CssLogic} = require("devtools/styleinspector/css-logic");
const {InplaceEditor, editableField, editableItem} = require("devtools/shared/inplace-editor");
const {ELEMENT_STYLE, PSEUDO_ELEMENTS} = require("devtools/server/actors/styles");
const {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});
const {OutputParser} = require("devtools/output-parser");
const {PrefObserver, PREF_ORIG_SOURCES} = require("devtools/styleeditor/utils");
const {parseSingleValue, parseDeclarations} = require("devtools/styleinspector/css-parsing-utils");
const overlays = require("devtools/styleinspector/style-inspector-overlays");
const EventEmitter = require("devtools/toolkit/event-emitter");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const PREF_UA_STYLES = "devtools.inspector.showUserAgentStyles";
const PREF_DEFAULT_COLOR_UNIT = "devtools.defaultColorUnit";
const FILTER_CHANGED_TIMEOUT = 150;







const CSS_LINE_RE = /(?:[^;\(]*(?:\([^\)]*?\))?[^;\(]*)*;?/g;


const CSS_PROP_RE = /\s*([^:\s]*)\s*:\s*(.*?)\s*(?:! (important))?;?$/;


const CSS_RESOURCE_RE = /url\([\'\"]?(.*?)[\'\"]?\)/;

const IOService = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService);

function promiseWarn(err) {
  console.error(err);
  return promise.reject(err);
}








var gDummyPromise;
function createDummyDocument() {
  if (gDummyPromise) {
    return gDummyPromise;
  }
  const { getDocShell, create: makeFrame } = require("sdk/frame/utils");

  let frame = makeFrame(Services.appShell.hiddenDOMWindow.document, {
    nodeName: "iframe",
    namespaceURI: "http://www.w3.org/1999/xhtml",
    allowJavascript: false,
    allowPlugins: false,
    allowAuth: false
  });
  let docShell = getDocShell(frame);
  let eventTarget = docShell.chromeEventHandler;
  docShell.createAboutBlankContentViewer(Cc["@mozilla.org/nullprincipal;1"].createInstance(Ci.nsIPrincipal));
  let window = docShell.contentViewer.DOMDocument.defaultView;
  window.location = "data:text/html,<html></html>";
  let deferred = promise.defer();
  eventTarget.addEventListener("DOMContentLoaded", function handler(event) {
    eventTarget.removeEventListener("DOMContentLoaded", handler, false);
    deferred.resolve(window.document);
    frame.remove();
  }, false);
  gDummyPromise = deferred.promise;
  return gDummyPromise;
}





































function ElementStyle(aElement, aStore, aPageStyle, aShowUserAgentStyles) {
  this.element = aElement;
  this.store = aStore || {};
  this.pageStyle = aPageStyle;
  this.showUserAgentStyles = aShowUserAgentStyles;
  this.rules = [];

  
  
  if (!("userProperties" in this.store)) {
    this.store.userProperties = new UserProperties();
  }

  if (!("disabled" in this.store)) {
    this.store.disabled = new WeakMap();
  }
}


exports._ElementStyle = ElementStyle;

ElementStyle.prototype = {
  
  element: null,

  
  
  dummyElement: null,

  init: function()
  {
    
    
    
    return this.dummyElementPromise = createDummyDocument().then(document => {
      
      let namespaceURI = this.element.namespaceURI || document.documentElement.namespaceURI;
      this.dummyElement = document.createElementNS(namespaceURI,
                                                   this.element.tagName);
      document.documentElement.appendChild(this.dummyElement);
      return this.dummyElement;
    }).then(null, promiseWarn);
  },

  destroy: function() {
    if (this.destroyed) {
      return;
    }
    this.destroyed = true;

    this.dummyElement = null;
    this.dummyElementPromise.then(dummyElement => {
      dummyElement.remove();
      this.dummyElementPromise = null;
    }, console.error);
  },

  



  _changed: function() {
    if (this.onChanged) {
      this.onChanged();
    }
  },

  






  populate: function() {
    let populated = this.pageStyle.getApplied(this.element, {
      inherited: true,
      matchedSelectors: true,
      filter: this.showUserAgentStyles ? "ua" : undefined,
    }).then(entries => {
      if (this.destroyed) {
        return;
      }

      
      return this.dummyElementPromise.then(() => {
        if (this.populated != populated) {
          
          return;
        }

        
        
        this._refreshRules = this.rules;

        this.rules = [];

        for (let entry of entries) {
          this._maybeAddRule(entry);
        }

        
        this.markOverriddenAll();

        this._sortRulesForPseudoElement();

        
        delete this._refreshRules;

        return null;
      });
    }).then(null, promiseWarn);
    this.populated = populated;
    return this.populated;
  },

  


   _sortRulesForPseudoElement: function() {
      this.rules = this.rules.sort((a, b) => {
        return (a.pseudoElement || "z") > (b.pseudoElement || "z");
      });
   },

  








  _maybeAddRule: function(aOptions) {
    
    
    if (aOptions.rule &&
        this.rules.some(function(rule) rule.domRule === aOptions.rule)) {
      return false;
    }

    if (aOptions.system) {
      return false;
    }

    let rule = null;

    
    
    if (this._refreshRules) {
      for (let r of this._refreshRules) {
        if (r.matches(aOptions)) {
          rule = r;
          rule.refresh(aOptions);
          break;
        }
      }
    }

    
    if (!rule) {
      rule = new Rule(this, aOptions);
    }

    
    if (aOptions.inherited && rule.textProps.length == 0) {
      return false;
    }

    this.rules.push(rule);
    return true;
  },

  


  markOverriddenAll: function() {
    this.markOverridden();
    for (let pseudo of PSEUDO_ELEMENTS) {
      this.markOverridden(pseudo);
    }
  },

  






  markOverridden: function(pseudo="") {
    
    
    
    
    
    let textProps = [];
    for (let rule of this.rules) {
      if (rule.pseudoElement == pseudo && !rule.keyframes) {
        textProps = textProps.concat(rule.textProps.slice(0).reverse());
      }
    }

    
    
    let computedProps = [];
    for (let textProp of textProps) {
      computedProps = computedProps.concat(textProp.computed);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let taken = {};
    for (let computedProp of computedProps) {
      let earlier = taken[computedProp.name];
      let overridden;
      if (earlier &&
          computedProp.priority === "important" &&
          earlier.priority !== "important") {
        
        
        earlier._overriddenDirty = !earlier._overriddenDirty;
        earlier.overridden = true;
        overridden = false;
      } else {
        overridden = !!earlier;
      }

      computedProp._overriddenDirty = (!!computedProp.overridden != overridden);
      computedProp.overridden = overridden;
      if (!computedProp.overridden && computedProp.textProp.enabled) {
        taken[computedProp.name] = computedProp;
      }
    }

    
    
    
    
    for (let textProp of textProps) {
      
      
      if (this._updatePropertyOverridden(textProp)) {
        textProp.updateEditor();
      }
    }
  },

  










  _updatePropertyOverridden: function(aProp) {
    let overridden = true;
    let dirty = false;
    for each (let computedProp in aProp.computed) {
      if (!computedProp.overridden) {
        overridden = false;
      }
      dirty = computedProp._overriddenDirty || dirty;
      delete computedProp._overriddenDirty;
    }

    dirty = (!!aProp.overridden != overridden) || dirty;
    aProp.overridden = overridden;
    return dirty;
  }
};














function Rule(aElementStyle, aOptions) {
  this.elementStyle = aElementStyle;
  this.domRule = aOptions.rule || null;
  this.style = aOptions.rule;
  this.matchedSelectors = aOptions.matchedSelectors || [];
  this.pseudoElement = aOptions.pseudoElement || "";

  this.isSystem = aOptions.isSystem;
  this.inherited = aOptions.inherited || null;
  this.keyframes = aOptions.keyframes || null;
  this._modificationDepth = 0;

  if (this.domRule && this.domRule.mediaText) {
    this.mediaText = this.domRule.mediaText;
  }

  
  
  this.textProps = this._getTextProperties();
  this.textProps = this.textProps.concat(this._getDisabledProperties());
}

Rule.prototype = {
  mediaText: "",

  get title() {
    if (this._title) {
      return this._title;
    }
    this._title = CssLogic.shortSource(this.sheet);
    if (this.domRule.type !== ELEMENT_STYLE && this.ruleLine > 0) {
      this._title += ":" + this.ruleLine;
    }

    this._title = this._title + (this.mediaText ? " @media " + this.mediaText : "");
    return this._title;
  },

  get inheritedSource() {
    if (this._inheritedSource) {
      return this._inheritedSource;
    }
    this._inheritedSource = "";
    if (this.inherited) {
      let eltText = this.inherited.tagName.toLowerCase();
      if (this.inherited.id) {
        eltText += "#" + this.inherited.id;
      }
      this._inheritedSource =
        CssLogic._strings.formatStringFromName("rule.inheritedFrom", [eltText], 1);
    }
    return this._inheritedSource;
  },

  get keyframesName() {
    if (this._keyframesName) {
      return this._keyframesName;
    }
    this._keyframesName = "";
    if (this.keyframes) {
      this._keyframesName =
        CssLogic._strings.formatStringFromName("rule.keyframe", [this.keyframes.name], 1);
    }
    return this._keyframesName;
  },

  get selectorText() {
    return this.domRule.selectors ? this.domRule.selectors.join(", ") : CssLogic.l10n("rule.sourceElement");
  },

  


  get sheet() {
    return this.domRule ? this.domRule.parentStyleSheet : null;
  },

  


  get ruleLine() {
    return this.domRule ? this.domRule.line : "";
  },

  


  get ruleColumn() {
    return this.domRule ? this.domRule.column : null;
  },

  







  getOriginalSourceStrings: function() {
    if (this._originalSourceStrings) {
      return promise.resolve(this._originalSourceStrings);
    }
    return this.domRule.getOriginalLocation().then(({href, line, mediaText}) => {
      let mediaString = mediaText ? " @" + mediaText : "";

      let sourceStrings = {
        full: (href || CssLogic.l10n("rule.sourceInline")) + ":" + line + mediaString,
        short: CssLogic.shortSource({href: href}) + ":" + line + mediaString
      };

      this._originalSourceStrings = sourceStrings;
      return sourceStrings;
    }, console.error);
  },

  






  matches: function(aOptions) {
    return this.style === aOptions.rule;
  },

  











  createProperty: function(aName, aValue, aPriority, aSiblingProp) {
    let prop = new TextProperty(this, aName, aValue, aPriority);

    if (aSiblingProp) {
      let ind = this.textProps.indexOf(aSiblingProp);
      this.textProps.splice(ind + 1, 0, prop);
    }
    else {
      this.textProps.push(prop);
    }

    this.applyProperties();
    return prop;
  },

  









  applyProperties: function(aModifications, aName) {
    this.elementStyle.markOverriddenAll();

    if (!aModifications) {
      aModifications = this.style.startModifyingProperties();
    }
    let disabledProps = [];
    let store = this.elementStyle.store;

    for (let prop of this.textProps) {
      if (!prop.enabled) {
        disabledProps.push({
          name: prop.name,
          value: prop.value,
          priority: prop.priority
        });
        continue;
      }
      if (prop.value.trim() === "") {
        continue;
      }

      aModifications.setProperty(prop.name, prop.value, prop.priority);

      prop.updateComputed();
    }

    
    let disabled = this.elementStyle.store.disabled;
    if (disabledProps.length > 0) {
      disabled.set(this.style, disabledProps);
    } else {
      disabled.delete(this.style);
    }

    let promise = aModifications.apply().then(() => {
      let cssProps = {};
      for (let cssProp of parseDeclarations(this.style.cssText)) {
        cssProps[cssProp.name] = cssProp;
      }

      for (let textProp of this.textProps) {
        if (!textProp.enabled) {
          continue;
        }
        let cssProp = cssProps[textProp.name];

        if (!cssProp) {
          cssProp = {
            name: textProp.name,
            value: "",
            priority: ""
          };
        }

        textProp.priority = cssProp.priority;
      }

      this.elementStyle.markOverriddenAll();

      if (promise === this._applyingModifications) {
        this._applyingModifications = null;
      }

      this.elementStyle._changed();
    }).then(null, promiseWarn);

    this._applyingModifications = promise;
    return promise;
  },

  







  setPropertyName: function(aProperty, aName) {
    if (aName === aProperty.name) {
      return;
    }
    let modifications = this.style.startModifyingProperties();
    modifications.removeProperty(aProperty.name);
    aProperty.name = aName;
    this.applyProperties(modifications, aName);
  },

  









  setPropertyValue: function(aProperty, aValue, aPriority) {
    if (aValue === aProperty.value && aPriority === aProperty.priority) {
      return;
    }

    aProperty.value = aValue;
    aProperty.priority = aPriority;
    this.applyProperties(null, aProperty.name);
  },

  










  previewPropertyValue: function(aProperty, aValue, aPriority) {
    aProperty.value = aValue;

    let modifications = this.style.startModifyingProperties();
    modifications.setProperty(aProperty.name, aValue, aPriority);
    modifications.apply();
  },

  






  setPropertyEnabled: function(aProperty, aValue) {
    aProperty.enabled = !!aValue;
    let modifications = this.style.startModifyingProperties();
    if (!aProperty.enabled) {
      modifications.removeProperty(aProperty.name);
    }
    this.applyProperties(modifications);
  },

  






  removeProperty: function(aProperty) {
    this.textProps = this.textProps.filter(function(prop) prop != aProperty);
    let modifications = this.style.startModifyingProperties();
    modifications.removeProperty(aProperty.name);
    
    
    this.applyProperties(modifications);
  },

  



  _getTextProperties: function() {
    let textProps = [];
    let store = this.elementStyle.store;
    let props = parseDeclarations(this.style.cssText);
    for (let prop of props) {
      let name = prop.name;
      if (this.inherited && !domUtils.isInheritedProperty(name)) {
        continue;
      }
      let value = store.userProperties.getProperty(this.style, name, prop.value);
      let textProp = new TextProperty(this, name, value, prop.priority);
      textProps.push(textProp);
    }

    return textProps;
  },

  


  _getDisabledProperties: function() {
    let store = this.elementStyle.store;

    
    let disabledProps = store.disabled.get(this.style);
    if (!disabledProps) {
      return [];
    }

    let textProps = [];

    for each (let prop in disabledProps) {
      let value = store.userProperties.getProperty(this.style, prop.name, prop.value);
      let textProp = new TextProperty(this, prop.name, value, prop.priority);
      textProp.enabled = false;
      textProps.push(textProp);
    }

    return textProps;
  },

  



  refresh: function(aOptions) {
    this.matchedSelectors = aOptions.matchedSelectors || [];
    let newTextProps = this._getTextProperties();

    
    
    
    
    
    
    let brandNewProps = [];
    for (let newProp of newTextProps) {
      if (!this._updateTextProperty(newProp)) {
        brandNewProps.push(newProp);
      }
    }

    
    
    for (let prop of this.textProps) {
      
      
      if (!prop._visited) {
        prop.enabled = false;
        prop.updateEditor();
      } else {
        delete prop._visited;
      }
    }

    
    this.textProps = this.textProps.concat(brandNewProps);

    
    if (this.editor) {
      this.editor.populate();
    }
  },

  






















  _updateTextProperty: function(aNewProp) {
    let match = { rank: 0, prop: null };

    for each (let prop in this.textProps) {
      if (prop.name != aNewProp.name)
        continue;

      
      prop._visited = true;

      
      let rank = 1;

      
      
      
      if (prop.value === aNewProp.value) {
        rank += 2;
        if (prop.priority === aNewProp.priority) {
          rank += 2;
        }
      }

      if (prop.enabled) {
        rank += 1;
      }

      if (rank > match.rank) {
        if (match.prop) {
          
          match.prop.enabled = false;
          match.prop.updateEditor();
        }
        match.rank = rank;
        match.prop = prop;
      } else if (rank) {
        
        prop.enabled = false;
        prop.updateEditor();
      }
    }

    
    
    if (match.prop) {
      match.prop.set(aNewProp);
      return true;
    }

    return false;
  },

  









  editClosestTextProperty: function(aTextProperty) {
    let index = this.textProps.indexOf(aTextProperty);
    let previous = false;

    
    if (index === this.textProps.length - 1) {
      index = index - 1;
      previous = true;
    }
    else {
      index = index + 1;
    }

    let nextProp = this.textProps[index];

    
    
    if (nextProp) {
      if (previous) {
        nextProp.editor.valueSpan.click();
      } else {
        nextProp.editor.nameSpan.click();
      }
    } else {
      aTextProperty.rule.editor.closeBrace.focus();
    }
  }
};














function TextProperty(aRule, aName, aValue, aPriority) {
  this.rule = aRule;
  this.name = aName;
  this.value = aValue;
  this.priority = aPriority;
  this.enabled = true;
  this.updateComputed();
}

TextProperty.prototype = {
  



  updateEditor: function() {
    if (this.editor) {
      this.editor.update();
    }
  },

  


  updateComputed: function() {
    if (!this.name) {
      return;
    }

    
    
    
    let dummyElement = this.rule.elementStyle.dummyElement;
    let dummyStyle = dummyElement.style;
    dummyStyle.cssText = "";
    dummyStyle.setProperty(this.name, this.value, this.priority);

    this.computed = [];

    try {
      
      
      
      let subProps = domUtils.getSubpropertiesForCSSProperty(this.name);

      for (let prop of subProps) {
        this.computed.push({
          textProp: this,
          name: prop,
          value: dummyStyle.getPropertyValue(prop),
          priority: dummyStyle.getPropertyPriority(prop),
        });
      }
    } catch(e) {
      
      
     }
  },

  






  set: function(aOther) {
    let changed = false;
    for (let item of ["name", "value", "priority", "enabled"]) {
      if (this[item] != aOther[item]) {
        this[item] = aOther[item];
        changed = true;
      }
    }

    if (changed) {
      this.updateEditor();
    }
  },

  setValue: function(aValue, aPriority, force=false) {
    let store = this.rule.elementStyle.store;

    if (this.editor && aValue !== this.editor.committed.value || force) {
      store.userProperties.setProperty(this.rule.style, this.name, aValue);
    }

    this.rule.setPropertyValue(this, aValue, aPriority);
    this.updateEditor();
  },

  setName: function(aName) {
    let store = this.rule.elementStyle.store;

    if (aName !== this.name) {
      store.userProperties.setProperty(this.rule.style, aName,
                                       this.editor.committed.value);
    }

    this.rule.setPropertyName(this, aName);
    this.updateEditor();
  },

  setEnabled: function(aValue) {
    this.rule.setPropertyEnabled(this, aValue);
    this.updateEditor();
  },

  remove: function() {
    this.rule.removeProperty(this);
  }
};


































function CssRuleView(aInspector, aDoc, aStore, aPageStyle) {
  this.inspector = aInspector;
  this.doc = aDoc;
  this.store = aStore || {};
  this.pageStyle = aPageStyle;

  this._highlightedElements = [];
  this._outputParser = new OutputParser();

  this._buildContextMenu = this._buildContextMenu.bind(this);
  this._onContextMenu = this._onContextMenu.bind(this);
  this._contextMenuUpdate = this._contextMenuUpdate.bind(this);
  this._onAddRule = this._onAddRule.bind(this);
  this._onSelectAll = this._onSelectAll.bind(this);
  this._onCopy = this._onCopy.bind(this);
  this._onCopyColor = this._onCopyColor.bind(this);
  this._onToggleOrigSources = this._onToggleOrigSources.bind(this);
  this._onFilterStyles = this._onFilterStyles.bind(this);
  this._onClearSearch = this._onClearSearch.bind(this);

  this.element = this.doc.getElementById("ruleview-container");
  this.searchField = this.doc.getElementById("ruleview-searchbox");
  this.searchClearButton = this.doc.getElementById("ruleview-searchinput-clear");

  this.searchClearButton.hidden = true;

  this.element.addEventListener("copy", this._onCopy);
  this.element.addEventListener("contextmenu", this._onContextMenu);
  this.searchField.addEventListener("input", this._onFilterStyles);
  this.searchClearButton.addEventListener("click", this._onClearSearch);

  this._handlePrefChange = this._handlePrefChange.bind(this);
  this._onSourcePrefChanged = this._onSourcePrefChanged.bind(this);

  this._prefObserver = new PrefObserver("devtools.");
  this._prefObserver.on(PREF_ORIG_SOURCES, this._onSourcePrefChanged);
  this._prefObserver.on(PREF_UA_STYLES, this._handlePrefChange);
  this._prefObserver.on(PREF_DEFAULT_COLOR_UNIT, this._handlePrefChange);

  this.showUserAgentStyles = Services.prefs.getBoolPref(PREF_UA_STYLES);

  let options = {
    autoSelect: true,
    theme: "auto"
  };
  this.popup = new AutocompletePopup(aDoc.defaultView.parent.document, options);

  this._buildContextMenu();
  this._showEmpty();

  
  this.tooltips = new overlays.TooltipsOverlay(this);
  this.tooltips.addToView();
  this.highlighters = new overlays.HighlightersOverlay(this);
  this.highlighters.addToView();

  EventEmitter.decorate(this);
}

exports.CssRuleView = CssRuleView;

CssRuleView.prototype = {
  
  _viewedElement: null,

  
  _filterChangedTimeout: null,

  


  _buildContextMenu: function() {
    let doc = this.doc.defaultView.parent.document;

    this._contextmenu = doc.createElementNS(XUL_NS, "menupopup");
    this._contextmenu.addEventListener("popupshowing", this._contextMenuUpdate);
    this._contextmenu.id = "rule-view-context-menu";

    this.menuitemAddRule = createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.addRule",
      accesskey: "ruleView.contextmenu.addRule.accessKey",
      command: this._onAddRule
    });
    this.menuitemSelectAll = createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.selectAll",
      accesskey: "ruleView.contextmenu.selectAll.accessKey",
      command: this._onSelectAll
    });
    this.menuitemCopy = createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.copy",
      accesskey: "ruleView.contextmenu.copy.accessKey",
      command: this._onCopy
    });
    this.menuitemCopyColor = createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.copyColor",
      accesskey: "ruleView.contextmenu.copyColor.accessKey",
      command: this._onCopyColor
    });
    this.menuitemSources = createMenuItem(this._contextmenu, {
      label: "ruleView.contextmenu.showOrigSources",
      accesskey: "ruleView.contextmenu.showOrigSources.accessKey",
      command: this._onToggleOrigSources,
      type: "checkbox"
    });

    let popupset = doc.documentElement.querySelector("popupset");
    if (!popupset) {
      popupset = doc.createElementNS(XUL_NS, "popupset");
      doc.documentElement.appendChild(popupset);
    }

    popupset.appendChild(this._contextmenu);
  },

  





  getSelectorHighlighter: Task.async(function*() {
    let utils = this.inspector.toolbox.highlighterUtils;
    if (!utils.supportsCustomHighlighters()) {
      return null;
    }

    if (this.selectorHighlighter) {
      return this.selectorHighlighter;
    }

    try {
      let h = yield utils.getHighlighterByType("SelectorHighlighter");
      return this.selectorHighlighter = h;
    } catch (e) {
      
      
      return null;
    }
  }),

  












  toggleSelectorHighlighter: function(selectorIcon, selector) {
    if (this.lastSelectorIcon) {
      this.lastSelectorIcon.classList.remove("highlighted");
    }
    selectorIcon.classList.remove("highlighted");

    this.unhighlightSelector().then(() => {
      if (selector !== this.highlightedSelector) {
        this.highlightedSelector = selector;
        selectorIcon.classList.add("highlighted");
        this.lastSelectorIcon = selectorIcon;
        this.highlightSelector(selector).then(() => {
          this.emit("ruleview-selectorhighlighter-toggled", true);
        }, Cu.reportError);
      } else {
        this.highlightedSelector = null;
        this.emit("ruleview-selectorhighlighter-toggled", false);
      }
    }, Cu.reportError);
  },

  highlightSelector: Task.async(function*(selector) {
    let node = this.inspector.selection.nodeFront;

    let highlighter = yield this.getSelectorHighlighter();
    if (!highlighter) {
      return;
    }

    yield highlighter.show(node, {
      hideInfoBar: true,
      hideGuides: true,
      selector
    });
  }),

  unhighlightSelector: Task.async(function*() {
    let highlighter = yield this.getSelectorHighlighter();
    if (!highlighter) {
      return;
    }

    yield highlighter.hide();
  }),

  



  _contextMenuUpdate: function() {
    let win = this.doc.defaultView;

    
    let selection = win.getSelection();
    let copy;

    if (selection.toString()) {
      
      copy = true;
    } else if (selection.anchorNode) {
      
      let { selectionStart, selectionEnd } = this.doc.popupNode;

      if (isFinite(selectionStart) && isFinite(selectionEnd) &&
          selectionStart !== selectionEnd) {
        copy = true;
      }
    } else {
      
      copy = false;
    }

    this.menuitemCopyColor.hidden = !this._isColorPopup();
    this.menuitemCopy.disabled = !copy;

    var showOrig = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    this.menuitemSources.setAttribute("checked", showOrig);

    this.menuitemAddRule.disabled = this.inspector.selection.isAnonymousNode();
  },

  








  getNodeInfo: function(node) {
    if (!node) {
      return null;
    }

    let type, value;
    let classes = node.classList;
    let prop = getParentTextProperty(node);

    if (classes.contains("ruleview-propertyname") && prop) {
      type = overlays.VIEW_NODE_PROPERTY_TYPE;
      value = {
        property: node.textContent,
        value: getPropertyNameAndValue(node).value,
        enabled: prop.enabled,
        overridden: prop.overridden,
        pseudoElement: prop.rule.pseudoElement,
        sheetHref: prop.rule.domRule.href
      };
    } else if (classes.contains("ruleview-propertyvalue") && prop) {
      type = overlays.VIEW_NODE_VALUE_TYPE;
      value = {
        property: getPropertyNameAndValue(node).name,
        value: node.textContent,
        enabled: prop.enabled,
        overridden: prop.overridden,
        pseudoElement: prop.rule.pseudoElement,
        sheetHref: prop.rule.domRule.href
      };
    } else if (classes.contains("theme-link") && prop) {
      type = overlays.VIEW_NODE_IMAGE_URL_TYPE;
      value = {
        property: getPropertyNameAndValue(node).name,
        value: node.parentNode.textContent,
        url: node.href,
        enabled: prop.enabled,
        overridden: prop.overridden,
        pseudoElement: prop.rule.pseudoElement,
        sheetHref: prop.rule.domRule.href
      };
    } else if (classes.contains("ruleview-selector-unmatched") ||
               classes.contains("ruleview-selector-matched")) {
      type = overlays.VIEW_NODE_SELECTOR_TYPE;
      value = node.textContent;
    } else {
      return null;
    }

    return {type, value};
  },

  






  _isColorPopup: function () {
    this._colorToCopy = "";

    let trigger = this.doc.popupNode;
    if (!trigger) {
      return false;
    }

    let container = (trigger.nodeType == trigger.TEXT_NODE) ?
                     trigger.parentElement : trigger;

    let isColorNode = el => el.dataset && "color" in el.dataset;

    while (!isColorNode(container)) {
      container = container.parentNode;
      if (!container) {
        return false;
      }
    }

    this._colorToCopy = container.dataset["color"];
    return true;
  },

  


  _onContextMenu: function(event) {
    try {
      
      
      this.doc.popupNode = event.explicitOriginalTarget;
      this.doc.defaultView.focus();
      this._contextmenu.openPopupAtScreen(event.screenX, event.screenY, true);
    } catch(e) {
      console.error(e);
    }
  },

  


  _onSelectAll: function() {
    let win = this.doc.defaultView;
    let selection = win.getSelection();

    selection.selectAllChildren(this.doc.documentElement);
  },

  





  _onCopy: function(event) {
    try {
      let target = event.target;
      let text;

      if (event.target.nodeName === "menuitem") {
        target = this.doc.popupNode;
      }

      if (target.nodeName == "input") {
        let start = Math.min(target.selectionStart, target.selectionEnd);
        let end = Math.max(target.selectionStart, target.selectionEnd);
        let count = end - start;
        text = target.value.substr(start, count);
      } else {
        let win = this.doc.defaultView;
        let selection = win.getSelection();

        text = selection.toString();

        
        text = text.replace(/(\r?\n)\r?\n/g, "$1");

        
        let inline = _strings.GetStringFromName("rule.sourceInline");
        let rx = new RegExp("^" + inline + "\\r?\\n?", "g");
        text = text.replace(rx, "");
      }

      clipboardHelper.copyString(text, this.doc);
      event.preventDefault();
    } catch(e) {
      console.error(e);
    }
  },

  


  _onCopyColor: function() {
    clipboardHelper.copyString(this._colorToCopy, this.styleDocument);
  },

  


  _onToggleOrigSources: function() {
    let isEnabled = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    Services.prefs.setBoolPref(PREF_ORIG_SOURCES, !isEnabled);
  },

  


  _onAddRule: function() {
    let elementStyle = this._elementStyle;
    let element = elementStyle.element;
    let rules = elementStyle.rules;
    let client = this.inspector.toolbox._target.client;

    if (!client.traits.addNewRule) {
      return;
    }

    this.pageStyle.addNewRule(element).then(options => {
      let newRule = new Rule(elementStyle, options);
      rules.push(newRule);
      let editor = new RuleEditor(this, newRule);

      
      if (rules.length <= 1) {
        this.element.appendChild(editor.element);
      } else {
        for (let rule of rules) {
          if (rule.domRule.type === ELEMENT_STYLE) {
            let referenceElement = rule.editor.element.nextSibling;
            this.element.insertBefore(editor.element, referenceElement);
            break;
          }
        }
      }

      
      editor.selectorText.click();
      elementStyle._changed();
    });
  },

  setPageStyle: function(aPageStyle) {
    this.pageStyle = aPageStyle;
  },

  


  get isEditing() {
    return this.element.querySelectorAll(".styleinspector-propertyeditor").length > 0
      || this.tooltips.isEditing;
  },

  _handlePrefChange: function(pref) {
    if (pref === PREF_UA_STYLES) {
      this.showUserAgentStyles = Services.prefs.getBoolPref(pref);
    }

    
    let refreshOnPrefs = [PREF_UA_STYLES, PREF_DEFAULT_COLOR_UNIT];
    if (refreshOnPrefs.indexOf(pref) > -1) {
      let element = this._viewedElement;
      this._viewedElement = null;
      this.selectElement(element);
    }
  },

  _onSourcePrefChanged: function() {
    if (this.menuitemSources) {
      let isEnabled = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
      this.menuitemSources.setAttribute("checked", isEnabled);
    }

    
    if (this._elementStyle && this._elementStyle.rules) {
      for (let rule of this._elementStyle.rules) {
        if (rule.editor) {
          rule.editor.updateSourceLink();
        }
      }
      this.inspector.emit("rule-view-sourcelinks-updated");
    }
  },

  


  _onFilterStyles: function() {
    if (this._filterChangedTimeout) {
      clearTimeout(this._filterChangedTimeout);
    }

    let filterTimeout = (this.searchField.value.length > 0)
      ? FILTER_CHANGED_TIMEOUT : 0;
    this.searchClearButton.hidden = this.searchField.value.length === 0;

    this._filterChangedTimeout = setTimeout(() => {
      if (this.searchField.value.length > 0) {
        this.searchField.setAttribute("filled", true);
      } else {
        this.searchField.removeAttribute("filled");
      }

      this._clearRules();
      this._createEditors();

      this.inspector.emit("ruleview-filtered");

      this._filterChangeTimeout = null;
    }, filterTimeout);
  },

  



  _onClearSearch: function() {
    this.searchField.value = "";
    this.searchField.focus();
    this._onFilterStyles();
  },

  destroy: function() {
    this.isDestroyed = true;
    this.clear();

    gDummyPromise = null;

    this._prefObserver.off(PREF_ORIG_SOURCES, this._onSourcePrefChanged);
    this._prefObserver.off(PREF_UA_STYLES, this._handlePrefChange);
    this._prefObserver.off(PREF_DEFAULT_COLOR_UNIT, this._handlePrefChange);
    this._prefObserver.destroy();

    this._outputParser = null;
    this._highlightedElements = null;

    
    if (this._contextmenu) {
      
      this.menuitemAddRule.removeEventListener("command", this._onAddRule);
      this.menuitemAddRule = null;

      
      this.menuitemSelectAll.removeEventListener("command", this._onSelectAll);
      this.menuitemSelectAll = null;

      
      this.menuitemCopy.removeEventListener("command", this._onCopy);
      this.menuitemCopy = null;

      
      this.menuitemCopyColor.removeEventListener("command", this._onCopyColor);
      this.menuitemCopyColor = null;

      this.menuitemSources.removeEventListener("command", this._onToggleOrigSources);
      this.menuitemSources = null;

      
      this._contextmenu.removeEventListener("popupshowing", this._contextMenuUpdate);
      this._contextmenu.parentNode.removeChild(this._contextmenu);
      this._contextmenu = null;
    }

    
    this.doc.popupNode = null;

    this.tooltips.destroy();
    this.highlighters.destroy();

    
    this.element.removeEventListener("copy", this._onCopy);
    this.element.removeEventListener("contextmenu", this._onContextMenu);
    this.searchField.removeEventListener("input", this._onFilterStyles);
    this.searchClearButton.removeEventListener("click", this._onClearSearch);
    this.searchField = null;
    this.searchClearButton = null;

    if (this.element.parentNode) {
      this.element.parentNode.removeChild(this.element);
    }

    if (this._elementStyle) {
      this._elementStyle.destroy();
    }

    this.popup.destroy();
  },

  





  selectElement: function(aElement) {
    if (this._viewedElement === aElement) {
      return promise.resolve(undefined);
    }

    this.clear();

    this._viewedElement = aElement;
    if (!this._viewedElement) {
      this._showEmpty();
      return promise.resolve(undefined);
    }

    this._elementStyle = new ElementStyle(aElement, this.store,
      this.pageStyle, this.showUserAgentStyles);

    return this._elementStyle.init().then(() => {
      if (this._viewedElement === aElement) {
        return this._populate();
      }
    }).then(() => {
      if (this._viewedElement === aElement) {
        this._elementStyle.onChanged = () => {
          this._changed();
        };
      }
    }).then(null, console.error);
  },

  


  refreshPanel: function() {
    
    if (this.isEditing || !this._elementStyle) {
      return;
    }

    
    let promises = [];
    for (let rule of this._elementStyle.rules) {
      if (rule._applyingModifications) {
        promises.push(rule._applyingModifications);
      }
    }

    return promise.all(promises).then(() => {
      return this._populate(true);
    });
  },

  _populate: function(clearRules = false) {
    let elementStyle = this._elementStyle;
    return this._elementStyle.populate().then(() => {
      if (this._elementStyle != elementStyle || this.isDestroyed) {
        return;
      }

      if (clearRules) {
        this._clearRules();
      }
      this._createEditors();

      
      this.emit("ruleview-refreshed");
      return undefined;
    }).then(null, promiseWarn);
  },

  


  _showEmpty: function() {
    if (this.doc.getElementById("noResults") > 0) {
      return;
    }

    createChild(this.element, "div", {
      id: "noResults",
      textContent: CssLogic.l10n("rule.empty")
    });
  },

  


  _clearRules: function() {
    while (this.element.hasChildNodes()) {
      this.element.removeChild(this.element.lastChild);
    }
  },

  


  clear: function() {
    this.lastSelectorIcon = null;

    this._clearRules();
    this._viewedElement = null;

    if (this._elementStyle) {
      this._elementStyle.destroy();
      this._elementStyle = null;
    }
  },

  



  _changed: function() {
    this.emit("ruleview-changed");
  },

  


  get selectedElementLabel() {
    if (this._selectedElementLabel) {
      return this._selectedElementLabel;
    }
    this._selectedElementLabel = CssLogic.l10n("rule.selectedElement");
    return this._selectedElementLabel;
  },

  


  get pseudoElementLabel() {
    if (this._pseudoElementLabel) {
      return this._pseudoElementLabel;
    }
    this._pseudoElementLabel = CssLogic.l10n("rule.pseudoElement");
    return this._pseudoElementLabel;
  },

  get showPseudoElements() {
    if (this._showPseudoElements === undefined) {
      this._showPseudoElements =
        Services.prefs.getBoolPref("devtools.inspector.show_pseudo_elements");
    }
    return this._showPseudoElements;
  },

  






  createExpandableContainer: function(aLabel, isPseudo = false) {
    let header = this.doc.createElementNS(HTML_NS, "div");
    header.className = this._getRuleViewHeaderClassName(true);
    header.classList.add("show-expandable-container");
    header.textContent = aLabel;

    let twisty = this.doc.createElementNS(HTML_NS, "span");
    twisty.className = "ruleview-expander theme-twisty";
    twisty.setAttribute("open", "true");

    header.insertBefore(twisty, header.firstChild);
    this.element.appendChild(header);

    let container = this.doc.createElementNS(HTML_NS, "div");
    container.classList.add("ruleview-expandable-container");
    this.element.appendChild(container);

    let toggleContainerVisibility = (isPseudo, showPseudo) => {
      let isOpen = twisty.getAttribute("open");

      if (isPseudo) {
        this._showPseudoElements = !!showPseudo;

        Services.prefs.setBoolPref("devtools.inspector.show_pseudo_elements",
          this.showPseudoElements);

        header.classList.toggle("show-expandable-container",
          this.showPseudoElements);

        isOpen = !this.showPseudoElements;
      } else {
        header.classList.toggle("show-expandable-container");
      }

      if (isOpen) {
        twisty.removeAttribute("open");
      } else {
        twisty.setAttribute("open", "true");
      }
    };

    header.addEventListener("dblclick", () => {
      toggleContainerVisibility(isPseudo, !this.showPseudoElements);
    }, false);
    twisty.addEventListener("click", () => {
      toggleContainerVisibility(isPseudo, !this.showPseudoElements);
    }, false);

    if (isPseudo) {
      toggleContainerVisibility(isPseudo, this.showPseudoElements);
    }

    return container;
  },

  _getRuleViewHeaderClassName: function(isPseudo) {
    let baseClassName = "theme-gutter ruleview-header";
    return isPseudo ? baseClassName + " ruleview-expandable-header" : baseClassName;
  },

  


  _createEditors: function() {
    
    
    let lastInheritedSource = "";
    let lastKeyframes = null;
    let seenPseudoElement = false;
    let seenNormalElement = false;
    let seenSearchTerm = false;
    let container = null;
    let searchTerm = this.searchField.value.toLowerCase();
    let isValidSearchTerm = searchTerm.trim().length > 0;

    if (!this._elementStyle.rules) {
      return;
    }

    if (this._highlightedElements.length > 0) {
      this.clearHighlight();
    }

    for (let rule of this._elementStyle.rules) {
      if (rule.domRule.system) {
        continue;
      }

      
      if (!rule.editor) {
        rule.editor = new RuleEditor(this, rule);
      }

      
      if (isValidSearchTerm) {
        if (this.highlightRules(rule, searchTerm)) {
          seenSearchTerm = true;
        } else if (rule.domRule.type !== ELEMENT_STYLE) {
          continue;
        }
      }

      
      if (seenPseudoElement && !seenNormalElement && !rule.pseudoElement) {
        seenNormalElement = true;
        let div = this.doc.createElementNS(HTML_NS, "div");
        div.className = this._getRuleViewHeaderClassName();
        div.textContent = this.selectedElementLabel;
        this.element.appendChild(div);
      }

      let inheritedSource = rule.inheritedSource;
      if (inheritedSource && inheritedSource != lastInheritedSource) {
        let div = this.doc.createElementNS(HTML_NS, "div");
        div.className = this._getRuleViewHeaderClassName();
        div.textContent = inheritedSource;
        lastInheritedSource = inheritedSource;
        this.element.appendChild(div);
      }

      if (!seenPseudoElement && rule.pseudoElement) {
        seenPseudoElement = true;
        container = this.createExpandableContainer(this.pseudoElementLabel, true);
      }

      let keyframes = rule.keyframes;
      if (keyframes && keyframes != lastKeyframes) {
        lastKeyframes = keyframes;
        container = this.createExpandableContainer(rule.keyframesName);
      }

      if (container && (rule.pseudoElement || keyframes)) {
        container.appendChild(rule.editor.element);
      } else {
        this.element.appendChild(rule.editor.element);
      }
    }

    if (searchTerm && !seenSearchTerm) {
      this.searchField.classList.add("devtools-style-searchbox-no-match");
    } else {
      this.searchField.classList.remove("devtools-style-searchbox-no-match");
    }
  },

  









  highlightRules: function(aRule, aValue) {
    let isHighlighted = false;

    let selectorNodes = [...aRule.editor.selectorText.childNodes];
    if (aRule.domRule.type === Ci.nsIDOMCSSRule.KEYFRAME_RULE) {
      selectorNodes = [aRule.editor.selectorText];
    } else if (aRule.domRule.type === ELEMENT_STYLE) {
      selectorNodes = [];
    }

    aValue = aValue.trim();

    
    for (let selectorNode of selectorNodes) {
      if (selectorNode.textContent.toLowerCase().contains(aValue)) {
        selectorNode.classList.add("ruleview-highlight");
        this._highlightedElements.push(selectorNode);
        isHighlighted = true;
      }
    }

    
    
    
    let propertyMatch = CSS_PROP_RE.exec(aValue);
    let name = propertyMatch ? propertyMatch[1] : aValue;
    let value = propertyMatch ? propertyMatch[2] : aValue;

    
    for (let textProp of aRule.textProps) {
      
      let propertyValue = textProp.editor.valueSpan.textContent.toLowerCase();
      let propertyName = textProp.name.toLowerCase();

      
      
      
      let matches = false;
      if (propertyMatch && name && value) {
        matches = propertyName.contains(name) && propertyValue.contains(value);
      } else {
        matches = (name && propertyName.contains(name)) ||
                  (value && propertyValue.contains(value));
      }

      if (matches) {
      
        textProp.editor.element.classList.add("ruleview-highlight");
        this._highlightedElements.push(textProp.editor.element);
        isHighlighted = true;
      }
    }

    return isHighlighted;
  },

  


  clearHighlight: function() {
    for (let element of this._highlightedElements) {
      element.classList.remove("ruleview-highlight");
    }

    this._highlightedElements = [];
  }
};










function RuleEditor(aRuleView, aRule) {
  this.ruleView = aRuleView;
  this.doc = this.ruleView.doc;
  this.rule = aRule;

  this.isEditable = !aRule.isSystem;
  
  
  this.isEditing = false;

  this._onNewProperty = this._onNewProperty.bind(this);
  this._newPropertyDestroy = this._newPropertyDestroy.bind(this);
  this._onSelectorDone = this._onSelectorDone.bind(this);

  this._create();
}

RuleEditor.prototype = {
  get isSelectorEditable() {
    let toolbox = this.ruleView.inspector.toolbox;
    let trait = this.isEditable &&
      toolbox.target.client.traits.selectorEditable &&
      this.rule.domRule.type !== ELEMENT_STYLE &&
      this.rule.domRule.type !== Ci.nsIDOMCSSRule.KEYFRAME_RULE;

    
    
    return trait && !this.rule.elementStyle.element.isAnonymous;
  },

  _create: function() {
    this.element = this.doc.createElementNS(HTML_NS, "div");
    this.element.className = "ruleview-rule theme-separator";
    this.element.setAttribute("uneditable", !this.isEditable);
    this.element._ruleEditor = this;

    
    
    this.element.style.position = "relative";

    
    let source = createChild(this.element, "div", {
      class: "ruleview-rule-source theme-link"
    });
    source.addEventListener("click", function() {
      if (source.hasAttribute("unselectable")) {
        return;
      }
      let rule = this.rule.domRule;
      this.ruleView.emit("ruleview-linked-clicked", rule);
    }.bind(this));
    let sourceLabel = this.doc.createElementNS(XUL_NS, "label");
    sourceLabel.setAttribute("crop", "center");
    sourceLabel.classList.add("source-link-label");
    source.appendChild(sourceLabel);

    this.updateSourceLink();

    let code = createChild(this.element, "div", {
      class: "ruleview-code"
    });

    let header = createChild(code, "div", {});

    this.selectorContainer = createChild(header, "span", {
      class: "ruleview-selectorcontainer"
    });

    if (this.rule.domRule.type !== Ci.nsIDOMCSSRule.KEYFRAME_RULE &&
        this.rule.domRule.selectors) {
      let selector = this.rule.domRule.selectors.join(", ");

      let selectorHighlighter = createChild(header, "span", {
        class: "ruleview-selectorhighlighter" +
               (this.ruleView.highlightedSelector === selector ? " highlighted": ""),
        title: CssLogic.l10n("rule.selectorHighlighter.tooltip")
      });
      selectorHighlighter.addEventListener("click", () => {
        this.ruleView.toggleSelectorHighlighter(selectorHighlighter, selector);
      });
    }

    this.selectorText = createChild(this.selectorContainer, "span", {
      class: "ruleview-selector theme-fg-color3"
    });

    if (this.isSelectorEditable) {
      this.selectorContainer.addEventListener("click", aEvent => {
        
        aEvent.stopPropagation();
      }, false);

      editableField({
        element: this.selectorText,
        done: this._onSelectorDone,
        stopOnShiftTab: true,
        stopOnTab: true,
        stopOnReturn: true
      });
    }

    this.openBrace = createChild(header, "span", {
      class: "ruleview-ruleopen",
      textContent: " {"
    });

    this.propertyList = createChild(code, "ul", {
      class: "ruleview-propertylist"
    });

    this.populate();

    this.closeBrace = createChild(code, "div", {
      class: "ruleview-ruleclose",
      tabindex: this.isEditable ? "0" : "-1",
      textContent: "}"
    });

    if (this.isEditable) {
      code.addEventListener("click", () => {
        let selection = this.doc.defaultView.getSelection();
        if (selection.isCollapsed) {
          this.newProperty();
        }
      }, false);

      this.element.addEventListener("mousedown", () => {
        this.doc.defaultView.focus();
      }, false);

      
      editableItem({ element: this.closeBrace }, (aElement) => {
        this.newProperty();
      });
    }
  },

  updateSourceLink: function RuleEditor_updateSourceLink()
  {
    let sourceLabel = this.element.querySelector(".source-link-label");
    let sourceHref = (this.rule.sheet && this.rule.sheet.href) ?
      this.rule.sheet.href : this.rule.title;
    let sourceLine = this.rule.ruleLine > 0 ? ":" + this.rule.ruleLine : "";

    sourceLabel.setAttribute("tooltiptext", sourceHref + sourceLine);

    if (this.rule.isSystem) {
      let uaLabel = _strings.GetStringFromName("rule.userAgentStyles");
      sourceLabel.setAttribute("value", uaLabel + " " + this.rule.title);

      
      
      
      if (sourceHref === "about:PreferenceStyleSheet") {
        sourceLabel.parentNode.setAttribute("unselectable", "true");
        sourceLabel.setAttribute("value", uaLabel);
        sourceLabel.removeAttribute("tooltiptext");
      }
    } else {
      sourceLabel.setAttribute("value", this.rule.title);
      if (this.rule.ruleLine == -1 && this.rule.domRule.parentStyleSheet) {
        sourceLabel.parentNode.setAttribute("unselectable", "true");
      }
    }

    let showOrig = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    if (showOrig && !this.rule.isSystem && this.rule.domRule.type != ELEMENT_STYLE) {
      this.rule.getOriginalSourceStrings().then((strings) => {
        sourceLabel.setAttribute("value", strings.short);
        sourceLabel.setAttribute("tooltiptext", strings.full);
      }, console.error);
    }
  },

  


  populate: function() {
    
    while (this.selectorText.hasChildNodes()) {
      this.selectorText.removeChild(this.selectorText.lastChild);
    }

    
    
    
    if (this.rule.domRule.type === ELEMENT_STYLE) {
      this.selectorText.textContent = this.rule.selectorText;
    } else if (this.rule.domRule.type === Ci.nsIDOMCSSRule.KEYFRAME_RULE) {
      this.selectorText.textContent = this.rule.domRule.keyText;
    } else {
      this.rule.domRule.selectors.forEach((selector, i) => {
        if (i != 0) {
          createChild(this.selectorText, "span", {
            class: "ruleview-selector-separator",
            textContent: ", "
          });
        }
        let cls;
        if (this.rule.matchedSelectors.indexOf(selector) > -1) {
          cls = "ruleview-selector-matched";
        } else {
          cls = "ruleview-selector-unmatched";
        }
        createChild(this.selectorText, "span", {
          class: cls,
          textContent: selector
        });
      });
    }

    for (let prop of this.rule.textProps) {
      if (!prop.editor) {
        let editor = new TextPropertyEditor(this, prop);
        this.propertyList.appendChild(editor.element);
      }
    }
  },

  













  addProperty: function(aName, aValue, aPriority, aSiblingProp) {
    let prop = this.rule.createProperty(aName, aValue, aPriority, aSiblingProp);
    let index = this.rule.textProps.indexOf(prop);
    let editor = new TextPropertyEditor(this, prop);

    
    
    
    
    
    this.propertyList.insertBefore(editor.element,
      this.propertyList.children[index]);

    return prop;
  },

  














  addProperties: function(aProperties, aSiblingProp) {
    if (!aProperties || !aProperties.length) {
      return;
    }

    let lastProp = aSiblingProp;
    for (let p of aProperties) {
      lastProp = this.addProperty(p.name, p.value, p.priority, lastProp);
    }

    
    if (lastProp && lastProp.value.trim() === "") {
      lastProp.editor.valueSpan.click();
    } else {
      this.newProperty();
    }
  },

  




  newProperty: function() {
    
    if (!this.closeBrace.hasAttribute("tabindex")) {
      return;
    }

    
    
    
    this.closeBrace.removeAttribute("tabindex");

    this.newPropItem = createChild(this.propertyList, "li", {
      class: "ruleview-property ruleview-newproperty",
    });

    this.newPropSpan = createChild(this.newPropItem, "span", {
      class: "ruleview-propertyname",
      tabindex: "0"
    });

    this.multipleAddedProperties = null;

    this.editor = new InplaceEditor({
      element: this.newPropSpan,
      done: this._onNewProperty,
      destroy: this._newPropertyDestroy,
      advanceChars: ":",
      contentType: InplaceEditor.CONTENT_TYPES.CSS_PROPERTY,
      popup: this.ruleView.popup
    });

    
    this.editor.input.addEventListener("paste",
      blurOnMultipleProperties, false);
  },

  







  _onNewProperty: function(aValue, aCommit) {
    if (!aValue || !aCommit) {
      return;
    }

    
    
    
    this.multipleAddedProperties = parseDeclarations(aValue).filter(d => d.name);

    
    
    this.editor.input.blur();
  },

  





  _newPropertyDestroy: function() {
    
    this.closeBrace.setAttribute("tabindex", "0");

    this.propertyList.removeChild(this.newPropItem);
    delete this.newPropItem;
    delete this.newPropSpan;

    
    
    
    if (this.multipleAddedProperties && this.multipleAddedProperties.length) {
      this.addProperties(this.multipleAddedProperties);
    }
  },

  









  _onSelectorDone: function(aValue, aCommit) {
    if (!aCommit || this.isEditing || aValue === "" ||
        aValue === this.rule.selectorText) {
      return;
    }

    this.isEditing = true;

    this.rule.domRule.modifySelector(aValue).then(isModified => {
      this.isEditing = false;

      if (isModified) {
        this.ruleView.refreshPanel();
      }
    }).then(null, err => {
      this.isEditing = false;
      promiseWarn(err);
    });
  }
};










function TextPropertyEditor(aRuleEditor, aProperty) {
  this.ruleEditor = aRuleEditor;
  this.doc = this.ruleEditor.doc;
  this.popup = this.ruleEditor.ruleView.popup;
  this.prop = aProperty;
  this.prop.editor = this;
  this.browserWindow = this.doc.defaultView.top;
  this.removeOnRevert = this.prop.value === "";

  this._onEnableClicked = this._onEnableClicked.bind(this);
  this._onExpandClicked = this._onExpandClicked.bind(this);
  this._onStartEditing = this._onStartEditing.bind(this);
  this._onNameDone = this._onNameDone.bind(this);
  this._onValueDone = this._onValueDone.bind(this);
  this._onValidate = throttle(this._previewValue, 10, this);
  this.update = this.update.bind(this);

  this._create();
  this.update();
}

TextPropertyEditor.prototype = {
  


  get editing() {
    return !!(this.nameSpan.inplaceEditor || this.valueSpan.inplaceEditor ||
      this.ruleEditor.ruleView.tooltips.isEditing) || this.popup.isOpen;
  },

  


  _create: function() {
    this.element = this.doc.createElementNS(HTML_NS, "li");
    this.element.classList.add("ruleview-property");

    
    this.enable = createChild(this.element, "div", {
      class: "ruleview-enableproperty theme-checkbox",
      tabindex: "-1"
    });

    
    this.expander = createChild(this.element, "span", {
      class: "ruleview-expander theme-twisty"
    });
    this.expander.addEventListener("click", this._onExpandClicked, true);

    this.nameContainer = createChild(this.element, "span", {
      class: "ruleview-namecontainer"
    });

    
    
    this.nameSpan = createChild(this.nameContainer, "span", {
      class: "ruleview-propertyname theme-fg-color5",
      tabindex: this.ruleEditor.isEditable ? "0" : "-1",
    });

    appendText(this.nameContainer, ": ");

    
    
    
    let propertyContainer = createChild(this.element, "span", {
      class: "ruleview-propertycontainer"
    });


    
    
    
    this.valueSpan = createChild(propertyContainer, "span", {
      class: "ruleview-propertyvalue theme-fg-color1",
      tabindex: this.ruleEditor.isEditable ? "0" : "-1",
    });

    
    
    this.valueSpan.textProperty = this.prop;
    this.nameSpan.textProperty = this.prop;

    
    
    
    
    let outputParser = this.ruleEditor.ruleView._outputParser;
    let frag = outputParser.parseCssProperty(this.prop.name, this.prop.value);
    let parsedValue = frag.textContent;

    
    
    this.committed = { name: this.prop.name,
                       value: parsedValue,
                       priority: this.prop.priority };

    appendText(propertyContainer, ";");

    this.warning = createChild(this.element, "div", {
      class: "ruleview-warning",
      hidden: "",
      title: CssLogic.l10n("rule.warning.title"),
    });

    
    
    this.computed = createChild(this.element, "ul", {
      class: "ruleview-computedlist",
    });

    
    if (this.ruleEditor.isEditable) {
      this.enable.addEventListener("click", this._onEnableClicked, true);

      this.nameContainer.addEventListener("click", (aEvent) => {
        
        aEvent.stopPropagation();
        if (aEvent.target === propertyContainer) {
          this.nameSpan.click();
        }
      }, false);

      editableField({
        start: this._onStartEditing,
        element: this.nameSpan,
        done: this._onNameDone,
        destroy: this.update,
        advanceChars: ':',
        contentType: InplaceEditor.CONTENT_TYPES.CSS_PROPERTY,
        popup: this.popup
      });

      
      this.nameContainer.addEventListener("paste",
        blurOnMultipleProperties, false);

      propertyContainer.addEventListener("click", (aEvent) => {
        
        aEvent.stopPropagation();

        if (aEvent.target === propertyContainer) {
          this.valueSpan.click();
        }
      }, false);

      this.valueSpan.addEventListener("click", (event) => {
        let target = event.target;

        if (target.nodeName === "a") {
          event.stopPropagation();
          event.preventDefault();
          this.browserWindow.openUILinkIn(target.href, "tab");
        }
      }, false);

      editableField({
        start: this._onStartEditing,
        element: this.valueSpan,
        done: this._onValueDone,
        destroy: this.update,
        validate: this._onValidate,
        advanceChars: ';',
        contentType: InplaceEditor.CONTENT_TYPES.CSS_VALUE,
        property: this.prop,
        popup: this.popup
      });
    }
  },

  




  get sheetHref() {
    let domRule = this.prop.rule.domRule;
    if (domRule) {
      return domRule.href || domRule.nodeHref;
    }
  },

  




  get sheetURI() {
    if (this._sheetURI === undefined) {
      if (this.sheetHref) {
        this._sheetURI = IOService.newURI(this.sheetHref, null, null);
      } else {
        this._sheetURI = null;
      }
    }

    return this._sheetURI;
  },

  




  resolveURI: function(relativePath) {
    if (this.sheetURI) {
      relativePath = this.sheetURI.resolve(relativePath);
    }
    return relativePath;
  },

  



  getResourceURI: function() {
    let val = this.prop.value;
    let uriMatch = CSS_RESOURCE_RE.exec(val);
    let uri = null;

    if (uriMatch && uriMatch[1]) {
      uri = uriMatch[1];
    }

    return uri;
  },

  


  update: function() {
    if (this.ruleEditor.ruleView.isDestroyed) {
      return;
    }

    if (this.prop.enabled) {
      this.enable.style.removeProperty("visibility");
      this.enable.setAttribute("checked", "");
    } else {
      this.enable.style.visibility = "visible";
      this.enable.removeAttribute("checked");
    }

    this.warning.hidden = this.editing || this.isValid();

    if ((this.prop.overridden || !this.prop.enabled) && !this.editing) {
      this.element.classList.add("ruleview-overridden");
    } else {
      this.element.classList.remove("ruleview-overridden");
    }

    let name = this.prop.name;
    this.nameSpan.textContent = name;

    
    
    let store = this.prop.rule.elementStyle.store;
    let val = store.userProperties.getProperty(this.prop.rule.style, name,
                                               this.prop.value);
    if (this.prop.priority) {
      val += " !" + this.prop.priority;
    }

    let propDirty = store.userProperties.contains(this.prop.rule.style, name);

    if (propDirty) {
      this.element.setAttribute("dirty", "");
    } else {
      this.element.removeAttribute("dirty");
    }

    const sharedSwatchClass = "ruleview-swatch ";
    const colorSwatchClass = "ruleview-colorswatch";
    const bezierSwatchClass = "ruleview-bezierswatch";
    const filterSwatchClass = "ruleview-filterswatch";

    let outputParser = this.ruleEditor.ruleView._outputParser;
    let frag = outputParser.parseCssProperty(name, val, {
      colorSwatchClass: sharedSwatchClass + colorSwatchClass,
      colorClass: "ruleview-color",
      bezierSwatchClass: sharedSwatchClass + bezierSwatchClass,
      bezierClass: "ruleview-bezier",
      filterSwatchClass: sharedSwatchClass + filterSwatchClass,
      filterClass: "ruleview-filter",
      defaultColorType: !propDirty,
      urlClass: "theme-link",
      baseURI: this.sheetURI
    });
    this.valueSpan.innerHTML = "";
    this.valueSpan.appendChild(frag);

    
    this._colorSwatchSpans = this.valueSpan.querySelectorAll("." + colorSwatchClass);
    if (this.ruleEditor.isEditable) {
      for (let span of this._colorSwatchSpans) {
        
        let originalValue = this.valueSpan.textContent;
        
        this.ruleEditor.ruleView.tooltips.colorPicker.addSwatch(span, {
          onPreview: () => this._previewValue(this.valueSpan.textContent),
          onCommit: () => this._applyNewValue(this.valueSpan.textContent),
          onRevert: () => this._applyNewValue(originalValue, false)
        });
      }
    }

    
    this._bezierSwatchSpans = this.valueSpan.querySelectorAll("." + bezierSwatchClass);
    if (this.ruleEditor.isEditable) {
      for (let span of this._bezierSwatchSpans) {
        
        let originalValue = this.valueSpan.textContent;
        
        this.ruleEditor.ruleView.tooltips.cubicBezier.addSwatch(span, {
          onPreview: () => this._previewValue(this.valueSpan.textContent),
          onCommit: () => this._applyNewValue(this.valueSpan.textContent),
          onRevert: () => this._applyNewValue(originalValue, false)
        });
      }
    }

    
    let span = this.valueSpan.querySelector("." + filterSwatchClass);
    if (this.ruleEditor.isEditable) {
      if(span) {
        let originalValue = this.valueSpan.textContent;

        this.ruleEditor.ruleView.tooltips.filterEditor.addSwatch(span, {
          onPreview: () => this._previewValue(this.valueSpan.textContent),
          onCommit: () => this._applyNewValue(this.valueSpan.textContent),
          onRevert: () => this._applyNewValue(originalValue, false)
        });
      }
    }

    
    this._updateComputed();
  },

  _onStartEditing: function() {
    this.element.classList.remove("ruleview-overridden");
    this._previewValue(this.prop.value);
  },

  


  _updateComputed: function () {
    
    while (this.computed.hasChildNodes()) {
      this.computed.removeChild(this.computed.lastChild);
    }

    let showExpander = false;
    for each (let computed in this.prop.computed) {
      
      
      if (computed.name === this.prop.name) {
        continue;
      }

      showExpander = true;

      let li = createChild(this.computed, "li", {
        class: "ruleview-computed"
      });

      if (computed.overridden) {
        li.classList.add("ruleview-overridden");
      }

      createChild(li, "span", {
        class: "ruleview-propertyname theme-fg-color5",
        textContent: computed.name
      });
      appendText(li, ": ");

      let outputParser = this.ruleEditor.ruleView._outputParser;
      let frag = outputParser.parseCssProperty(
        computed.name, computed.value, {
          colorSwatchClass: "ruleview-colorswatch",
          urlClass: "theme-link",
          baseURI: this.sheetURI
        }
      );

      createChild(li, "span", {
        class: "ruleview-propertyvalue theme-fg-color1",
        child: frag
      });

      appendText(li, ";");
    }

    
    if (showExpander) {
      this.expander.style.visibility = "visible";
    } else {
      this.expander.style.visibility = "hidden";
    }
  },

  


  _onEnableClicked: function(aEvent) {
    let checked = this.enable.hasAttribute("checked");
    if (checked) {
      this.enable.removeAttribute("checked");
    } else {
      this.enable.setAttribute("checked", "");
    }
    this.prop.setEnabled(!checked);
    aEvent.stopPropagation();
  },

  


  _onExpandClicked: function(aEvent) {
    this.computed.classList.toggle("styleinspector-open");
    if (this.computed.classList.contains("styleinspector-open")) {
      this.expander.setAttribute("open", "true");
    } else {
      this.expander.removeAttribute("open");
    }
    aEvent.stopPropagation();
  },

  









  _onNameDone: function(aValue, aCommit) {
    if (aCommit && !this.ruleEditor.isEditing) {
      
      
      if (aValue.trim() === "") {
        this.remove();
      } else {
        
        
        let properties = parseDeclarations(aValue);

        if (properties.length) {
          this.prop.setName(properties[0].name);
          if (properties.length > 1) {
            this.prop.setValue(properties[0].value, properties[0].priority);
            this.ruleEditor.addProperties(properties.slice(1), this.prop);
          }
        }
      }
    }
  },

  



  remove: function() {
    if (this._colorSwatchSpans && this._colorSwatchSpans.length) {
      for (let span of this._colorSwatchSpans) {
        this.ruleEditor.ruleView.tooltips.colorPicker.removeSwatch(span);
      }
    }

    this.element.parentNode.removeChild(this.element);
    this.ruleEditor.rule.editClosestTextProperty(this.prop);
    this.nameSpan.textProperty = null;
    this.valueSpan.textProperty = null;
    this.prop.remove();
  },

  








   _onValueDone: function(aValue, aCommit) {
    if (!aCommit && !this.ruleEditor.isEditing) {
       
       if (this.removeOnRevert) {
         this.remove();
       } else {
         this.prop.setValue(this.committed.value, this.committed.priority);
       }
       return;
    }

    let {propertiesToAdd,firstValue} = this._getValueAndExtraProperties(aValue);

    
    let val = parseSingleValue(firstValue);

    this.prop.setValue(val.value, val.priority);
    this.removeOnRevert = false;
    this.committed.value = this.prop.value;
    this.committed.priority = this.prop.priority;

    
    this.ruleEditor.addProperties(propertiesToAdd, this.prop);

    
    
    
    
    
    if (val.value.trim() === "") {
      setTimeout(() => {
        if (!this.editing) {
          this.remove();
        }
      }, 0);
    }
  },

  














  _getValueAndExtraProperties: function(aValue) {
    
    
    
    
    let firstValue = aValue;
    let propertiesToAdd = [];

    let properties = parseDeclarations(aValue);

    
    if (properties.length) {
      
      if (!properties[0].name && properties[0].value) {
        firstValue = properties[0].value;
        propertiesToAdd = properties.slice(1);
      }
      
      
      else if (properties[0].name && properties[0].value) {
        firstValue = properties[0].name + ": " + properties[0].value;
        propertiesToAdd = properties.slice(1);
      }
    }

    return {
      propertiesToAdd: propertiesToAdd,
      firstValue: firstValue
    };
  },

  









  _applyNewValue: function(aValue, markChanged=true) {
    let val = parseSingleValue(aValue);

    if (!markChanged) {
      let store = this.prop.rule.elementStyle.store;
      this.prop.editor.committed.value = aValue;
      store.userProperties.setProperty(this.prop.rule.style,
                                       this.prop.rule.name, aValue);
    }

    this.prop.setValue(val.value, val.priority, markChanged);
    this.removeOnRevert = false;
    this.committed.value = this.prop.value;
    this.committed.priority = this.prop.priority;
  },

  



  _previewValue: function(aValue) {
    
    
    if (!this.editing || this.ruleEditor.isEditing) {
      return;
    }

    let val = parseSingleValue(aValue);
    this.ruleEditor.rule.previewPropertyValue(this.prop, val.value, val.priority);
  },

  





  isValid: function() {
    return domUtils.cssPropertyIsValid(this.prop.name, this.prop.value);
  }
};





function UserProperties() {
  this.map = new Map();
}

UserProperties.prototype = {
  












  getProperty: function(aStyle, aName, aDefault) {
    let key = this.getKey(aStyle);
    let entry = this.map.get(key, null);

    if (entry && aName in entry) {
      return entry[aName];
    }
    return aDefault;
  },

  









  setProperty: function(aStyle, aName, aUserValue) {
    let key = this.getKey(aStyle, aName);
    let entry = this.map.get(key, null);

    if (entry) {
      entry[aName] = aUserValue;
    } else {
      let props = {};
      props[aName] = aUserValue;
      this.map.set(key, props);
    }
  },

  







  contains: function(aStyle, aName) {
    let key = this.getKey(aStyle, aName);
    let entry = this.map.get(key, null);
    return !!entry && aName in entry;
  },

  getKey: function(aStyle, aName) {
    return aStyle.actorID + ":" + aName;
  },

  clear: function() {
    this.map.clear();
  }
};















function createChild(aParent, aTag, aAttributes) {
  let elt = aParent.ownerDocument.createElementNS(HTML_NS, aTag);
  for (let attr in aAttributes) {
    if (aAttributes.hasOwnProperty(attr)) {
      if (attr === "textContent") {
        elt.textContent = aAttributes[attr];
      } else if(attr === "child") {
        elt.appendChild(aAttributes[attr]);
      } else {
        elt.setAttribute(attr, aAttributes[attr]);
      }
    }
  }
  aParent.appendChild(elt);
  return elt;
}

function createMenuItem(aMenu, aAttributes) {
  let item = aMenu.ownerDocument.createElementNS(XUL_NS, "menuitem");

  item.setAttribute("label", _strings.GetStringFromName(aAttributes.label));
  item.setAttribute("accesskey", _strings.GetStringFromName(aAttributes.accesskey));
  item.addEventListener("command", aAttributes.command);

  if (aAttributes.type) {
    item.setAttribute("type", aAttributes.type);
  }

  aMenu.appendChild(item);

  return item;
}

function setTimeout() {
  let window = Services.appShell.hiddenDOMWindow;
  return window.setTimeout.apply(window, arguments);
}

function clearTimeout() {
  let window = Services.appShell.hiddenDOMWindow;
  return window.clearTimeout.apply(window, arguments);
}

function throttle(func, wait, scope) {
  var timer = null;
  return function() {
    if(timer) {
      clearTimeout(timer);
    }
    var args = arguments;
    timer = setTimeout(function() {
      timer = null;
      func.apply(scope, args);
    }, wait);
  };
}





function blurOnMultipleProperties(e) {
  setTimeout(() => {
    let props = parseDeclarations(e.target.value);
    if (props.length > 1) {
      e.target.blur();
    }
  }, 0);
}




function appendText(aParent, aText) {
  aParent.appendChild(aParent.ownerDocument.createTextNode(aText));
}








function getParentTextPropertyHolder(node) {
  while (true) {
    if (!node || !node.classList) {
      return null;
    }
    if (node.classList.contains("ruleview-property")) {
      return node;
    }
    node = node.parentNode;
  }
}






function getParentTextProperty(node) {
  let parent = getParentTextPropertyHolder(node);
  if (!parent) {
    return null;
  }

  let propValue = parent.querySelector(".ruleview-propertyvalue");
  if (!propValue) {
    return null;
  }

  return propValue.textProperty;
}









function getPropertyNameAndValue(node) {
  while (true) {
    if (!node || !node.classList) {
      return null;
    }
    
    if (node.classList.contains("ruleview-computed") ||
        node.classList.contains("ruleview-property")) {
      return {
        name: node.querySelector(".ruleview-propertyname").textContent,
        value: node.querySelector(".ruleview-propertyvalue").textContent
      };
    }
    node = node.parentNode;
  }
}

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});

XPCOMUtils.defineLazyGetter(this, "_strings", function() {
  return Services.strings.createBundle(
    "chrome://global/locale/devtools/styleinspector.properties");
});

XPCOMUtils.defineLazyGetter(this, "domUtils", function() {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

loader.lazyGetter(this, "AutocompletePopup", () => require("devtools/shared/autocomplete-popup").AutocompletePopup);
