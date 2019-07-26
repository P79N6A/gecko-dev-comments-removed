





"use strict";

const {Cc, Ci, Cu} = require("chrome");
const promise = require("sdk/core/promise");

let {CssLogic} = require("devtools/styleinspector/css-logic");
let {InplaceEditor, editableField, editableItem} = require("devtools/shared/inplace-editor");
let {ELEMENT_STYLE, PSEUDO_ELEMENTS} = require("devtools/server/actors/styles");
let {colorUtils} = require("devtools/css-color");
let {gDevTools} = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";







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
  }, false);
  gDummyPromise = deferred.promise;
  return gDummyPromise;
}



































function ElementStyle(aElement, aStore, aPageStyle)
{
  this.element = aElement;
  this.store = aStore || {};
  this.pageStyle = aPageStyle;

  
  
  if (!("userProperties" in this.store)) {
    this.store.userProperties = new UserProperties();
  }

  if (!("disabled" in this.store)) {
    this.store.disabled = new WeakMap();
  }

  
  
  
  this.dummyElementPromise = createDummyDocument().then(document => {
    this.dummyElement = document.createElementNS(this.element.namespaceURI,
                                                 this.element.tagName);
    document.documentElement.appendChild(this.dummyElement);
    return this.dummyElement;
  }).then(null, promiseWarn);
}

exports._ElementStyle = ElementStyle;

