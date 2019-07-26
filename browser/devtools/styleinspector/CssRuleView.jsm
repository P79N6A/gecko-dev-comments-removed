





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";







const CSS_LINE_RE = /(?:[^;\(]*(?:\([^\)]*?\))?[^;\(]*)*;?/g;


const CSS_PROP_RE = /\s*([^:\s]*)\s*:\s*(.*?)\s*(?:! (important))?;?$/;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/InplaceEditor.jsm");

this.EXPORTED_SYMBOLS = ["CssRuleView",
                         "_ElementStyle"];
































function ElementStyle(aElement, aStore)
{
  this.element = aElement;
  this.store = aStore || {};

  
  
  if (!("userProperties" in this.store)) {
    this.store.userProperties = new UserProperties();
  }

  if (this.store.disabled) {
    this.store.disabled = aStore.disabled;
  } else {
    
    
    this.store.disabled = new Map();
  }

  let doc = aElement.ownerDocument;

  
  
  
  this.dummyElement = doc.createElementNS(this.element.namespaceURI,
                                          this.element.tagName);
  this.populate();
}

this._ElementStyle = ElementStyle;

ElementStyle.prototype = {

  
  element: null,

  
  
  dummyElement: null,

  



  _changed: function ElementStyle_changed()
  {
    if (this.onChanged) {
      this.onChanged();
    }
  },

  



  populate: function ElementStyle_populate()
  {
    
    
    this._refreshRules = this.rules;

    this.rules = [];

    let element = this.element;
    do {
      this._addElementRules(element);
    } while ((element = element.parentNode) &&
             element.nodeType === Ci.nsIDOMNode.ELEMENT_NODE);

    
    this.markOverridden();

    
    delete this._refreshRules;
  },

  _addElementRules: function ElementStyle_addElementRules(aElement)
  {
    let inherited = aElement !== this.element ? aElement : null;

    
    this._maybeAddRule({
      style: aElement.style,
      selectorText: CssLogic.l10n("rule.sourceElement"),
      inherited: inherited
    });

    
    var domRules = domUtils.getCSSStyleRules(aElement);

    
    
    for (let i = domRules.Count() - 1; i >= 0; i--) {
      let domRule = domRules.GetElementAt(i);

      
      let contentSheet = CssLogic.isContentStylesheet(domRule.parentStyleSheet);
      if (!contentSheet) {
        continue;
      }

      if (domRule.type !== Ci.nsIDOMCSSRule.STYLE_RULE) {
        continue;
      }

      this._maybeAddRule({
        domRule: domRule,
        inherited: inherited
      });
    }
  },

  








  _maybeAddRule: function ElementStyle_maybeAddRule(aOptions)
  {
    
    
    if (aOptions.domRule &&
        this.rules.some(function(rule) rule.domRule === aOptions.domRule)) {
      return false;
    }

    let rule = null;

    
    
    for (let r of (this._refreshRules || [])) {
      if (r.matches(aOptions)) {
        rule = r;
        rule.refresh();
        break;
      }
    }

    
    if (!rule) {
      rule = new Rule(this, aOptions);
    }

    
    if (aOptions.inherited && rule.textProps.length == 0) {
      return false;
    }

    this.rules.push(rule);
  },

  



  markOverridden: function ElementStyle_markOverridden()
  {
    
    
    let textProps = [];
    for each (let rule in this.rules) {
      textProps = textProps.concat(rule.textProps.slice(0).reverse());
    }

    
    
    let computedProps = [];
    for each (let textProp in textProps) {
      computedProps = computedProps.concat(textProp.computed);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let taken = {};
    for each (let computedProp in computedProps) {
      let earlier = taken[computedProp.name];
      let overridden;
      if (earlier
          && computedProp.priority === "important"
          && earlier.priority !== "important") {
        
        
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

    
    
    
    
    for each (let textProp in textProps) {
      
      
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
  this.domRule = aOptions.domRule || null;
  this.style = aOptions.style || this.domRule.style;
  this.selectorText = aOptions.selectorText || this.domRule.selectorText;
  this.inherited = aOptions.inherited || null;

  if (this.domRule) {
    let parentRule = this.domRule.parentRule;
    if (parentRule && parentRule.type == Ci.nsIDOMCSSRule.MEDIA_RULE) {
      this.mediaText = parentRule.media.mediaText;
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
    if (this.domRule) {
      this._title += ":" + this.ruleLine;
    }

    return this._title + (this.mediaText ? " @media " + this.mediaText : "");
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

  


  get sheet()
  {
    return this.domRule ? this.domRule.parentStyleSheet : null;
  },

  


  get ruleLine()
  {
    if (!this.sheet) {
      
      return null;
    }
    return domUtils.getRuleLine(this.domRule);
  },

  






  matches: function Rule_matches(aOptions)
  {
    return (this.style === (aOptions.style || aOptions.domRule.style));
  },

  









  createProperty: function Rule_createProperty(aName, aValue, aPriority)
  {
    let prop = new TextProperty(this, aName, aValue, aPriority);
    this.textProps.push(prop);
    this.applyProperties();
    return prop;
  },

  









  applyProperties: function Rule_applyProperties(aName)
  {
    let disabledProps = [];
    let store = this.elementStyle.store;

    for each (let prop in this.textProps) {
      if (!prop.enabled) {
        disabledProps.push({
          name: prop.name,
          value: prop.value,
          priority: prop.priority
        });
        continue;
      }

      this.style.setProperty(prop.name, prop.value, prop.priority);

      if (aName && prop.name == aName) {
        store.userProperties.setProperty(
          this.style, prop.name,
          this.style.getPropertyValue(prop.name),
          prop.value);
      }

      
      
      prop.priority = this.style.getPropertyPriority(prop.name);
      prop.updateComputed();
    }
    this.elementStyle._changed();

    
    let disabled = this.elementStyle.store.disabled;
    if (disabledProps.length > 0) {
      disabled.set(this.style, disabledProps);
    } else {
      disabled.delete(this.style);
    }

    this.elementStyle.markOverridden();
  },

  







  setPropertyName: function Rule_setPropertyName(aProperty, aName)
  {
    if (aName === aProperty.name) {
      return;
    }
    this.style.removeProperty(aProperty.name);
    aProperty.name = aName;
    this.applyProperties(aName);
  },

  









  setPropertyValue: function Rule_setPropertyValue(aProperty, aValue, aPriority)
  {
    if (aValue === aProperty.value && aPriority === aProperty.priority) {
      return;
    }
    aProperty.value = aValue;
    aProperty.priority = aPriority;
    this.applyProperties(aProperty.name);
  },

  


  setPropertyEnabled: function Rule_enableProperty(aProperty, aValue)
  {
    aProperty.enabled = !!aValue;
    if (!aProperty.enabled) {
      this.style.removeProperty(aProperty.name);
    }
    this.applyProperties();
  },

  



  removeProperty: function Rule_removeProperty(aProperty)
  {
    this.textProps = this.textProps.filter(function(prop) prop != aProperty);
    this.style.removeProperty(aProperty);
    
    
    this.applyProperties();
  },

  



  _getTextProperties: function Rule_getTextProperties()
  {
    let textProps = [];
    let store = this.elementStyle.store;
    let lines = this.style.cssText.match(CSS_LINE_RE);
    for each (let line in lines) {
      let matches = CSS_PROP_RE.exec(line);
      if (!matches || !matches[2])
        continue;

      let name = matches[1];
      if (this.inherited && !domUtils.isInheritedProperty(name)) {
        continue;
      }
      let value = store.userProperties.getProperty(this.style, name, matches[2]);
      let prop = new TextProperty(this, name, value, matches[3] || "");
      textProps.push(prop);
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

  



  refresh: function Rule_refresh()
  {
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
};














function TextProperty(aRule, aName, aValue, aPriority)
{
  this.rule = aRule;
  this.name = aName;
  this.value = aValue;
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
  }
};


































this.CssRuleView = function CssRuleView(aDoc, aStore)
{
  this.doc = aDoc;
  this.store = aStore;
  this.element = this.doc.createElementNS(HTML_NS, "div");
  this.element.className = "ruleview devtools-monospace";
  this.element.flex = 1;

  this._boundCopy = this._onCopy.bind(this);
  this.element.addEventListener("copy", this._boundCopy);

  this._showEmpty();
}

CssRuleView.prototype = {
  
  _viewedElement: null,

  


  get isEditing() {
    return this.element.querySelectorAll(".styleinspector-propertyeditor").length > 0;
  },

  destroy: function CssRuleView_destroy()
  {
    this.clear();

    this.element.removeEventListener("copy", this._boundCopy);
    delete this._boundCopy;

    if (this.element.parentNode) {
      this.element.parentNode.removeChild(this.element);
    }
  },

  





  highlight: function CssRuleView_highlight(aElement)
  {
    if (this._viewedElement === aElement) {
      return;
    }

    this.clear();

    if (this._elementStyle) {
      delete this._elementStyle;
    }

    this._viewedElement = aElement;
    if (!this._viewedElement) {
      this._showEmpty();
      return;
    }

    this._elementStyle = new ElementStyle(aElement, this.store);
    this._elementStyle.onChanged = function() {
      this._changed();
    }.bind(this);

    this._createEditors();
  },

  


  nodeChanged: function CssRuleView_nodeChanged()
  {
    
    if (this.isEditing) {
      return;
    }

    this._clearRules();

    
    this._elementStyle.populate();

    
    this._createEditors();

    
    var evt = this.doc.createEvent("Events");
    evt.initEvent("CssRuleViewRefreshed", true, false);
    this.element.dispatchEvent(evt);
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

  


  _createEditors: function CssRuleView_createEditors()
  {
    
    
    let lastInheritedSource = "";
    for each (let rule in this._elementStyle.rules) {

      let inheritedSource = rule.inheritedSource;
      if (inheritedSource != lastInheritedSource) {
        let h2 = this.doc.createElementNS(HTML_NS, "div");
        h2.className = "ruleview-rule-inheritance theme-gutter";
        h2.textContent = inheritedSource;
        lastInheritedSource = inheritedSource;
        this.element.appendChild(h2);
      }

      if (!rule.editor) {
        new RuleEditor(this, rule);
      }

      this.element.appendChild(rule.editor.element);
    }
  },

  





  _onCopy: function CssRuleView_onCopy(aEvent)
  {
    let target = aEvent.target;

    let text;

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

    aEvent.preventDefault();
  },

};










function RuleEditor(aRuleView, aRule)
{
  this.ruleView = aRuleView;
  this.doc = this.ruleView.doc;
  this.rule = aRule;
  this.rule.editor = this;

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

    
    
    this.element.style.position = "relative";

    
    let source = createChild(this.element, "div", {
      class: "ruleview-rule-source theme-link"
    });
    source.addEventListener("click", function() {
      let rule = this.rule;
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

      let editorNodes =
        this.doc.querySelectorAll(".styleinspector-propertyeditor");

      if (editorNodes) {
        for (let node of editorNodes) {
          if (node.inplaceEditor) {
            node.inplaceEditor._clear();
          }
        }
      }
    }.bind(this), false);

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

    
    
    
    if (this.rule.domRule && this.rule.domRule.selectorText) {
      let selectors = CssLogic.getSelectors(this.rule.domRule);
      let element = this.rule.inherited || this.ruleView._viewedElement;
      for (let i = 0; i < selectors.length; i++) {
        let selector = selectors[i];
        if (i != 0) {
          createChild(this.selectorText, "span", {
            class: "ruleview-selector-separator",
            textContent: ", "
          });
        }
        let cls;
        if (domUtils.selectorMatchesElement(element, this.rule.domRule, i)) {
          cls = "ruleview-selector-matched";
        } else {
          cls = "ruleview-selector-unmatched";
        }
        createChild(this.selectorText, "span", {
          class: cls,
          textContent: selector
        });
      }
    } else {
      this.selectorText.textContent = this.rule.selectorText;
    }

    for (let prop of this.rule.textProps) {
      if (!prop.editor) {
        new TextPropertyEditor(this, prop);
        this.propertyList.appendChild(prop.editor.element);
      }
    }
  },

  









  addProperty: function RuleEditor_addProperty(aName, aValue, aPriority)
  {
    let prop = this.rule.createProperty(aName, aValue, aPriority);
    let editor = new TextPropertyEditor(this, prop);
    this.propertyList.appendChild(editor.element);
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
      advanceChars: ":"
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
  this.doc = aRuleEditor.doc;
  this.prop = aProperty;
  this.prop.editor = this;

  this._onEnableClicked = this._onEnableClicked.bind(this);
  this._onExpandClicked = this._onExpandClicked.bind(this);
  this._onStartEditing = this._onStartEditing.bind(this);
  this._onNameDone = this._onNameDone.bind(this);
  this._onValueDone = this._onValueDone.bind(this);

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
    this.nameContainer.addEventListener("click", function(aEvent) {
      
      aEvent.stopPropagation();
      if (aEvent.target === propertyContainer) {
        this.nameSpan.click();
      }
    }.bind(this), false);

    
    
    this.nameSpan = createChild(this.nameContainer, "span", {
      class: "ruleview-propertyname theme-fg-color5",
      tabindex: "0",
    });

    editableField({
      start: this._onStartEditing,
      element: this.nameSpan,
      done: this._onNameDone,
      advanceChars: ':'
    });

    appendText(this.nameContainer, ": ");

    
    
    
    let propertyContainer = createChild(this.element, "span", {
      class: "ruleview-propertycontainer"
    });
    propertyContainer.addEventListener("click", function(aEvent) {
      
      aEvent.stopPropagation();
      if (aEvent.target === propertyContainer) {
        this.valueSpan.click();
      }
    }.bind(this), false);

    
    
    
    this.valueSpan = createChild(propertyContainer, "span", {
      class: "ruleview-propertyvalue theme-fg-color1",
      tabindex: "0",
    });

    
    
    this.committed = { name: this.prop.name,
                       value: this.prop.value,
                       priority: this.prop.priority };

    appendText(propertyContainer, ";");

    this.warning = createChild(this.element, "div", {
      hidden: "",
      class: "ruleview-warning",
      title: CssLogic.l10n("rule.warning.title"),
    });

    
    
    this.computed = createChild(this.element, "ul", {
      class: "ruleview-computedlist",
    });

    editableField({
      start: this._onStartEditing,
      element: this.valueSpan,
      done: this._onValueDone,
      validate: this._validate.bind(this),
      warning: this.warning,
      advanceChars: ';'
    });
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

    if (this.prop.overridden && !this.editing) {
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
    this.valueSpan.textContent = val;
    this.warning.hidden = this._validate();

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
    if (!aCommit) {
      if (this.prop.overridden) {
        this.element.classList.add("ruleview-overridden");
      }

      return;
    }
    if (!aValue) {
      this.prop.remove();
      this.element.parentNode.removeChild(this.element);
      return;
    }
    this.prop.setName(aValue);
  },

  







  _parseValue: function TextPropertyEditor_parseValue(aValue)
  {
    let pieces = aValue.split("!", 2);
    return {
      value: pieces[0].trim(),
      priority: (pieces.length > 1 ? pieces[1].trim() : "")
    };
  },

  








   _onValueDone: function PropertyEditor_onValueDone(aValue, aCommit)
  {
    if (aCommit) {
      let val = this._parseValue(aValue);
      this.prop.setValue(val.value, val.priority);
      this.committed.value = this.prop.value;
      this.committed.priority = this.prop.priority;
      if (this.prop.overridden) {
        this.element.classList.add("ruleview-overridden");
      }
    } else {
      this.prop.setValue(this.committed.value, this.committed.priority);
    }
  },

  








  _validate: function TextPropertyEditor_validate(aValue)
  {
    let name = this.prop.name;
    let value = typeof aValue == "undefined" ? this.prop.value : aValue;
    let val = this._parseValue(value);
    let style = this.doc.createElementNS(HTML_NS, "div").style;
    let prefs = Services.prefs;

    
    let prefVal = Services.prefs.getBoolPref("layout.css.report_errors");
    prefs.setBoolPref("layout.css.report_errors", false);

    try {
      style.setProperty(name, val.value, val.priority);
    } finally {
      prefs.setBoolPref("layout.css.report_errors", prefVal);
    }
    return !!style.getPropertyValue(name);
  },
};





function UserProperties()
{
  
  
  this.map = new Map();
}

UserProperties.prototype = {
  














  getProperty: function UP_getProperty(aStyle, aName, aComputedValue) {
    let entry = this.map.get(aStyle, null);

    if (entry && aName in entry) {
      let item = entry[aName];
      if (item.computed != aComputedValue) {
        delete entry[aName];
        return aComputedValue;
      }

      return item.user;
    }
    return aComputedValue;

  },

  












  setProperty: function UP_setProperty(aStyle, aName, aComputedValue, aUserValue) {
    let entry = this.map.get(aStyle, null);
    if (entry) {
      entry[aName] = { computed: aComputedValue, user: aUserValue };
    } else {
      let props = {};
      props[aName] = { computed: aComputedValue, user: aUserValue };
      this.map.set(aStyle, props);
    }
  },

  







  contains: function UP_contains(aStyle, aName) {
    let entry = this.map.get(aStyle, null);
    return !!entry && aName in entry;
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
