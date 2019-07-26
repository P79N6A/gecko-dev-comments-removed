




"use strict";

Components.utils.import('resource://gre/modules/Services.jsm');

let EXPORTED_SYMBOLS = ["VariablesView", "create"];












function VariablesView(aParentNode) {
  this._store = new Map();
  this._prevHierarchy = new Map();
  this._currHierarchy = new Map();
  this._parent = aParentNode;
  this._appendEmptyNotice();

  
  this._list = this.document.createElement("vbox");
  this._parent.appendChild(this._list);
}

VariablesView.prototype = {
  







  addScope: function VV_addScope(aName) {
    this._removeEmptyNotice();

    let scope = new Scope(this, aName);
    this._store.set(scope.id, scope);
    return scope;
  },

  


  empty: function VV_empty() {
    let list = this._list;
    let firstChild;

    while (firstChild = list.firstChild) {
      list.removeChild(firstChild);
    }

    this._store = new Map();
    this._appendEmptyNotice();
  },

  



  set enumVisible(aFlag) {
    this._enumVisible = aFlag;

    for (let [_, scope] in this) {
      scope._nonEnumVisible = aFlag;
    }
  },

  



  set nonEnumVisible(aFlag) {
    this._nonEnumVisible = aFlag;

    for (let [_, scope] in this) {
      scope._nonEnumVisible = aFlag;
    }
  },

  



  set emptyText(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
  },

  


  _appendEmptyNotice: function VV__appendEmptyNotice() {
    if (this._emptyTextNode) {
      return;
    }

    let label = this.document.createElement("label");
    label.className = "empty list-item";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.appendChild(label);
    this._emptyTextNode = label;
  },

  


  _removeEmptyNotice: function VV__removeEmptyNotice() {
    if (!this._emptyTextNode) {
      return;
    }

    this._parent.removeChild(this._emptyTextNode);
    this._emptyTextNode = null;
  },

  



  get parentNode() this._parent,

  



  get document() this._parent.ownerDocument,

  



  get window() this.document.defaultView,

  eval: null,
  _store: null,
  _prevHierarchy: null,
  _currHierarchy: null,
  _enumVisible: true,
  _nonEnumVisible: true,
  _list: null,
  _parent: null,
  _emptyTextNode: null,
  _emptyTextValue: ""
};












function Scope(aView, aName, aFlags = {}) {
  this.show = this.show.bind(this);
  this.hide = this.hide.bind(this);
  this.expand = this.expand.bind(this);
  this.collapse = this.collapse.bind(this);
  this.toggle = this.toggle.bind(this);

  this.ownerView = aView;
  this.eval = aView.eval;

  this._store = new Map();
  this._init(aName.trim(), aFlags);
}

