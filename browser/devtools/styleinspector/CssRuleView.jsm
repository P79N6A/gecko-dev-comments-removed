






































"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const FOCUS_FORWARD = Ci.nsIFocusManager.MOVEFOCUS_FORWARD;
const FOCUS_BACKWARD = Ci.nsIFocusManager.MOVEFOCUS_BACKWARD;







const CSS_LINE_RE = /(?:[^;\(]*(?:\([^\)]*?\))?[^;\(]*)*;?/g;


const CSS_PROP_RE = /\s*([^:\s]*)\s*:\s*(.*?)\s*(?:! (important))?;?$/;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/devtools/CssLogic.jsm");

var EXPORTED_SYMBOLS = ["CssRuleView",
                        "_ElementStyle",
                        "_editableField"];
































function ElementStyle(aElement, aStore)
{
  this.element = aElement;
  this.store = aStore || {};
  if (this.store.disabled) {
    this.store.disabled = aStore.disabled;
  } else {
    this.store.disabled = WeakMap();
  }

  let doc = aElement.ownerDocument;

  
  
  
  this.dummyElement = doc.createElementNS(this.element.namespaceURI,
                                          this.element.tagName);
  this._populate();
}

var _ElementStyle = ElementStyle;

ElementStyle.prototype = {

  
  element: null,

  
  
  dummyElement: null,

  domUtils: Cc["@mozilla.org/inspector/dom-utils;1"].getService(Ci.inIDOMUtils),

  



  _changed: function ElementStyle_changed()
  {
    if (this.onChanged) {
      this.onChanged();
    }
  },

  



  _populate: function ElementStyle_populate()
  {
    this.rules = [];

    let element = this.element;
    do {
      this._addElementRules(element);
    } while ((element = element.parentNode) &&
             element.nodeType === Ci.nsIDOMNode.ELEMENT_NODE);

    
    this.markOverridden();
  },

  _addElementRules: function ElementStyle_addElementRules(aElement)
  {
    let inherited = aElement !== this.element ? aElement : null;

    
    this._maybeAddRule({
      style: aElement.style,
      selectorText: CssLogic.l10n("rule.sourceElement"),
      inherited: inherited
    });

    
    var domRules = this.domUtils.getCSSStyleRules(aElement);

    
    
    for (let i = domRules.Count() - 1; i >= 0; i--) {
      let domRule = domRules.GetElementAt(i);

      
      let systemSheet = CssLogic.isSystemStyleSheet(domRule.parentStyleSheet);
      if (systemSheet) {
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

    let rule = new Rule(this, aOptions);

    
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
}

















function Rule(aElementStyle, aOptions)
{
  this.elementStyle = aElementStyle;
  this.domRule = aOptions.domRule || null;
  this.style = aOptions.style || this.domRule.style;
  this.selectorText = aOptions.selectorText || this.domRule.selectorText;
  this.inherited = aOptions.inherited || null;
  this._getTextProperties();
}

Rule.prototype = {
  get title()
  {
    if (this._title) {
      return this._title;
    }
    let sheet = this.domRule ? this.domRule.parentStyleSheet : null;
    this._title = CssLogic.shortSource(sheet);
    if (this.domRule) {
      let line = this.elementStyle.domUtils.getRuleLine(this.domRule);
      this._title += ":" + line;
    }

    if (this.inherited) {
      let eltText = this.inherited.tagName.toLowerCase();
      if (this.inherited.id) {
        eltText += "#" + this.inherited.id;
      }
      let args = [eltText, this._title];
      this._title = CssLogic._strings.formatStringFromName("rule.inheritedSource",
                                                           args, args.length);
    }

    return this._title;
  },

  









  createProperty: function Rule_createProperty(aName, aValue, aPriority)
  {
    let prop = new TextProperty(this, aName, aValue, aPriority);
    this.textProps.push(prop);
    this.applyProperties();
    return prop;
  },

  




  applyProperties: function Rule_applyProperties()
  {
    let disabledProps = [];

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
      
      
      prop.value = this.style.getPropertyValue(prop.name);
      prop.priority = this.style.getPropertyPriority(prop.name);
      prop.updateComputed();
    }
    this.elementStyle._changed();

    
    let disabled = this.elementStyle.store.disabled;
    disabled.set(this.style, disabledProps);

    this.elementStyle.markOverridden();
  },

  







  setPropertyName: function Rule_setPropertyName(aProperty, aName)
  {
    if (aName === aProperty.name) {
      return;
    }
    this.style.removeProperty(aProperty.name);
    aProperty.name = aName;
    this.applyProperties();
  },

  









  setPropertyValue: function Rule_setPropertyValue(aProperty, aValue, aPriority)
  {
    if (aValue === aProperty.value && aPriority === aProperty.priority) {
      return;
    }
    aProperty.value = aValue;
    aProperty.priority = aPriority;
    this.applyProperties();
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
    this.textProps = [];
    let lines = this.style.cssText.match(CSS_LINE_RE);
    for each (let line in lines) {
      let matches = CSS_PROP_RE.exec(line);
      if(!matches || !matches[2])
        continue;

      let name = matches[1];
      if (this.inherited &&
          !this.elementStyle.domUtils.isInheritedProperty(name)) {
        continue;
      }

      let prop = new TextProperty(this, name, matches[2], matches[3] || "");
      this.textProps.push(prop);
    }

    
    let disabledProps = this.elementStyle.store.disabled.get(this.style);
    if (!disabledProps) {
      return;
    }

    for each (let prop in disabledProps) {
      let textProp = new TextProperty(this, prop.name,
                                      prop.value, prop.priority);
      textProp.enabled = false;
      this.textProps.push(textProp);
    }
  },
}














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
}
































function CssRuleView(aDoc, aStore)
{
  this.doc = aDoc;
  this.store = aStore;

  this.element = this.doc.createElementNS(XUL_NS, "vbox");
  this.element.setAttribute("tabindex", "0");
  this.element.classList.add("ruleview");
  this.element.flex = 1;

  
  
  this.element.style.position = "relative";
}

CssRuleView.prototype = {
  
  _viewedElement: null,

  





  highlight: function CssRuleView_highlight(aElement)
  {
    if (this._viewedElement === aElement) {
      return;
    }

    this.clear();

    this._viewedElement = aElement;
    if (!this._viewedElement) {
      return;
    }

    if (this._elementStyle) {
      delete this._elementStyle.onChanged;
    }

    this._elementStyle = new ElementStyle(aElement, this.store);
    this._elementStyle.onChanged = function() {
      this._changed();
    }.bind(this);

    this._createEditors();
  },

  


  clear: function CssRuleView_clear()
  {
    while (this.element.hasChildNodes()) {
      this.element.removeChild(this.element.lastChild);
    }
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
    for each (let rule in this._elementStyle.rules) {
      
      
      let editor = new RuleEditor(this.doc, rule);
      this.element.appendChild(editor.element);
    }
  },
};










function RuleEditor(aDoc, aRule)
{
  this.doc = aDoc;
  this.rule = aRule;

  this._onNewProperty = this._onNewProperty.bind(this);

  this._create();
}

RuleEditor.prototype = {
  _create: function RuleEditor_create()
  {
    this.element = this.doc.createElementNS(HTML_NS, "div");
    this.element._ruleEditor = this;

    
    let source = createChild(this.element, "div", {
      class: "ruleview-rule-source",
      textContent: this.rule.title
    });

    let code = createChild(this.element, "div", {
      class: "ruleview-code"
    });

    let header = createChild(code, "div", {});

    let selectors = createChild(header, "span", {
      class: "ruleview-selector",
      textContent: this.rule.selectorText
    });
    appendText(header, " {");

    this.propertyList = createChild(code, "ul", {
      class: "ruleview-propertylist"
    });

    for each (let prop in this.rule.textProps) {
      let propEditor = new TextPropertyEditor(this, prop);
      this.propertyList.appendChild(propEditor.element);
    }

    this.closeBrace = createChild(code, "div", {
      class: "ruleview-ruleclose",
      tabindex: "0",
      textContent: "}"
    });

    
    
    this.closeBrace.addEventListener("focus", function() {
      this.newProperty();
    }.bind(this), true);
  },

  




  newProperty: function RuleEditor_newProperty()
  {
    
    
    
    this.closeBrace.removeAttribute("tabindex");

    this.newPropItem = createChild(this.propertyList, "li", {
      class: "ruleview-property ruleview-newproperty",
    });

    this.newPropSpan = createChild(this.newPropItem, "span", {
      class: "ruleview-propertyname"
    });

    new InplaceEditor({
      element: this.newPropSpan,
      done: this._onNewProperty,
      advanceChars: ":"
    });
  },

  _onNewProperty: function RuleEditor_onNewProperty(aValue, aCommit)
  {
    
    this.closeBrace.setAttribute("tabindex", "0");

    this.propertyList.removeChild(this.newPropItem);
    delete this.newPropItem;
    delete this.newPropSpan;

    if (!aValue || !aCommit) {
      return;
    }

    
    let prop = this.rule.createProperty(aValue, "", "");
    let editor = new TextPropertyEditor(this, prop);
    this.propertyList.appendChild(editor.element);
    editor.valueSpan.focus();
  },
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

    
    this.enable = createChild(this.element, "input", {
      class: "ruleview-enableproperty",
      type: "checkbox",
      tabindex: "-1"
    });
    this.enable.addEventListener("click", this._onEnableClicked, true);

    
    this.expander = createChild(this.element, "span", {
      class: "ruleview-expander"
    });
    this.expander.addEventListener("click", this._onExpandClicked, true);

    
    
    this.nameSpan = createChild(this.element, "span", {
      class: "ruleview-propertyname",
      tabindex: "0",
    });
    editableField({
      start: this._onStartEditing,
      element: this.nameSpan,
      done: this._onNameDone,
      advanceChars: ':'
    });

    appendText(this.element, ": ");

    
    
    
    this.valueSpan = createChild(this.element, "span", {
      class: "ruleview-propertyvalue",
      tabindex: "0",
    });

    editableField({
      start: this._onStartEditing,
      element: this.valueSpan,
      done: this._onValueDone,
      advanceChars: ';'
    });

    
    
    this.committed = { name: this.prop.name,
                       value: this.prop.value,
                       priority: this.prop.priority };

    appendText(this.element, ";");

    
    
    this.computed = createChild(this.element, "ul", {
      class: "ruleview-computedlist",
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

    this.nameSpan.textContent = this.prop.name;

    
    
    let val = this.prop.value;
    if (this.prop.priority) {
      val += " !" + this.prop.priority;
    }
    this.valueSpan.textContent = val;

    
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
        class: "ruleview-propertyname",
        textContent: computed.name
      });
      appendText(li, ": ");
      createChild(li, "span", {
        class: "ruleview-propertyvalue",
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

  


  _onEnableClicked: function TextPropertyEditor_onEnableClicked()
  {
    this.prop.setEnabled(this.enable.checked);
  },

  


  _onExpandClicked: function TextPropertyEditor_onExpandClicked()
  {
    this.expander.classList.toggle("styleinspector-open");
    this.computed.classList.toggle("styleinspector-open");
  },

  









  _onNameDone: function TextPropertyEditor_onNameDone(aValue, aCommit)
  {
    if (!aCommit) {
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
    let value = pieces[0];
    let priority = pieces.length > 1 ? pieces[1] : "";
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
    } else {
      this.prop.setValue(this.committed.value, this.committed.priority);
    }
  },
};
























function editableField(aOptions)
{
  aOptions.element.addEventListener("focus", function() {
    new InplaceEditor(aOptions);
  }, false);
}
var _editableField = editableField;

function InplaceEditor(aOptions)
{
  this.elt = aOptions.element;
  this.elt.inplaceEditor = this;

  this.change = aOptions.change;
  this.done = aOptions.done;
  this.initial = aOptions.initial ? aOptions.initial : this.elt.textContent;
  this.doc = this.elt.ownerDocument;

  this._onBlur = this._onBlur.bind(this);
  this._onKeyPress = this._onKeyPress.bind(this);
  this._onInput = this._onInput.bind(this);

  this._createInput();
  this._autosize();

  
  
  this._advanceCharCodes = {};
  let advanceChars = aOptions.advanceChars || '';
  for (let i = 0; i < advanceChars.length; i++) {
    this._advanceCharCodes[advanceChars.charCodeAt(i)] = true;
  }

  
  this.originalDisplay = this.elt.style.display;
  this.elt.style.display = "none";
  this.elt.parentNode.insertBefore(this.input, this.elt);

  this.input.select();
  this.input.focus();

  this.input.addEventListener("blur", this._onBlur, false);
  this.input.addEventListener("keypress", this._onKeyPress, false);
  this.input.addEventListener("input", this._onInput, false);

  if (aOptions.start) {
    aOptions.start();
  }
}

InplaceEditor.prototype = {
  _createInput: function InplaceEditor_createEditor()
  {
    this.input = this.doc.createElementNS(HTML_NS, "input");
    this.input.inplaceEditor = this;
    this.input.classList.add("styleinspector-propertyeditor");
    this.input.value = this.initial;

    copyTextStyles(this.elt, this.input);
  },

  


  _clear: function InplaceEditor_clear()
  {
    this.input.removeEventListener("blur", this._onBlur, false);
    this.input.removeEventListener("keypress", this._onKeyPress, false);
    this.input.removeEventListener("oninput", this._onInput, false);
    this._stopAutosize();

    this.elt.parentNode.removeChild(this.input);
    this.elt.style.display = this.originalDisplay;
    this.input = null;

    delete this.elt.inplaceEditor;
    delete this.elt;
  },

  



  _autosize: function InplaceEditor_autosize()
  {
    
    

    
    
    
    
    this._measurement = this.doc.createElementNS(HTML_NS, "span");
    this.elt.parentNode.appendChild(this._measurement);
    let style = this._measurement.style;
    style.visibility = "hidden";
    style.position = "absolute";
    style.top = "0";
    style.left = "0";
    copyTextStyles(this.input, this._measurement);
    this._updateSize();
  },

  


  _stopAutosize: function InplaceEditor_stopAutosize()
  {
    if (!this._measurement) {
      return;
    }
    this._measurement.parentNode.removeChild(this._measurement);
    delete this._measurement;
  },

  


  _updateSize: function InplaceEditor_updateSize()
  {
    
    
    
    this._measurement.textContent = this.input.value.replace(' ', '\u00a0', 'g');

    
    
    
    let width = this._measurement.offsetWidth + 10;

    this.input.style.width = width + "px";
  },

  



  _onBlur: function InplaceEditor_onBlur(aEvent)
  {
    if (this.done) {
      this.done(this.cancelled ? this.initial : this.input.value.trim(),
                !this.cancelled);
    }
    this._clear();
  },

  _onKeyPress: function InplaceEditor_onKeyPress(aEvent)
  {
    let prevent = false;
    if (aEvent.charCode in this._advanceCharCodes
       || aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_RETURN) {
      
      
      
      prevent = true;
      moveFocus(this.input.ownerDocument.defaultView, FOCUS_FORWARD);
    } else if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_ESCAPE) {
      
      
      prevent = true;
      this.cancelled = true;
      this.input.blur();
    } else if (aEvent.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_SPACE) {
      
      
      
      prevent = !this.input.value;
    }

    if (prevent) {
      aEvent.preventDefault();
    }
  },

  


  _onInput: function InplaceEditor_onInput(aEvent)
  {
    
    if (this._measurement) {
      this._updateSize();
    }

    
    if (this.change) {
      this.change(this.input.value.trim());
    }
  }
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




function appendText(aParent, aText)
{
  aParent.appendChild(aParent.ownerDocument.createTextNode(aText));
}




function copyTextStyles(aFrom, aTo)
{
  let win = aFrom.ownerDocument.defaultView;
  let style = win.getComputedStyle(aFrom);
  aTo.style.fontFamily = style.getPropertyCSSValue("font-family").cssText;
  aTo.style.fontSize = style.getPropertyCSSValue("font-size").cssText;
  aTo.style.fontWeight = style.getPropertyCSSValue("font-weight").cssText;
  aTo.style.fontStyle = style.getPropertyCSSValue("font-style").cssText;
}




function moveFocus(aWin, aDirection)
{
  let fm = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);
  fm.moveFocus(aWin, null, aDirection, 0);
}
