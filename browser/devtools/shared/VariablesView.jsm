




"use strict";

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const LAZY_EMPTY_DELAY = 150; 
const SEARCH_ACTION_MAX_DELAY = 1000; 

Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["VariablesView", "create"];














this.VariablesView = function VariablesView(aParentNode) {
  this._store = new Map();
  this._prevHierarchy = new Map();
  this._currHierarchy = new Map();
  this._parent = aParentNode;
  this._appendEmptyNotice();

  this._onSearchboxInput = this._onSearchboxInput.bind(this);
  this._onSearchboxKeyPress = this._onSearchboxKeyPress.bind(this);

  
  this._list = this.document.createElement("vbox");
  this._parent.appendChild(this._list);
}

VariablesView.prototype = {
  






  set rawObject(aObject) {
    this.empty();
    this.addScope().addVar().populate(aObject);
  },

  







  addScope: function VV_addScope(aName = "") {
    this._removeEmptyNotice();
    this._toggleSearch(true);

    let scope = new Scope(this, aName);
    this._store.set(scope.id, scope);
    this._currHierarchy.set(aName, scope);
    scope.header = !!aName;
    return scope;
  },

  






  empty: function VV_empty(aTimeout = LAZY_EMPTY_DELAY) {
    
    if (!this._store.size) {
      return;
    }
    
    if (this.lazyEmpty && aTimeout > 0) {
      this._emptySoon(aTimeout);
      return;
    }

    let list = this._list;
    let firstChild;

    while (firstChild = list.firstChild) {
      list.removeChild(firstChild);
    }

    this._store = new Map();
    this._appendEmptyNotice();
    this._toggleSearch(false);
  },

  














  _emptySoon: function VV__emptySoon(aTimeout) {
    let window = this.window;
    let document = this.document;

    let prevList = this._list;
    let currList = this._list = this.document.createElement("vbox");
    this._store = new Map();

    this._emptyTimeout = window.setTimeout(function() {
      this._emptyTimeout = null;

      this._parent.removeChild(prevList);
      this._parent.appendChild(currList);

      if (!this._store.size) {
        this._appendEmptyNotice();
        this._toggleSearch(false);
      }
    }.bind(this), aTimeout);
  },

  




  set enumVisible(aFlag) {
    this._enumVisible = aFlag;

    for (let [, scope] in this) {
      scope._enumVisible = aFlag;
    }
  },

  




  set nonEnumVisible(aFlag) {
    this._nonEnumVisible = aFlag;

    for (let [, scope] in this) {
      scope._nonEnumVisible = aFlag;
    }
  },

  




  set onlyEnumVisible(aFlag) {
    if (aFlag) {
      this.enumVisible = true;
      this.nonEnumVisible = false;
    } else {
      this.enumVisible = true;
      this.nonEnumVisible = true;
    }
  },

  


  enableSearch: function VV_enableSearch() {
    
    if (this._searchboxContainer) {
      return;
    }
    let document = this.document;
    let ownerView = this._parent.parentNode;

    let container = this._searchboxContainer = document.createElement("hbox");
    container.className = "devtools-toolbar";
    container.hidden = !this._store.size;

    let searchbox = this._searchboxNode = document.createElement("textbox");
    searchbox.className = "variables-searchinput devtools-searchinput";
    searchbox.setAttribute("placeholder", this._searchboxPlaceholder);
    searchbox.setAttribute("type", "search");
    searchbox.setAttribute("flex", "1");
    searchbox.addEventListener("input", this._onSearchboxInput, false);
    searchbox.addEventListener("keypress", this._onSearchboxKeyPress, false);

    container.appendChild(searchbox);
    ownerView.insertBefore(container, this._parent);
  },

  


  disableSearch: function VV_disableSearch() {
    
    if (!this._searchboxContainer) {
      return;
    }
    this._searchboxContainer.parentNode.removeChild(this._searchboxContainer);
    this._searchboxNode.addEventListener("input", this._onSearchboxInput, false);
    this._searchboxNode.addEventListener("keypress", this._onSearchboxKeyPress, false);

    this._searchboxContainer = null;
    this._searchboxNode = null;
  },

  





  _toggleSearch: function VV__toggleSearch(aVisibleFlag) {
    
    if (!this._searchboxContainer) {
      return;
    }
    this._searchboxContainer.hidden = !aVisibleFlag;
  },

  


  set searchEnabled(aFlag) aFlag ? this.enableSearch() : this.disableSearch(),

  


  get searchEnabled() !!this._searchboxContainer,

  


  delayedSearch: true,

  





  scheduleSearch: function VV_scheduleSearch(aQuery) {
    if (!this.delayedSearch) {
      this.performSearch(aQuery);
      return;
    }
    let delay = Math.max(SEARCH_ACTION_MAX_DELAY / aQuery.length, 0);

    this.window.clearTimeout(this._searchTimeout);
    this._searchFunction = this._startSearch.bind(this, aQuery);
    this._searchTimeout = this.window.setTimeout(this._searchFunction, delay);
  },

  





  performSearch: function VV_performSearch(aQuery) {
    this.window.clearTimeout(this._searchTimeout);
    this._searchFunction = null;
    this._startSearch(aQuery);
  },

  






  _startSearch: function VV__startSearch(aQuery) {
    for (let [, scope] in this) {
      switch (aQuery) {
        case "":
          scope.expand();
          
        case null:
          scope._performSearch("");
          break;
        default:
          scope._performSearch(aQuery.toLowerCase());
          break;
      }
    }
  },

  


  expandFirstSearchResults: function VV_expandFirstSearchResults() {
    for (let [, scope] in this) {
      for (let [, variable] in scope) {
        if (variable._isMatch) {
          variable.expand();
          break;
        }
      }
    }
  },

  



  set searchPlaceholder(aValue) {
    if (this._searchboxNode) {
      this._searchboxNode.setAttribute("placeholder", aValue);
    }
    this._searchboxPlaceholder = aValue;
  },

  


  _onSearchboxInput: function VV__onSearchboxInput() {
    this.performSearch(this._searchboxNode.value);
  },

  


  _onSearchboxKeyPress: function VV__onSearchboxKeyPress(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._onSearchboxInput();
        return;
      case e.DOM_VK_ESCAPE:
        this._searchboxNode.value = "";
        this._onSearchboxInput();
        return;
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
  lazyEmpty: false,
  _store: null,
  _prevHierarchy: null,
  _currHierarchy: null,
  _emptyTimeout: null,
  _searchTimeout: null,
  _searchFunction: null,
  _enumVisible: true,
  _nonEnumVisible: true,
  _parent: null,
  _list: null,
  _searchboxNode: null,
  _searchboxContainer: null,
  _searchboxPlaceholder: "",
  _emptyTextNode: null,
  _emptyTextValue: ""
};












function Scope(aView, aName, aFlags = {}) {
  this.show = this.show.bind(this);
  this.hide = this.hide.bind(this);
  this.expand = this.expand.bind(this);
  this.collapse = this.collapse.bind(this);
  this.toggle = this.toggle.bind(this);
  this._openEnum = this._openEnum.bind(this);
  this._openNonEnum = this._openNonEnum.bind(this);

  this.ownerView = aView;
  this.eval = aView.eval;
  this.switch = aView.switch;
  this.delete = aView.delete;

  this._store = new Map();
  this._init(aName.trim(), aFlags);
}

Scope.prototype = {
  



















  addVar: function S_addVar(aName = "", aDescriptor = {}) {
    if (this._store.has(aName)) {
      return null;
    }

    let variable = new Variable(this, aName, aDescriptor);
    this._store.set(aName, variable);
    this._variablesView._currHierarchy.set(variable._absoluteName, variable);
    variable.header = !!aName;
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

  


  expand: function S_expand() {
    if (this._isExpanded || this._locked) {
      return;
    }
    if (this._variablesView._enumVisible) {
      this._openEnum();
    }
    if (this._variablesView._nonEnumVisible) {
      Services.tm.currentThread.dispatch({ run: this._openNonEnum }, 0);
    }
    this._isExpanded = true;

    if (this.onexpand) {
      this.onexpand(this);
    }
  },

  


  collapse: function S_collapse() {
    if (!this._isExpanded || this._locked) {
      return;
    }
    this._arrow.removeAttribute("open");
    this._enum.removeAttribute("open");
    this._nonenum.removeAttribute("open");
    this._isExpanded = false;

    if (this.oncollapse) {
      this.oncollapse(this);
    }
  },

  


  toggle: function S_toggle(e) {
    if (e && e.button != 0) {
      
      return;
    }
    this._wasToggled = true;
    this.expanded ^= 1;

    
    for (let [, variable] in this) {
      variable.header = true;
      variable._match = true;
    }
    if (this.ontoggle) {
      this.ontoggle(this);
    }
  },

  


  showHeader: function S_showHeader() {
    if (this._isHeaderVisible) {
      return;
    }
    this._target.removeAttribute("non-header");
    this._isHeaderVisible = true;
  },

  



  hideHeader: function S_hideHeader() {
    if (!this._isHeaderVisible) {
      return;
    }
    this.expand();
    this._target.setAttribute("non-header", "");
    this._isHeaderVisible = false;
  },

  


  showArrow: function S_showArrow() {
    if (this._isArrowVisible) {
      return;
    }
    this._arrow.removeAttribute("invisible");
    this._isArrowVisible = true;
  },

  


  hideArrow: function S_hideArrow() {
    if (!this._isArrowVisible) {
      return;
    }
    this._arrow.setAttribute("invisible", "");
    this._isArrowVisible = false;
  },

  



  get visible() this._isShown,

  



  get expanded() this._isExpanded,

  



  get header() this._isHeaderVisible,

  



  get twisty() this._isArrowVisible,

  



  set visible(aFlag) aFlag ? this.show() : this.hide(),

  



  set expanded(aFlag) aFlag ? this.expand() : this.collapse(),

  



  set header(aFlag) aFlag ? this.showHeader() : this.hideHeader(),

  



  set twisty(aFlag) aFlag ? this.showArrow() : this.hideArrow(),

  




  showDescriptorTooltip: true,

  



  allowNameInput: false,

  



  allowValueInput: true,

  



  allowDeletion: false,

  


  contextMenu: "",

  



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
    element.id = this._idString;
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

  



  set onmouseover(aCallback) {
    this._title.addEventListener("mouseover", aCallback, false);
  },

  


  _openEnum: function S__openEnum() {
    this._arrow.setAttribute("open", "");
    this._enum.setAttribute("open", "");
  },

  


  _openNonEnum: function S__openNonEnum() {
    this._nonenum.setAttribute("open", "");
  },

  



  set _enumVisible(aFlag) {
    for (let [, variable] in this) {
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
    for (let [, variable] in this) {
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

  






  _performSearch: function S__performSearch(aLowerCaseQuery) {
    for (let [, variable] in this) {
      let currentObject = variable;
      let lowerCaseName = variable._nameString.toLowerCase();
      let lowerCaseValue = variable._valueString.toLowerCase();

      
      if (!lowerCaseName.contains(aLowerCaseQuery) &&
          !lowerCaseValue.contains(aLowerCaseQuery)) {
        variable._match = false;
      }
      
      else {
        variable._match = true;

        
        
        

        if (variable._wasToggled && aLowerCaseQuery) {
          variable.expand();
        }
        if (variable._isExpanded && !aLowerCaseQuery) {
          variable._wasToggled = true;
        }

        
        
        

        while ((variable = variable.ownerView) &&  
               (variable instanceof Scope ||
                variable instanceof Variable ||
                variable instanceof Property)) {

          
          variable._match = true;
          aLowerCaseQuery && variable.expand();
        }
      }

      
      if (currentObject._wasToggled ||
          currentObject.getter ||
          currentObject.setter) {
        currentObject._performSearch(aLowerCaseQuery);
      }
    }
  },

  



  set _match(aStatus) {
    if (this._isMatch == aStatus) {
      return;
    }
    if (aStatus) {
      this._isMatch = true;
      this.target.removeAttribute("non-match");
    } else {
      this._isMatch = false;
      this.target.setAttribute("non-match", "");
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
  fetched: false,
  _committed: false,
  _locked: false,
  _isShown: true,
  _isExpanded: false,
  _wasToggled: false,
  _isHeaderVisible: true,
  _isArrowVisible: true,
  _isMatch: true,
  _store: null,
  _idString: "",
  _nameString: "",
  _target: null,
  _arrow: null,
  _name: null,
  _title: null,
  _enum: null,
  _nonenum: null
};












function Variable(aScope, aName, aDescriptor) {
  this._onClose = this._onClose.bind(this);
  this._displayTooltip = this._displayTooltip.bind(this);
  this._activateNameInput = this._activateNameInput.bind(this);
  this._activateValueInput = this._activateValueInput.bind(this);
  this._deactivateNameInput = this._deactivateNameInput.bind(this);
  this._deactivateValueInput = this._deactivateValueInput.bind(this);
  this._onNameInputKeyPress = this._onNameInputKeyPress.bind(this);
  this._onValueInputKeyPress = this._onValueInputKeyPress.bind(this);

  Scope.call(this, aScope, aName, aDescriptor);
  this._setGrip(aDescriptor.value);
  this._symbolicName = aName;
  this._absoluteName = aScope.name + "." + aName;
  this._initialDescriptor = aDescriptor;
}

create({ constructor: Variable, proto: Scope.prototype }, {
  



















  addProperty: function V_addProperty(aName = "", aDescriptor = {}) {
    if (this._store.has(aName)) {
      return null;
    }

    let property = new Property(this, aName, aDescriptor);
    this._store.set(aName, property);
    this._variablesView._currHierarchy.set(property._absoluteName, property);
    property.header = !!aName;
    return property;
  },

  
















  addProperties: function V_addProperties(aProperties) {
    
    let sortedPropertyNames = Object.keys(aProperties).sort();

    for (let name of sortedPropertyNames) {
      this.addProperty(name, aProperties[name]);
    }
  },

  





  populate: function V_populate(aObject) {
    
    if (this.fetched) {
      return;
    }
    this.fetched = true;

    
    let sortedPropertyNames = Object.getOwnPropertyNames(aObject).sort();
    let prototype = Object.getPrototypeOf(aObject);

    
    for (let name of sortedPropertyNames) {
      let descriptor = Object.getOwnPropertyDescriptor(aObject, name);
      if (descriptor.get || descriptor.set) {
        this._addRawNonValueProperty(name, descriptor);
      } else {
        this._addRawValueProperty(name, descriptor, aObject[name]);
      }
    }
    
    if (prototype) {
      this._addRawValueProperty("__proto__", {}, prototype);
    }
  },

  










  _addRawValueProperty: function V__addRawValueProperty(aName, aDescriptor, aValue) {
    let descriptor = Object.create(aDescriptor);
    descriptor.value = VariablesView.getGrip(aValue);

    let propertyItem = this.addProperty(aName, descriptor);

    
    
    if (!VariablesView.isPrimitive(descriptor)) {
      propertyItem.onexpand = this.populate.bind(propertyItem, aValue);
    }

    return propertyItem;
  },

  








  _addRawNonValueProperty: function V__addRawNonValueProperty(aName, aDescriptor) {
    let descriptor = Object.create(aDescriptor);
    descriptor.get = VariablesView.getGrip(aDescriptor.get);
    descriptor.set = VariablesView.getGrip(aDescriptor.set);

    let propertyItem = this.addProperty(aName, descriptor);
    return propertyItem;
  },

  


  get value() this._initialDescriptor.value,

  


  get getter() this._initialDescriptor.get,

  


  get setter() this._initialDescriptor.set,

  














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
    this._prepareTooltip();
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
    separatorLabel.setAttribute("value", this.ownerView.separator);

    let valueLabel = this._valueLabel = document.createElement("label");
    valueLabel.className = "value plain";

    this._title.appendChild(separatorLabel);
    this._title.appendChild(valueLabel);

    let isPrimitive = VariablesView.isPrimitive(aDescriptor);
    let isUndefined = VariablesView.isUndefined(aDescriptor);

    if (isPrimitive || isUndefined) {
      this.hideArrow();
    }
    if (!isUndefined && (aDescriptor.get || aDescriptor.set)) {
      this.addProperty("get", { value: aDescriptor.get });
      this.addProperty("set", { value: aDescriptor.set });
      this.expand();
      separatorLabel.hidden = true;
      valueLabel.hidden = true;
    }
    if (this.ownerView.allowDeletion) {
      let closeNode = this._closeNode = document.createElement("toolbarbutton");
      closeNode.className = "dbg-variables-delete plain devtools-closebutton";
      closeNode.addEventListener("click", this._onClose, false);
      this._title.appendChild(closeNode);
    }
    if (this.ownerView.contextMenu) {
      this._title.setAttribute("context", this.ownerView.contextMenu);
    }
  },

  


  _prepareTooltip: function V__prepareTooltip() {
    this._target.addEventListener("mouseover", this._displayTooltip, false);
  },

  


  _displayTooltip: function V__displayTooltip() {
    this._target.removeEventListener("mouseover", this._displayTooltip, false);
    let document = this.document;

    if (this.ownerView.showDescriptorTooltip) {
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
    }
    if (this.ownerView.allowNameInput) {
      this._name.setAttribute("tooltiptext", L10N.getStr("variablesEditableNameTooltip"));
    }
    if (this.ownerView.allowValueInput) {
      this._valueLabel.setAttribute("tooltiptext", L10N.getStr("variablesEditableValueTooltip"));
    }
    if (this.ownerView.allowDeletion) {
      this._closeNode.setAttribute("tooltiptext", L10N.getStr("variablesCloseButtonTooltip"));
    }
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
    this._name.addEventListener("dblclick", this._activateNameInput, false);
    this._valueLabel.addEventListener("click", this._activateValueInput, false);
  },

  


  _onClose: function V__onClose() {
    this.hide();

    if (this.delete) {
      this.delete(this);
    }
  },

  









  _activateInput: function V__activateInput(aLabel, aClassName, aCallbacks) {
    let initialString = aLabel.getAttribute("value");

    
    
    let input = this.document.createElement("textbox");
    input.setAttribute("value", initialString);
    input.className = "plain " + aClassName;
    input.width = this._target.clientWidth;

    aLabel.parentNode.replaceChild(input, aLabel);
    input.select();

    
    
    
    if (aLabel.getAttribute("value").match(/^"[^"]*"$/)) {
      input.selectionEnd--;
      input.selectionStart++;
    }

    input.addEventListener("keypress", aCallbacks.onKeypress, false);
    input.addEventListener("blur", aCallbacks.onBlur, false);

    this._prevExpandable = this.twisty;
    this._prevExpanded = this.expanded;
    this.collapse();
    this.hideArrow();
    this._locked = true;
  },

  







  _deactivateInput: function V__deactivateInput(aLabel, aInput, aCallbacks) {
    aInput.parentNode.replaceChild(aLabel, aInput);
    aInput.removeEventListener("keypress", aCallbacks.onKeypress, false);
    aInput.removeEventListener("blur", aCallbacks.onBlur, false);

    this._locked = false;
    this.twisty = this._prevExpandable;
    this.expanded = this._prevExpanded;
  },

  


  _activateNameInput: function V__activateNameInput(e) {
    if (e && e.button != 0) {
      
      return;
    }
    if (!this.ownerView.allowNameInput || !this.switch) {
      return;
    }
    this._activateInput(this._name, "element-name-input", {
      onKeypress: this._onNameInputKeyPress,
      onBlur: this._deactivateNameInput
    });
    this._separatorLabel.hidden = true;
    this._valueLabel.hidden = true;
  },

  


  _deactivateNameInput: function V__deactivateNameInput(e) {
    this._deactivateInput(this._name, e.target, {
      onKeypress: this._onNameInputKeyPress,
      onBlur: this._deactivateNameInput
    });
    this._separatorLabel.hidden = false;
    this._valueLabel.hidden = false;
  },

  


  _activateValueInput: function V__activateValueInput(e) {
    if (e && e.button != 0) {
      
      return;
    }
    if (!this.ownerView.allowValueInput || !this.eval) {
      return;
    }
    this._activateInput(this._valueLabel, "element-value-input", {
      onKeypress: this._onValueInputKeyPress,
      onBlur: this._deactivateValueInput
    });
  },

  


  _deactivateValueInput: function V__deactivateValueInput(e) {
    this._deactivateInput(this._valueLabel, e.target, {
      onKeypress: this._onValueInputKeyPress,
      onBlur: this._deactivateValueInput
    });
  },

  


  _disable: function V__disable() {
    this.twisty = false;
    this._separatorLabel.hidden = true;
    this._valueLabel.hidden = true;
    this._enum.hidden = true;
    this._nonenum.hidden = true;
  },

  


  _saveNameInput: function V__saveNameInput(e) {
    let input = e.target;
    let initialString = this._name.getAttribute("value");
    let currentString = input.value.trim();
    this._deactivateNameInput(e);

    if (initialString != currentString) {
      this._disable();
      this._name.value = currentString;
      this.switch(this, currentString);
    }
  },

  


  _saveValueInput: function V__saveValueInput(e) {
    let input = e.target;
    let initialString = this._valueLabel.getAttribute("value");
    let currentString = input.value.trim();
    this._deactivateValueInput(e);

    if (initialString != currentString) {
      this._disable();
      this.eval(this._symbolicName + "=" + currentString);
    }
  },

  


  _onNameInputKeyPress: function V__onNameInputKeyPress(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveNameInput(e);
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateNameInput(e);
        return;
    }
  },

  


  _onValueInputKeyPress: function V__onValueInputKeyPress(e) {
    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveValueInput(e);
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateValueInput(e);
        return;
    }
  },

  _symbolicName: "",
  _absoluteName: "",
  _initialDescriptor: null,
  _separatorLabel: null,
  _valueLabel: null,
  _closeNode: null,
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
    this._prepareTooltip();
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
    
    if (this.commitHierarchyIgnoredItems[currVariable._nameString]) {
      continue;
    }

    
    
    let prevVariable = prevHierarchy.get(absoluteName);
    let expanded = false;
    let changed = false;

    
    
    
    if (prevVariable) {
      expanded = prevVariable._isExpanded;
      changed = prevVariable._valueString != currVariable._valueString;
    }

    
    
    currVariable._committed = true;

    
    if (expanded) {
      currVariable._wasToggled = prevVariable._wasToggled;
      currVariable.expand();
    }
    
    if (!changed) {
      continue;
    }

    
    
    
    this.window.setTimeout(function(aTarget) {
      aTarget.addEventListener("transitionend", function onEvent() {
        aTarget.removeEventListener("transitionend", onEvent, false);
        aTarget.removeAttribute("changed");
      }, false);
      aTarget.setAttribute("changed", "");
    }.bind(this, currVariable.target), LAZY_EMPTY_DELAY + 1);
  }
};



VariablesView.prototype.commitHierarchyIgnoredItems = Object.create(null, {
  "window": { value: true },
  "this": { value: true }
});








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
  if (type == "undefined" || type == "null") {
    return true;
  }

  return false;
};







VariablesView.isUndefined = function VV_isUndefined(aDescriptor) {
  
  
  let getter = aDescriptor.get;
  let setter = aDescriptor.set;
  if (typeof getter == "object" && getter.type == "undefined" &&
      typeof setter == "object" && setter.type == "undefined") {
    return true;
  }

  
  
  
  let grip = aDescriptor.value;
  if (grip && grip.type == "undefined") {
    return true;
  }

  return false;
};







VariablesView.isFalsy = function VV_isFalsy(aDescriptor) {
  if (!aDescriptor || typeof aDescriptor != "object") {
    return true;
  }

  
  
  let grip = aDescriptor.value;
  if (typeof grip != "object") {
    return !grip;
  }

  
  let type = grip.type;
  if (type == "undefined" || type == "null") {
    return true;
  }

  return false;
};









VariablesView.getGrip = function VV_getGrip(aValue) {
  if (aValue === undefined) {
    return { type: "undefined" };
  }
  if (aValue === null) {
    return { type: "null" };
  }
  if (typeof aValue == "object" || typeof aValue == "function") {
    if (aValue.constructor) {
      return { type: "object", class: aValue.constructor.name };
    } else {
      return { type: "object", class: "Object" };
    }
  }
  return aValue;
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




let L10N = {
  





  getStr: function L10N_getStr(aName) {
    return this.stringBundle.GetStringFromName(aName);
  }
};

XPCOMUtils.defineLazyGetter(L10N, "stringBundle", function() {
  return Services.strings.createBundle(DBG_STRINGS_URI);
});





Scope.prototype.separator = L10N.getStr("variablesSeparatorLabel");










let generateId = (function() {
  let count = 0;
  return function VV_generateId(aName = "") {
    return aName.toLowerCase().trim().replace(/\s+/g, "-") + (++count);
  };
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