Scope.prototype = {
  



















  addVar: function S_addVar(aName, aDescriptor = {}) {
    if (this._store.has(aName)) {
      return null;
    }

    let variable = new Variable(this, aName, aDescriptor);
    this._store.set(aName, variable);
    this._variablesView._currHierarchy.set(variable._absoluteName, variable);
    return variable;
  },

  





  get: function S_get(aName) {
    return this._store.get(aName);
  },

  


  show: function S_show() {
    this._target.hidden = false;
    this._isShown = true;

    if (this.onshow) {
      this.onshow(this);
    }
  },

  


  hide: function S_hide() {
    this._target.hidden = true;
    this._isShown = false;

    if (this.onhide) {
      this.onhide(this);
    }
  },

  





  expand: function S_expand(aSkipAnimationFlag) {
    if (this._locked) {
      return;
    }
    if (this._variablesView._enumVisible) {
      this._arrow.setAttribute("open", "");
      this._enum.setAttribute("open", "");
    }
    if (this._variablesView._nonEnumVisible) {
      this._nonenum.setAttribute("open", "");
    }
    if (!aSkipAnimationFlag) {
      this._enum.setAttribute("animated", "");
      this._nonenum.setAttribute("animated", "");
    }
    this._isExpanded = true;

    if (this.onexpand) {
      this.onexpand(this);
    }
  },

  


  collapse: function S_collapse() {
    if (this._locked) {
      return;
    }
    this._arrow.removeAttribute("open");
    this._enum.removeAttribute("open");
    this._nonenum.removeAttribute("open");
    this._enum.removeAttribute("animated");
    this._nonenum.removeAttribute("animated");
    this._isExpanded = false;

    if (this.oncollapse) {
      this.oncollapse(this);
    }
  },

  


  toggle: function S_toggle() {
    this.expanded ^= 1;

    if (this.ontoggle) {
      this.ontoggle(this);
    }
  },

  


  showArrow: function S_showArrow() {
    this._arrow.removeAttribute("invisible");
    this._isArrowVisible = true;
  },

  


  hideArrow: function S_hideArrow() {
    this._arrow.setAttribute("invisible", "");
    this._isArrowVisible = false;
  },

  



  get visible() this._isShown,

  



  get expanded() this._isExpanded,

  



  get twisty() this._isArrowVisible,

  



  set visible(aFlag) aFlag ? this.show() : this.hide(),

  



  set expanded(aFlag) aFlag ? this.expand() : this.collapse(),

  



  set twisty(aFlag) aFlag ? this.showArrow() : this.hideArrow(),

  



  get id() this._idString,

  



  get name() this._nameString,

  



  get target() this._target,

  









  _init: function S__init(aName, aFlags = {}, aClassName = "scope") {
    this._idString = generateId(this._nameString = aName);
    this._createScope(aName, aClassName);
    this._addEventListeners();
    this.parentNode.appendChild(this._target);
  },

  







  _createScope: function S__createScope(aName, aClassName) {
    let document = this.document;

    let element = this._target = document.createElement("vbox");
    element.id = this._id;
    element.className = aClassName;

    let arrow = this._arrow = document.createElement("hbox");
    arrow.className = "arrow";

    let name = this._name = document.createElement("label");
    name.className = "name plain";
    name.setAttribute("value", aName);

    let title = this._title = document.createElement("hbox");
    title.className = "title" + (aClassName == "scope" ? " devtools-toolbar" : "");
    title.setAttribute("align", "center");

    let enumerable = this._enum = document.createElement("vbox");
    let nonenum = this._nonenum = document.createElement("vbox");
    enumerable.className = "details";
    nonenum.className = "details nonenum";

    title.appendChild(arrow);
    title.appendChild(name);

    element.appendChild(title);
    element.appendChild(enumerable);
    element.appendChild(nonenum);
  },

  


  _addEventListeners: function S__addEventListeners() {
    this._title.addEventListener("mousedown", this.toggle, false);
  },

  



  set _enumVisible(aFlag) {
    for (let [_, variable] in this) {
      variable._enumVisible = aFlag;

      if (!this.expanded) {
        continue;
      }
      if (aFlag) {
        this._enum.setAttribute("open", "");
      } else {
        this._enum.removeAttribute("open");
      }
    }
  },

  



  set _nonEnumVisible(aFlag) {
    for (let [_, variable] in this) {
      variable._nonEnumVisible = aFlag;

      if (!this.expanded) {
        continue;
      }
      if (aFlag) {
        this._nonenum.setAttribute("open", "");
      } else {
        this._nonenum.removeAttribute("open");
      }
    }
  },

  



  get _variablesView() {
    let parentView = this.ownerView;
    let topView;

    while (topView = parentView.ownerView) {
      parentView = topView;
    }
    return parentView;
  },

  



  get parentNode() this.ownerView._list,

  



  get document() this.ownerView.document,

  



  get window() this.ownerView.window,

  ownerView: null,
  eval: null,
  _committed: false,
  _locked: false,
  _isShown: true,
  _isExpanded: false,
  _isArrowVisible: true,
  _store: null,
  _idString: null,
  _nameString: null,
  _target: null,
  _arrow: null,
  _name: null,
  _title: null,
  _enum: null,
  _nonenum: null
};












function Variable(aScope, aName, aDescriptor) {
  this._activateInput = this._activateInput.bind(this);
  this._deactivateInput = this._deactivateInput.bind(this);
  this._saveInput = this._saveInput.bind(this);
  this._onInputKeyPress = this._onInputKeyPress.bind(this);

  Scope.call(this, aScope, aName, aDescriptor);
  this._setGrip(aDescriptor.value);
  this._symbolicName = aName;
  this._absoluteName = aScope.name + "." + aName;
  this._initialDescriptor = aDescriptor;
}