ElementStyle.prototype = {

  
  element: null,

  
  
  dummyElement: null,

  destroy: function()
  {
    this.dummyElement = null;
    this.dummyElementPromise.then(dummyElement => {
      if (dummyElement.parentNode) {
        dummyElement.parentNode.removeChild(dummyElement);
      }
      this.dummyElementPromise = null;
    });
  },

  



  _changed: function ElementStyle_changed()
  {
    if (this.onChanged) {
      this.onChanged();
    }
  },

  






  populate: function ElementStyle_populate()
  {
    let populated = this.pageStyle.getApplied(this.element, {
      inherited: true,
      matchedSelectors: true
    }).then(entries => {
      
      return this.dummyElementPromise.then(() => {
        if (this.populated != populated) {
          
          return promise.reject("unused");
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

  


   _sortRulesForPseudoElement: function ElementStyle_sortRulesForPseudoElement()
   {
      this.rules = this.rules.sort((a, b) => {
        return (a.pseudoElement || "z") > (b.pseudoElement || "z");
      });
   },

  








  _maybeAddRule: function ElementStyle_maybeAddRule(aOptions)
  {
    
    
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

  


  markOverriddenAll: function ElementStyle_markOverriddenAll()
  {
    this.markOverridden();
    for (let pseudo of PSEUDO_ELEMENTS) {
      this.markOverridden(pseudo);
    }
  },

  






  markOverridden: function ElementStyle_markOverridden(pseudo="")
  {
    
    
    let textProps = [];
    for (let rule of this.rules) {
      if (rule.pseudoElement == pseudo) {
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

  










  _updatePropertyOverridden: function ElementStyle_updatePropertyOverridden(aProp)
  {
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













function Rule(aElementStyle, aOptions)
{
  this.elementStyle = aElementStyle;
  this.domRule = aOptions.rule || null;
  this.style = aOptions.rule;
  this.matchedSelectors = aOptions.matchedSelectors || [];
  this.pseudoElement = aOptions.pseudoElement || "";

  this.inherited = aOptions.inherited || null;
  this._modificationDepth = 0;

  if (this.domRule) {
    let parentRule = this.domRule.parentRule;
    if (parentRule && parentRule.type == Ci.nsIDOMCSSRule.MEDIA_RULE) {
      this.mediaText = parentRule.mediaText;
    }
  }

  
  
  this.textProps = this._getTextProperties();
  this.textProps = this.textProps.concat(this._getDisabledProperties());
}

Rule.prototype = {
  mediaText: "",

  get title()
  {
    if (this._title) {
      return this._title;
    }
    this._title = CssLogic.shortSource(this.sheet);
    if (this.domRule.type !== ELEMENT_STYLE) {
      this._title += ":" + this.ruleLine;
    }

    this._title = this._title + (this.mediaText ? " @media " + this.mediaText : "");
    return this._title;
  },

  get inheritedSource()
  {
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

  get selectorText()
  {
    return this.domRule.selectors ? this.domRule.selectors.join(", ") : CssLogic.l10n("rule.sourceElement");
  },

  


  get sheet()
  {
    return this.domRule ? this.domRule.parentStyleSheet : null;
  },

  


  get ruleLine()
  {
    return this.domRule ? this.domRule.line : null;
  },

  






  matches: function Rule_matches(aOptions)
  {
    return this.style === aOptions.rule;
  },

  









  createProperty: function Rule_createProperty(aName, aValue, aPriority)
  {
    let prop = new TextProperty(this, aName, aValue, aPriority);
    this.textProps.push(prop);
    this.applyProperties();
    return prop;
  },

  









  applyProperties: function Rule_applyProperties(aModifications, aName)
  {
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
      for (let cssProp of this._parseCSSText(this.style.cssText)) {
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

        if (aName && textProp.name == aName) {
          store.userProperties.setProperty(
            this.style,
            textProp.name,
            cssProp.value);
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

  







  setPropertyName: function Rule_setPropertyName(aProperty, aName)
  {
    if (aName === aProperty.name) {
      return;
    }
    let modifications = this.style.startModifyingProperties();
    modifications.removeProperty(aProperty.name);
    aProperty.name = aName;
    this.applyProperties(modifications, aName);
  },

  









  setPropertyValue: function Rule_setPropertyValue(aProperty, aValue, aPriority)
  {
    if (aValue === aProperty.value && aPriority === aProperty.priority) {
      return;
    }

    aProperty.value = aValue;
    aProperty.priority = aPriority;
    this.applyProperties(null, aProperty.name);
  },

  


  setPropertyEnabled: function Rule_enableProperty(aProperty, aValue)
  {
    aProperty.enabled = !!aValue;
    let modifications = this.style.startModifyingProperties();
    if (!aProperty.enabled) {
      modifications.removeProperty(aProperty.name);
    }
    this.applyProperties(modifications);
  },

  



  removeProperty: function Rule_removeProperty(aProperty)
  {
    this.textProps = this.textProps.filter(function(prop) prop != aProperty);
    let modifications = this.style.startModifyingProperties();
    modifications.removeProperty(aProperty.name);
    
    
    this.applyProperties(modifications);
  },

  _parseCSSText: function Rule_parseProperties(aCssText)
  {
    let lines = aCssText.match(CSS_LINE_RE);
    let props = [];

    for (let line of lines) {
      let [, name, value, priority] = CSS_PROP_RE.exec(line) || []
      if (!name || !value) {
        continue;
      }

      props.push({
        name: name,
        value: value,
        priority: priority || ""
      });
    }
    return props;
  },

  



  _getTextProperties: function Rule_getTextProperties()
  {
    let textProps = [];
    let store = this.elementStyle.store;
    let props = this._parseCSSText(this.style.cssText);
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

  


  _getDisabledProperties: function Rule_getDisabledProperties()
  {
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

  



  refresh: function Rule_refresh(aOptions)
  {
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

  






















  _updateTextProperty: function Rule__updateTextProperty(aNewProp) {
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

  









  editClosestTextProperty: function Rule__editClosestTextProperty(aTextProperty)
  {
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














function TextProperty(aRule, aName, aValue, aPriority)
{
  this.rule = aRule;
  this.name = aName;
  this.value = colorUtils.processCSSString(aValue);
  this.priority = aPriority;
  this.enabled = true;
  this.updateComputed();
}

TextProperty.prototype = {
  



  updateEditor: function TextProperty_updateEditor()
  {
    if (this.editor) {
      this.editor.update();
    }
  },

  


  updateComputed: function TextProperty_updateComputed()
  {
    if (!this.name) {
      return;
    }

    
    
    
    let dummyElement = this.rule.elementStyle.dummyElement;
    let dummyStyle = dummyElement.style;
    dummyStyle.cssText = "";
    dummyStyle.setProperty(this.name, this.value, this.priority);

    this.computed = [];
    for (let i = 0, n = dummyStyle.length; i < n; i++) {
      let prop = dummyStyle.item(i);
      this.computed.push({
        textProp: this,
        name: prop,
        value: dummyStyle.getPropertyValue(prop),
        priority: dummyStyle.getPropertyPriority(prop),
      });
    }
  },

  






  set: function TextProperty_set(aOther)
  {
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

  setValue: function TextProperty_setValue(aValue, aPriority)
  {
    this.rule.setPropertyValue(this, aValue, aPriority);
    this.updateEditor();
  },

  setName: function TextProperty_setName(aName)
  {
    this.rule.setPropertyName(this, aName);
    this.updateEditor();
  },

  setEnabled: function TextProperty_setEnabled(aValue)
  {
    this.rule.setPropertyEnabled(this, aValue);
    this.updateEditor();
  },

  remove: function TextProperty_remove()
  {
    this.rule.removeProperty(this);
  },
};


































function CssRuleView(aDoc, aStore, aPageStyle)
{
  this.doc = aDoc;
  this.store = aStore || {};
  this.pageStyle = aPageStyle;
  this.element = this.doc.createElementNS(HTML_NS, "div");
  this.element.className = "ruleview devtools-monospace";
  this.element.flex = 1;

  this._buildContextMenu = this._buildContextMenu.bind(this);
  this._contextMenuUpdate = this._contextMenuUpdate.bind(this);
  this._onSelectAll = this._onSelectAll.bind(this);
  this._onCopy = this._onCopy.bind(this);

  this.element.addEventListener("copy", this._onCopy);

  this._handlePrefChange = this._handlePrefChange.bind(this);
  gDevTools.on("pref-changed", this._handlePrefChange);

  let options = {
    fixedWidth: true,
    autoSelect: true,
    theme: "auto"
  };
  this.popup = new AutocompletePopup(aDoc.defaultView.parent.document, options);

  this._buildContextMenu();
  this._showEmpty();
}

exports.CssRuleView = CssRuleView;

CssRuleView.prototype = {
  
  _viewedElement: null,

  


  _buildContextMenu: function() {
    let doc = this.doc.defaultView.parent.document;

    this._contextmenu = doc.createElementNS(XUL_NS, "menupopup");
    this._contextmenu.addEventListener("popupshowing", this._contextMenuUpdate);
    this._contextmenu.id = "rule-view-context-menu";

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

    let popupset = doc.documentElement.querySelector("popupset");
    if (!popupset) {
      popupset = doc.createElementNS(XUL_NS, "popupset");
      doc.documentElement.appendChild(popupset);
    }

    popupset.appendChild(this._contextmenu);
  },

  



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

    this.menuitemCopy.disabled = !copy;
  },

  


  _onSelectAll: function()
  {
    let win = this.doc.defaultView;
    let selection = win.getSelection();

    selection.selectAllChildren(this.doc.documentElement);
  },

  





  _onCopy: function(event)
  {
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
        text = win.getSelection().toString();

        
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

  setPageStyle: function(aPageStyle) {
    this.pageStyle = aPageStyle;
  },

  


  get isEditing() {
    return this.element.querySelectorAll(".styleinspector-propertyeditor").length > 0;
  },

  _handlePrefChange: function(event, data) {
    if (data.pref == "devtools.defaultColorUnit") {
      let element = this._viewedElement;
      this._viewedElement = null;
      this.highlight(element);
    }
  },

  destroy: function CssRuleView_destroy()
  {
    this.clear();

    gDevTools.off("pref-changed", this._handlePrefChange);

    this.element.removeEventListener("copy", this._onCopy);
    delete this._onCopy;

    
    if (this._contextmenu) {
      
      this.menuitemSelectAll.removeEventListener("command", this._onSelectAll);
      this.menuitemSelectAll = null;

      
      this.menuitemCopy.removeEventListener("command", this._onCopy);
      this.menuitemCopy = null;

      
      this._contextmenu.removeEventListener("popupshowing", this._contextMenuUpdate);
      this._contextmenu.parentNode.removeChild(this._contextmenu);
      this._contextmenu = null;
    }

    
    this.doc.popupNode = null;

    if (this.element.parentNode) {
      this.element.parentNode.removeChild(this.element);
    }

    if (this.elementStyle) {
      this.elementStyle.destroy();
    }

    this.popup.destroy();
  },

  





  highlight: function CssRuleView_highlight(aElement)
  {
    if (this._viewedElement === aElement) {
      return promise.resolve(undefined);
    }

    this.clear();

    if (this._elementStyle) {
      delete this._elementStyle;
    }

    this._viewedElement = aElement;
    if (!this._viewedElement) {
      this._showEmpty();
      return promise.resolve(undefined);
    }

    this._elementStyle = new ElementStyle(aElement, this.store, this.pageStyle);
    return this._populate().then(() => {
      this._elementStyle.onChanged = () => {
        this._changed();
      };
    }).then(null, console.error);
  },

  


  nodeChanged: function CssRuleView_nodeChanged()
  {
    
    if (this.isEditing || !this._elementStyle) {
      return;
    }

    this._clearRules();

    
    this._populate();
  },

  _populate: function() {
    let elementStyle = this._elementStyle;
    return this._elementStyle.populate().then(() => {
      if (this._elementStyle != elementStyle) {
        return promise.reject("element changed");
      }
      this._createEditors();


      
      var evt = this.doc.createEvent("Events");
      evt.initEvent("CssRuleViewRefreshed", true, false);
      this.element.dispatchEvent(evt);
      return undefined;
    }).then(null, promiseWarn);
  },

  


  _showEmpty: function CssRuleView_showEmpty()
  {
    if (this.doc.getElementById("noResults") > 0) {
      return;
    }

    createChild(this.element, "div", {
      id: "noResults",
      textContent: CssLogic.l10n("rule.empty")
    });
  },

  


  _clearRules: function CssRuleView_clearRules()
  {
    while (this.element.hasChildNodes()) {
      this.element.removeChild(this.element.lastChild);
    }
  },

  


  clear: function CssRuleView_clear()
  {
    this._clearRules();
    this._viewedElement = null;
    this._elementStyle = null;
  },

  



  _changed: function CssRuleView_changed()
  {
    var evt = this.doc.createEvent("Events");
    evt.initEvent("CssRuleViewChanged", true, false);
    this.element.dispatchEvent(evt);
  },

  


  get selectedElementLabel ()
  {
    if (this._selectedElementLabel) {
      return this._selectedElementLabel;
    }
    this._selectedElementLabel = CssLogic.l10n("rule.selectedElement");
    return this._selectedElementLabel;
  },

  


  get pseudoElementLabel ()
  {
    if (this._pseudoElementLabel) {
      return this._pseudoElementLabel;
    }
    this._pseudoElementLabel = CssLogic.l10n("rule.pseudoElement");
    return this._pseudoElementLabel;
  },

  togglePseudoElementVisibility: function(value)
  {
    this._showPseudoElements = !!value;
    let isOpen = this.showPseudoElements;

    Services.prefs.setBoolPref("devtools.inspector.show_pseudo_elements",
      isOpen);

    this.element.classList.toggle("show-pseudo-elements", isOpen);

    if (this.pseudoElementTwisty) {
      if (isOpen) {
        this.pseudoElementTwisty.setAttribute("open", "true");
      }
      else {
        this.pseudoElementTwisty.removeAttribute("open");
      }
    }
  },

  get showPseudoElements ()
  {
    if (this._showPseudoElements === undefined) {
      this._showPseudoElements =
        Services.prefs.getBoolPref("devtools.inspector.show_pseudo_elements");
    }
    return this._showPseudoElements;
  },

  _getRuleViewHeaderClassName: function(isPseudo) {
    let baseClassName = "theme-gutter ruleview-header";
    return isPseudo ? baseClassName + " ruleview-expandable-header" : baseClassName;
  },

  


  _createEditors: function CssRuleView_createEditors()
  {
    
    
    let lastInheritedSource = "";
    let seenPseudoElement = false;
    let seenNormalElement = false;

    for (let rule of this._elementStyle.rules) {
      if (rule.domRule.system) {
        continue;
      }

      
      if (seenPseudoElement && !seenNormalElement && !rule.pseudoElement) {
        seenNormalElement = true;
        let div = this.doc.createElementNS(HTML_NS, "div");
        div.className = this._getRuleViewHeaderClassName();
        div.textContent = this.selectedElementLabel;
        this.element.appendChild(div);
      }

      let inheritedSource = rule.inheritedSource;
      if (inheritedSource != lastInheritedSource) {
        let div = this.doc.createElementNS(HTML_NS, "div");
        div.className = this._getRuleViewHeaderClassName();
        div.textContent = inheritedSource;
        lastInheritedSource = inheritedSource;
        this.element.appendChild(div);
      }

      if (!seenPseudoElement && rule.pseudoElement) {
        seenPseudoElement = true;

        let div = this.doc.createElementNS(HTML_NS, "div");
        div.className = this._getRuleViewHeaderClassName(true);
        div.textContent = this.pseudoElementLabel;
        div.addEventListener("dblclick", () => {
          this.togglePseudoElementVisibility(!this.showPseudoElements);
        }, false);

        let twisty = this.pseudoElementTwisty =
          this.doc.createElementNS(HTML_NS, "span");
        twisty.className = "ruleview-expander theme-twisty";
        twisty.addEventListener("click", () => {
          this.togglePseudoElementVisibility(!this.showPseudoElements);
        }, false);

        div.insertBefore(twisty, div.firstChild);
        this.element.appendChild(div);
      }

      if (!rule.editor) {
        rule.editor = new RuleEditor(this, rule);
      }

      this.element.appendChild(rule.editor.element);
    }

    this.togglePseudoElementVisibility(this.showPseudoElements);
  },

};










function RuleEditor(aRuleView, aRule)
{
  this.ruleView = aRuleView;
  this.doc = this.ruleView.doc;
  this.rule = aRule;

  this._onNewProperty = this._onNewProperty.bind(this);
  this._newPropertyDestroy = this._newPropertyDestroy.bind(this);

  this._create();
}

RuleEditor.prototype = {
  _create: function RuleEditor_create()
  {
    this.element = this.doc.createElementNS(HTML_NS, "div");
    this.element.className = "ruleview-rule theme-separator";
    this.element._ruleEditor = this;
    if (this.rule.pseudoElement) {
      this.element.classList.add("ruleview-rule-pseudo-element");
    }

    
    
    this.element.style.position = "relative";

    
    let source = createChild(this.element, "div", {
      class: "ruleview-rule-source theme-link"
    });
    source.addEventListener("click", function() {
      let rule = this.rule.domRule;
      let evt = this.doc.createEvent("CustomEvent");
      evt.initCustomEvent("CssRuleViewCSSLinkClicked", true, false, {
        rule: rule,
      });
      this.element.dispatchEvent(evt);
    }.bind(this));
    let sourceLabel = this.doc.createElementNS(XUL_NS, "label");
    sourceLabel.setAttribute("crop", "center");
    sourceLabel.setAttribute("value", this.rule.title);
    sourceLabel.setAttribute("tooltiptext", this.rule.title);
    source.appendChild(sourceLabel);

    let code = createChild(this.element, "div", {
      class: "ruleview-code"
    });

    let header = createChild(code, "div", {});

    this.selectorText = createChild(header, "span", {
      class: "ruleview-selector theme-fg-color3"
    });

    this.openBrace = createChild(header, "span", {
      class: "ruleview-ruleopen",
      textContent: " {"
    });

    code.addEventListener("click", function() {
      let selection = this.doc.defaultView.getSelection();
      if (selection.isCollapsed) {
        this.newProperty();
      }
    }.bind(this), false);

    this.element.addEventListener("mousedown", function() {
      this.doc.defaultView.focus();
    }.bind(this), false);

    this.element.addEventListener("contextmenu", event => {
      try {
        
        
        this.doc.popupNode = event.explicitOriginalTarget;
        let win = this.doc.defaultView;
        win.focus();

        this.ruleView._contextmenu.openPopup(
          event.target.ownerDocument.documentElement,
          "overlap", event.clientX, event.clientY, true, false, null);
      } catch(e) {
        console.error(e);
      }
    }, false);

    this.propertyList = createChild(code, "ul", {
      class: "ruleview-propertylist"
    });

    this.populate();

    this.closeBrace = createChild(code, "div", {
      class: "ruleview-ruleclose",
      tabindex: "0",
      textContent: "}"
    });

    
    editableItem({ element: this.closeBrace }, function(aElement) {
      this.newProperty();
    }.bind(this));
  },

  


  populate: function RuleEditor_populate()
  {
    
    while (this.selectorText.hasChildNodes()) {
      this.selectorText.removeChild(this.selectorText.lastChild);
    }

    
    
    
    if (this.rule.domRule.type === ELEMENT_STYLE) {
      this.selectorText.textContent = this.rule.selectorText;
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

  











  addProperty: function RuleEditor_addProperty(aName, aValue, aPriority)
  {
    let prop = this.rule.createProperty(aName, aValue, aPriority);
    let editor = new TextPropertyEditor(this, prop);
    this.propertyList.appendChild(editor.element);
    return prop;
  },

  




  newProperty: function RuleEditor_newProperty()
  {
    
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

    new InplaceEditor({
      element: this.newPropSpan,
      done: this._onNewProperty,
      destroy: this._newPropertyDestroy,
      advanceChars: ":",
      contentType: InplaceEditor.CONTENT_TYPES.CSS_PROPERTY,
      popup: this.ruleView.popup
    });
  },

  








  _onNewProperty: function RuleEditor__onNewProperty(aValue, aCommit)
  {
    if (!aValue || !aCommit) {
      return;
    }

    
    let prop = this.rule.createProperty(aValue, "", "");
    let editor = new TextPropertyEditor(this, prop);
    this.propertyList.appendChild(editor.element);
    editor.valueSpan.click();
  },

  


  _newPropertyDestroy: function RuleEditor__newPropertyDestroy()
  {
    
    this.closeBrace.setAttribute("tabindex", "0");

    this.propertyList.removeChild(this.newPropItem);
    delete this.newPropItem;
    delete this.newPropSpan;
  }
};










function TextPropertyEditor(aRuleEditor, aProperty)
{
  this.ruleEditor = aRuleEditor;
  this.doc = this.ruleEditor.doc;
  this.popup = this.ruleEditor.ruleView.popup;
  this.prop = aProperty;
  this.prop.editor = this;
  this.browserWindow = this.doc.defaultView.top;
  this.removeOnRevert = this.prop.value === "";

  let domRule = this.prop.rule.domRule;
  let href = domRule ? domRule.href : null;
  if (href) {
    this.sheetURI = IOService.newURI(href, null, null);
  }

  this._onEnableClicked = this._onEnableClicked.bind(this);
  this._onExpandClicked = this._onExpandClicked.bind(this);
  this._onStartEditing = this._onStartEditing.bind(this);
  this._onNameDone = this._onNameDone.bind(this);
  this._onValueDone = this._onValueDone.bind(this);
  this._onValidate = throttle(this._livePreview, 10, this, this.browserWindow);

  this._create();
  this.update();
}

TextPropertyEditor.prototype = {
  


  get editing() {
    return !!(this.nameSpan.inplaceEditor || this.valueSpan.inplaceEditor);
  },

  


  _create: function TextPropertyEditor_create()
  {
    this.element = this.doc.createElementNS(HTML_NS, "li");
    this.element.classList.add("ruleview-property");

    
    this.enable = createChild(this.element, "div", {
      class: "ruleview-enableproperty theme-checkbox",
      tabindex: "-1"
    });
    this.enable.addEventListener("click", this._onEnableClicked, true);

    
    this.expander = createChild(this.element, "span", {
      class: "ruleview-expander theme-twisty"
    });
    this.expander.addEventListener("click", this._onExpandClicked, true);

    this.nameContainer = createChild(this.element, "span", {
      class: "ruleview-namecontainer"
    });
    this.nameContainer.addEventListener("click", (aEvent) => {
      
      aEvent.stopPropagation();
      if (aEvent.target === propertyContainer) {
        this.nameSpan.click();
      }
    }, false);

    
    
    this.nameSpan = createChild(this.nameContainer, "span", {
      class: "ruleview-propertyname theme-fg-color5",
      tabindex: "0",
    });

    editableField({
      start: this._onStartEditing,
      element: this.nameSpan,
      done: this._onNameDone,
      destroy: this.update.bind(this),
      advanceChars: ':',
      contentType: InplaceEditor.CONTENT_TYPES.CSS_PROPERTY,
      popup: this.popup
    });

    appendText(this.nameContainer, ": ");

    
    
    
    let propertyContainer = createChild(this.element, "span", {
      class: "ruleview-propertycontainer"
    });
    propertyContainer.addEventListener("click", (aEvent) => {
      
      aEvent.stopPropagation();
      if (aEvent.target === propertyContainer) {
        this.valueSpan.click();
      }
    }, false);

    
    
    
    this.valueSpan = createChild(propertyContainer, "span", {
      class: "ruleview-propertyvalue theme-fg-color1",
      tabindex: "0",
    });

    
    
    this.committed = { name: this.prop.name,
                       value: this.prop.value,
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

    editableField({
      start: this._onStartEditing,
      element: this.valueSpan,
      done: this._onValueDone,
      destroy: this.update.bind(this),
      validate: this._onValidate,
      advanceChars: ';',
      contentType: InplaceEditor.CONTENT_TYPES.CSS_VALUE,
      property: this.prop,
      popup: this.popup
    });
  },

  




  resolveURI: function(relativePath)
  {
    if (this.sheetURI) {
      relativePath = this.sheetURI.resolve(relativePath);
    }
    return relativePath;
  },

  



  getResourceURI: function()
  {
    let val = this.prop.value;
    let uriMatch = CSS_RESOURCE_RE.exec(val);
    let uri = null;

    if (uriMatch && uriMatch[1]) {
      uri = uriMatch[1];
    }

    return uri;
  },

  


  update: function TextPropertyEditor_update()
  {
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

    
    
    let val = this.prop.value;
    if (this.prop.priority) {
      val += " !" + this.prop.priority;
    }
    
    
    let resourceURI = this.getResourceURI();
    if (resourceURI) {
      this.valueSpan.textContent = "";

      appendText(this.valueSpan, val.split(resourceURI)[0]);

      let a = createChild(this.valueSpan, "a",  {
        target: "_blank",
        class: "theme-link",
        textContent: resourceURI,
        href: this.resolveURI(resourceURI)
      });

      a.addEventListener("click", (aEvent) => {

        
        aEvent.stopPropagation();
        aEvent.preventDefault();

        this.browserWindow.openUILinkIn(aEvent.target.href, "tab");

      }, false);

      appendText(this.valueSpan, val.split(resourceURI)[1]);
    } else {
      this.valueSpan.textContent = val;
    }

    let store = this.prop.rule.elementStyle.store;
    let propDirty = store.userProperties.contains(this.prop.rule.style, name);
    if (propDirty) {
      this.element.setAttribute("dirty", "");
    } else {
      this.element.removeAttribute("dirty");
    }

    
    this._updateComputed();
  },

  _onStartEditing: function TextPropertyEditor_onStartEditing()
  {
    this.element.classList.remove("ruleview-overridden");
    this._livePreview(this.prop.value);
  },

  


  _updateComputed: function TextPropertyEditor_updateComputed()
  {
    
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

      createChild(li, "span", {
        class: "ruleview-propertyvalue theme-fg-color1",
        textContent: computed.value
      });
      appendText(li, ";");
    }

    
    if (showExpander) {
      this.expander.style.visibility = "visible";
    } else {
      this.expander.style.visibility = "hidden";
    }
  },

  


  _onEnableClicked: function TextPropertyEditor_onEnableClicked(aEvent)
  {
    let checked = this.enable.hasAttribute("checked");
    if (checked) {
      this.enable.removeAttribute("checked");
    } else {
      this.enable.setAttribute("checked", "");
    }
    this.prop.setEnabled(!checked);
    aEvent.stopPropagation();
  },

  


  _onExpandClicked: function TextPropertyEditor_onExpandClicked(aEvent)
  {
    this.computed.classList.toggle("styleinspector-open");
    if (this.computed.classList.contains("styleinspector-open")) {
      this.expander.setAttribute("open", "true");
    } else {
      this.expander.removeAttribute("open");
    }
    aEvent.stopPropagation();
  },

  









  _onNameDone: function TextPropertyEditor_onNameDone(aValue, aCommit)
  {
    if (aCommit) {
      if (aValue.trim() === "") {
        this.remove();
      } else {
        this.prop.setName(aValue);
      }
    }
  },

  







  _parseValue: function TextPropertyEditor_parseValue(aValue)
  {
    let pieces = aValue.split("!", 2);
    return {
      value: pieces[0].trim(),
      priority: (pieces.length > 1 ? pieces[1].trim() : "")
    };
  },

  



  remove: function TextPropertyEditor_remove()
  {
    this.element.parentNode.removeChild(this.element);
    this.ruleEditor.rule.editClosestTextProperty(this.prop);
    this.prop.remove();
  },

  








   _onValueDone: function PropertyEditor_onValueDone(aValue, aCommit)
  {
    if (aCommit) {
      let val = this._parseValue(aValue);
      
      if (val.value.trim() === "") {
        this.remove();
      } else {
        this.prop.setValue(val.value, val.priority);
        this.removeOnRevert = false;
        this.committed.value = this.prop.value;
        this.committed.priority = this.prop.priority;
      }
    } else {
      
      if (this.removeOnRevert) {
        this.remove();
      } else {
        this.prop.setValue(this.committed.value, this.committed.priority);
      }
    }
  },

  





  _livePreview: function TextPropertyEditor_livePreview(aValue)
  {
    
    if (!this.editing) {
      return;
    }

    let val = this._parseValue(aValue);

    
    
    this.ruleEditor.rule.setPropertyValue(this.prop, val.value, val.priority);
  },

  









  isValid: function TextPropertyEditor_isValid(aValue)
  {
    let name = this.prop.name;
    let value = typeof aValue == "undefined" ? this.prop.value : aValue;
    let val = this._parseValue(value);

    let style = this.doc.createElementNS(HTML_NS, "div").style;
    let prefs = Services.prefs;

    
    let prefVal = prefs.getBoolPref("layout.css.report_errors");
    prefs.setBoolPref("layout.css.report_errors", false);

    let validValue = false;
    try {
      style.setProperty(name, val.value, val.priority);
      validValue = style.getPropertyValue(name) !== "" || val.value === "";
    } finally {
      prefs.setBoolPref("layout.css.report_errors", prefVal);
    }
    return validValue;
  }
};





function UserProperties()
{
  this.map = new Map();
}

UserProperties.prototype = {
  













  getProperty: function(aStyle, aName, aDefault) {
    let key = this.getKey(aStyle);
    let entry = this.map.get(key, null);

    if (entry && aName in entry) {
      let item = entry[aName];
      if (item != aDefault) {
        delete entry[aName];
        return aDefault;
      }
      return item;
    }
    return aDefault;
  },

  









  setProperty: function(aStyle, aName, aUserValue) {
    let key = this.getKey(aStyle);
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
    let key = this.getKey(aStyle);
    let entry = this.map.get(key, null);
    return !!entry && aName in entry;
  },

  getKey: function(aStyle) {
    return aStyle.href + ":" + aStyle.line;
  },
};















function createChild(aParent, aTag, aAttributes)
{
  let elt = aParent.ownerDocument.createElementNS(HTML_NS, aTag);
  for (let attr in aAttributes) {
    if (aAttributes.hasOwnProperty(attr)) {
      if (attr === "textContent") {
        elt.textContent = aAttributes[attr];
      } else {
        elt.setAttribute(attr, aAttributes[attr]);
      }
    }
  }
  aParent.appendChild(elt);
  return elt;
}

function createMenuItem(aMenu, aAttributes)
{
  let item = aMenu.ownerDocument.createElementNS(XUL_NS, "menuitem");

  item.setAttribute("label", _strings.GetStringFromName(aAttributes.label));
  item.setAttribute("accesskey", _strings.GetStringFromName(aAttributes.accesskey));
  item.addEventListener("command", aAttributes.command);

  aMenu.appendChild(item);

  return item;
}


function throttle(func, wait, scope, window) {
  var timer = null;
  return function() {
    if(timer) {
      window.clearTimeout(timer);
    }
    var args = arguments;
    timer = window.setTimeout(function() {
      timer = null;
      func.apply(scope, args);
    }, wait);
  };
}





function appendText(aParent, aText)
{
  aParent.appendChild(aParent.ownerDocument.createTextNode(aText));
}

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});

XPCOMUtils.defineLazyGetter(this, "_strings", function() {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/styleinspector.properties");
});

XPCOMUtils.defineLazyGetter(this, "domUtils", function() {
  return Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils);
});

loader.lazyGetter(this, "AutocompletePopup", () => require("devtools/shared/autocomplete-popup").AutocompletePopup);