create({ constructor: Variable, proto: Scope.prototype }, {
  



















  addProperty: function V_addProperty(aName, aDescriptor = {}) {
    if (this._store.has(aName)) {
      return null;
    }

    let property = new Property(this, aName, aDescriptor);
    this._store.set(aName, property);
    this._variablesView._currHierarchy.set(property._absoluteName, property);
    return property;
  },

  
















  addProperties: function V_addProperties(aProperties) {
    
    let sortedPropertyNames = Object.keys(aProperties).sort();

    for (let name of sortedPropertyNames) {
      this.addProperty(name, aProperties[name]);
    }
  },

  














  _setGrip: function V__setGrip(aGrip) {
    if (aGrip === undefined) {
      aGrip = { type: "undefined" };
    }
    if (aGrip === null) {
      aGrip = { type: "null" };
    }
    this._applyGrip(aGrip);
  },

  






  _applyGrip: function V__applyGrip(aGrip) {
    let prevGrip = this._valueGrip;
    if (prevGrip) {
      this._valueLabel.classList.remove(VariablesView.getClass(prevGrip));
    }
    this._valueGrip = aGrip;
    this._valueString = VariablesView.getString(aGrip);
    this._valueClassName = VariablesView.getClass(aGrip);

    this._valueLabel.classList.add(this._valueClassName);
    this._valueLabel.setAttribute("value", this._valueString);
  },

  







  _init: function V__init(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._createScope(aName, "variable");
    this._displayVariable(aDescriptor);
    this._displayTooltip();
    this._setAttributes(aName, aDescriptor);
    this._addEventListeners();

    if (aDescriptor.enumerable || aName == "this" || aName == "<exception>") {
      this.ownerView._enum.appendChild(this._target);
    } else {
      this.ownerView._nonenum.appendChild(this._target);
    }
  },

  





  _displayVariable: function V__displayVariable(aDescriptor) {
    let document = this.document;

    let separatorLabel = this._separatorLabel = document.createElement("label");
    separatorLabel.className = "plain";
    separatorLabel.setAttribute("value", ":");

    let valueLabel = this._valueLabel = document.createElement("label");
    valueLabel.className = "value plain";

    this._title.appendChild(separatorLabel);
    this._title.appendChild(valueLabel);

    if (VariablesView.isPrimitive(aDescriptor)) {
      this.hideArrow();
    }
    if (aDescriptor.get || aDescriptor.set) {
      this.addProperty("get ", { value: aDescriptor.get });
      this.addProperty("set ", { value: aDescriptor.set });
      this.expand();
      separatorLabel.hidden = true;
      valueLabel.hidden = true;
    }
  },

  


  _displayTooltip: function V__displayTooltip() {
    let document = this.document;

    let tooltip = document.createElement("tooltip");
    tooltip.id = "tooltip-" + this.id;

    let configurableLabel = document.createElement("label");
    configurableLabel.setAttribute("value", "configurable");

    let enumerableLabel = document.createElement("label");
    enumerableLabel.setAttribute("value", "enumerable");

    let writableLabel = document.createElement("label");
    writableLabel.setAttribute("value", "writable");

    tooltip.setAttribute("orient", "horizontal")
    tooltip.appendChild(configurableLabel);
    tooltip.appendChild(enumerableLabel);
    tooltip.appendChild(writableLabel);

    this._target.appendChild(tooltip);
    this._target.setAttribute("tooltip", tooltip.id);
  },

  








  _setAttributes: function V__setAttributes(aName, aDescriptor) {
    if (aDescriptor) {
      if (!aDescriptor.configurable) {
        this._target.setAttribute("non-configurable", "");
      }
      if (!aDescriptor.enumerable) {
        this._target.setAttribute("non-enumerable", "");
      }
      if (!aDescriptor.writable) {
        this._target.setAttribute("non-writable", "");
      }
    }
    if (aName == "this") {
      this._target.setAttribute("self", "");
    }
    if (aName == "<exception>") {
      this._target.setAttribute("exception", "");
    }
    if (aName == "__proto__") {
      this._target.setAttribute("proto", "");
    }
  },

  


  _addEventListeners: function V__addEventListeners() {
    this._arrow.addEventListener("mousedown", this.toggle, false);
    this._name.addEventListener("mousedown", this.toggle, false);
    this._valueLabel.addEventListener("click", this._activateInput, false);
  },

  


  _activateInput: function V__activateInput(e) {
    if (!this.eval) {
      return;
    }

    let title = this._title;
    let valueLabel = this._valueLabel;
    let initialString = this._valueLabel.getAttribute("value");

    
    
    let input = this.document.createElement("textbox");
    input.setAttribute("value", initialString);
    input.className = "element-input";
    input.width = valueLabel.clientWidth + 1;

    title.removeChild(valueLabel);
    title.appendChild(input);
    input.select();

    
    
    
    if (valueLabel.getAttribute("value").match(/^"[^"]*"$/)) {
      input.selectionEnd--;
      input.selectionStart++;
    }

    input.addEventListener("keypress", this._onInputKeyPress, false);
    input.addEventListener("blur", this._deactivateInput, false);

    this._prevExpandable = this.twisty;
    this._prevExpanded = this.expanded;
    this.collapse();
    this.hideArrow();
    this._locked = true;
  },

  


  _deactivateInput: function V__deactivateInput(e) {
    let input = e.target;
    let title = this._title;
    let valueLabel = this._valueLabel;

    title.removeChild(input);
    title.appendChild(valueLabel);

    input.removeEventListener("keypress", this._onInputKeyPress, false);
    input.removeEventListener("blur", this._deactivateInput, false);

    this._locked = false;
    this.twisty = this._prevExpandable;
    this.expanded = this._prevExpanded;
  },

  


  _saveInput: function V__saveInput(e) {
    let input = e.target;
    let valueLabel = this._valueLabel;
    let initialString = this._valueLabel.getAttribute("value");
    let currentString = input.value;

    this._deactivateInput(e);

    if (initialString != currentString) {
      this._separatorLabel.hidden = true;
      this._valueLabel.hidden = true;
      this.collapse();
      this.eval("(" + this._symbolicName + "=" + currentString + ")");
    }
  },

  


  _onInputKeyPress: function V__onInputKeyPress(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveInput(e);
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateInput(e);
        return;
    }
  },

  _symbolicName: "",
  _absoluteName: "",
  _initialDescriptor: null,
  _separatorLabel: null,
  _valueLabel: null,
  _tooltip: null,
  _valueGrip: null,
  _valueString: "",
  _valueClassName: "",
  _prevExpandable: false,
  _prevExpanded: false
});












function Property(aVar, aName, aDescriptor) {
  Variable.call(this, aVar, aName, aDescriptor);
  this._symbolicName = aVar._symbolicName + "[\"" + aName + "\"]";
  this._absoluteName = aVar._absoluteName + "." + aName;
  this._initialDescriptor = aDescriptor;
}

create({ constructor: Property, proto: Variable.prototype }, {
  







  _init: function P__init(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._createScope(aName, "property");
    this._displayVariable(aDescriptor);
    this._displayTooltip();
    this._setAttributes(aName, aDescriptor);
    this._addEventListeners();

    if (aDescriptor.enumerable) {
      this.ownerView._enum.appendChild(this._target);
    } else {
      this.ownerView._nonenum.appendChild(this._target);
    }
  }
});




VariablesView.prototype.__iterator__ =
Scope.prototype.__iterator__ =
Variable.prototype.__iterator__ =
Property.prototype.__iterator__ = function VV_iterator() {
  for (let item of this._store) {
    yield item;
  }
};





VariablesView.prototype.createHierarchy = function VV_createHierarchy() {
  this._prevHierarchy = this._currHierarchy;
  this._currHierarchy = new Map();
};





VariablesView.prototype.commitHierarchy = function VV_commitHierarchy() {
  let prevHierarchy = this._prevHierarchy;
  let currHierarchy = this._currHierarchy;

  for (let [absoluteName, currVariable] of currHierarchy) {
    
    if (currVariable._committed) {
      continue;
    }

    
    
    let prevVariable = prevHierarchy.get(absoluteName);
    let changed = false;

    
    
    if (prevVariable) {
      let prevString = prevVariable._valueString;
      let currString = currVariable._valueString;
      changed = prevString != currString;
    }

    
    
    currVariable._committed = true;

    
    if (!changed) {
      continue;
    }

    
    
    
    Services.tm.currentThread.dispatch({ run: function(aTarget) {
      aTarget.setAttribute("changed", "");

      aTarget.addEventListener("transitionend", function onEvent() {
        aTarget.removeEventListener("transitionend", onEvent, false);
        aTarget.removeAttribute("changed");
      }, false);
    }.bind(this, currVariable.target)}, 0);
  }
};








VariablesView.isPrimitive = function VV_isPrimitive(aDescriptor) {
  if (!aDescriptor || typeof aDescriptor != "object") {
    return true;
  }

  
  
  let getter = aDescriptor.get;
  let setter = aDescriptor.set;
  if (getter || setter) {
    return false;
  }

  
  
  let grip = aDescriptor.value;
  if (!grip || typeof grip != "object") {
    return true;
  }

  
  let type = grip.type;
  if (["undefined", "null"].indexOf(type + "") != -1) {
    return true;
  }

  return false;
};











VariablesView.getString = function VV_getString(aGrip, aConciseFlag) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
        return "undefined";
      case "null":
        return "null";
      default:
        if (!aConciseFlag) {
          return "[" + aGrip.type + " " + aGrip.class + "]";
        } else {
          return aGrip.class;
        }
    }
  } else {
    switch (typeof aGrip) {
      case "string":
        return "\"" + aGrip + "\"";
      case "boolean":
        return aGrip ? "true" : "false";
    }
  }
  return aGrip + "";
};









VariablesView.getClass = function VV_getClass(aGrip) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
        return "token-undefined";
      case "null":
        return "token-null";
    }
  } else {
    switch (typeof aGrip) {
      case "string":
        return "token-string";
      case "boolean":
        return "token-boolean";
      case "number":
        return "token-number";
    }
  }
  return "token-other";
};










let generateId = (function() {
  let count = 0;
  return function(aName = "") {
    return aName.toLowerCase().trim().replace(/\s+/g, "-") + (++count);
  }
})();








function create({ constructor, proto }, properties = {}) {
  let propertiesObject = {
    constructor: { value: constructor }
  };
  for (let name in properties) {
    propertiesObject[name] = Object.getOwnPropertyDescriptor(properties, name);
  }
  constructor.prototype = Object.create(proto, propertiesObject);
}
