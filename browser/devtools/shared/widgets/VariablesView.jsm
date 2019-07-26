




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const LAZY_EMPTY_DELAY = 150; 
const LAZY_EXPAND_DELAY = 50; 
const LAZY_APPEND_DELAY = 100; 
const LAZY_APPEND_BATCH = 100; 
const PAGE_SIZE_SCROLL_HEIGHT_RATIO = 100;
const PAGE_SIZE_MAX_JUMPS = 30;
const SEARCH_ACTION_MAX_DELAY = 300; 

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource:///modules/devtools/shared/event-emitter.js");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

Object.defineProperty(this, "WebConsoleUtils", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/utils").Utils;
  },
  configurable: true,
  enumerable: true
});

Object.defineProperty(this, "NetworkHelper", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/network-helper");
  },
  configurable: true,
  enumerable: true
});

this.EXPORTED_SYMBOLS = ["VariablesView"];




const STR = Services.strings.createBundle(DBG_STRINGS_URI);

















this.VariablesView = function VariablesView(aParentNode, aFlags = {}) {
  this._store = []; 
  this._itemsByElement = new WeakMap();
  this._prevHierarchy = new Map();
  this._currHierarchy = new Map();

  this._parent = aParentNode;
  this._parent.classList.add("variables-view-container");
  this._appendEmptyNotice();

  this._onSearchboxInput = this._onSearchboxInput.bind(this);
  this._onSearchboxKeyPress = this._onSearchboxKeyPress.bind(this);
  this._onViewKeyPress = this._onViewKeyPress.bind(this);

  
  this._list = this.document.createElement("scrollbox");
  this._list.setAttribute("orient", "vertical");
  this._list.addEventListener("keypress", this._onViewKeyPress, false);
  this._parent.appendChild(this._list);
  this._boxObject = this._list.boxObject.QueryInterface(Ci.nsIScrollBoxObject);

  for (let name in aFlags) {
    this[name] = aFlags[name];
  }

  EventEmitter.decorate(this);
};

VariablesView.prototype = {
  






  set rawObject(aObject) {
    this.empty();
    this.addScope().addItem().populate(aObject, { sorted: true });
  },

  







  addScope: function(aName = "") {
    this._removeEmptyNotice();
    this._toggleSearchVisibility(true);

    let scope = new Scope(this, aName);
    this._store.push(scope);
    this._itemsByElement.set(scope._target, scope);
    this._currHierarchy.set(aName, scope);
    scope.header = !!aName;
    return scope;
  },

  






  empty: function(aTimeout = this.lazyEmptyDelay) {
    
    if (!this._store.length) {
      return;
    }
    
    if (this.lazyEmpty && aTimeout > 0) {
      this._emptySoon(aTimeout);
      return;
    }

    let list = this._list;

    while (list.hasChildNodes()) {
      list.firstChild.remove();
    }

    this._store.length = 0;
    this._itemsByElement.clear();

    this._appendEmptyNotice();
    this._toggleSearchVisibility(false);
  },

  














  _emptySoon: function(aTimeout) {
    let prevList = this._list;
    let currList = this._list = this.document.createElement("scrollbox");

    this._store.length = 0;
    this._itemsByElement.clear();

    this.window.setTimeout(() => {
      prevList.removeEventListener("keypress", this._onViewKeyPress, false);
      currList.addEventListener("keypress", this._onViewKeyPress, false);
      currList.setAttribute("orient", "vertical");

      this._parent.removeChild(prevList);
      this._parent.appendChild(currList);
      this._boxObject = currList.boxObject.QueryInterface(Ci.nsIScrollBoxObject);

      if (!this._store.length) {
        this._appendEmptyNotice();
        this._toggleSearchVisibility(false);
      }
    }, aTimeout);
  },

  


  controller: null,

  


  lazyEmptyDelay: LAZY_EMPTY_DELAY,

  



  lazyEmpty: false,

  



  lazyAppend: true,

  



  lazyExpand: true,

  


  lazySearch: true,

  






  eval: null,

  






  switch: null,

  






  delete: null,

  



  preventDisableOnChage: false,

  







  preventDescriptorModifiers: false,

  






  editableValueTooltip: STR.GetStringFromName("variablesEditableValueTooltip"),

  






  editableNameTooltip: STR.GetStringFromName("variablesEditableNameTooltip"),

  







  editButtonTooltip: STR.GetStringFromName("variablesEditButtonTooltip"),

  






  deleteButtonTooltip: STR.GetStringFromName("variablesCloseButtonTooltip"),

  





  contextMenuId: "",

  





  separatorStr: STR.GetStringFromName("variablesSeparatorLabel"),

  




  set enumVisible(aFlag) {
    this._enumVisible = aFlag;

    for (let scope of this._store) {
      scope._enumVisible = aFlag;
    }
  },

  




  set nonEnumVisible(aFlag) {
    this._nonEnumVisible = aFlag;

    for (let scope of this._store) {
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

  



  set searchEnabled(aFlag) aFlag ? this._enableSearch() : this._disableSearch(),

  



  get searchEnabled() !!this._searchboxContainer,

  



  set searchPlaceholder(aValue) {
    if (this._searchboxNode) {
      this._searchboxNode.setAttribute("placeholder", aValue);
    }
    this._searchboxPlaceholder = aValue;
  },

  



  get searchPlaceholder() this._searchboxPlaceholder,

  



  _enableSearch: function() {
    
    if (this._searchboxContainer) {
      return;
    }
    let document = this.document;
    let ownerView = this._parent.parentNode;

    let container = this._searchboxContainer = document.createElement("hbox");
    container.className = "devtools-toolbar";

    
    
    container.hidden = !this._store.length;

    let searchbox = this._searchboxNode = document.createElement("textbox");
    searchbox.className = "variables-view-searchinput devtools-searchinput";
    searchbox.setAttribute("placeholder", this._searchboxPlaceholder);
    searchbox.setAttribute("type", "search");
    searchbox.setAttribute("flex", "1");
    searchbox.addEventListener("input", this._onSearchboxInput, false);
    searchbox.addEventListener("keypress", this._onSearchboxKeyPress, false);

    container.appendChild(searchbox);
    ownerView.insertBefore(container, this._parent);
  },

  



  _disableSearch: function() {
    
    if (!this._searchboxContainer) {
      return;
    }
    this._searchboxContainer.remove();
    this._searchboxNode.removeEventListener("input", this._onSearchboxInput, false);
    this._searchboxNode.removeEventListener("keypress", this._onSearchboxKeyPress, false);

    this._searchboxContainer = null;
    this._searchboxNode = null;
  },

  






  _toggleSearchVisibility: function(aVisibleFlag) {
    
    if (!this._searchboxContainer) {
      return;
    }
    this._searchboxContainer.hidden = !aVisibleFlag;
  },

  


  _onSearchboxInput: function() {
    this.scheduleSearch(this._searchboxNode.value);
  },

  


  _onSearchboxKeyPress: function(e) {
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

  







  scheduleSearch: function(aToken, aWait) {
    
    if (!this.lazySearch) {
      this._doSearch(aToken);
      return;
    }

    
    let maxDelay = SEARCH_ACTION_MAX_DELAY;
    let delay = aWait === undefined ? maxDelay / aToken.length : aWait;

    
    setNamedTimeout("vview-search", delay, () => this._doSearch(aToken));
  },

  










  _doSearch: function(aToken) {
    for (let scope of this._store) {
      switch (aToken) {
        case "":
        case null:
        case undefined:
          scope.expand();
          scope._performSearch("");
          break;
        default:
          scope._performSearch(aToken.toLowerCase());
          break;
      }
    }
  },

  










  _findInVisibleItems: function(aPredicate) {
    for (let scope of this._store) {
      let result = scope._findInVisibleItems(aPredicate);
      if (result) {
        return result;
      }
    }
    return null;
  },

  











  _findInVisibleItemsReverse: function(aPredicate) {
    for (let i = this._store.length - 1; i >= 0; i--) {
      let scope = this._store[i];
      let result = scope._findInVisibleItemsReverse(aPredicate);
      if (result) {
        return result;
      }
    }
    return null;
  },

  







  getScopeForNode: function(aNode) {
    let item = this._itemsByElement.get(aNode);
    
    if (item && !(item instanceof Variable)) {
      return item;
    }
    return null;
  },

  








  getItemForNode: function(aNode) {
    return this._itemsByElement.get(aNode);
  },

  





  getFocusedItem: function() {
    let focused = this.document.commandDispatcher.focusedElement;
    return this.getItemForNode(focused);
  },

  


  focusFirstVisibleItem: function() {
    let focusableItem = this._findInVisibleItems(item => item.focusable);
    if (focusableItem) {
      this._focusItem(focusableItem);
    }
    this._parent.scrollTop = 0;
    this._parent.scrollLeft = 0;
  },

  


  focusLastVisibleItem: function() {
    let focusableItem = this._findInVisibleItemsReverse(item => item.focusable);
    if (focusableItem) {
      this._focusItem(focusableItem);
    }
    this._parent.scrollTop = this._parent.scrollHeight;
    this._parent.scrollLeft = 0;
  },

  


  focusNextItem: function() {
    this.focusItemAtDelta(+1);
  },

  


  focusPrevItem: function() {
    this.focusItemAtDelta(-1);
  },

  






  focusItemAtDelta: function(aDelta) {
    let direction = aDelta > 0 ? "advanceFocus" : "rewindFocus";
    let distance = Math.abs(Math[aDelta > 0 ? "ceil" : "floor"](aDelta));
    while (distance--) {
      if (!this._focusChange(direction)) {
        break; 
      }
    }
  },

  








  _focusChange: function(aDirection) {
    let commandDispatcher = this.document.commandDispatcher;
    let prevFocusedElement = commandDispatcher.focusedElement;
    let currFocusedItem = null;

    do {
      commandDispatcher.suppressFocusScroll = true;
      commandDispatcher[aDirection]();

      
      
      if (!(currFocusedItem = this.getFocusedItem())) {
        prevFocusedElement.focus();
        return false;
      }
    } while (!currFocusedItem.focusable);

    
    return true;
  },

  









  _focusItem: function(aItem, aCollapseFlag) {
    if (!aItem.focusable) {
      return false;
    }
    if (aCollapseFlag) {
      aItem.collapse();
    }
    aItem._target.focus();
    this._boxObject.ensureElementIsVisible(aItem._arrow);
    return true;
  },

  


  _onViewKeyPress: function(e) {
    let item = this.getFocusedItem();

    
    ViewHelpers.preventScrolling(e);

    switch (e.keyCode) {
      case e.DOM_VK_UP:
        
        this.focusPrevItem(true);
        return;

      case e.DOM_VK_DOWN:
        
        this.focusNextItem(true);
        return;

      case e.DOM_VK_LEFT:
        
        if (item._isExpanded && item._isArrowVisible) {
          item.collapse();
        } else {
          this._focusItem(item.ownerView);
        }
        return;

      case e.DOM_VK_RIGHT:
        
        if (!item._isArrowVisible) {
          return;
        }
        
        if (!item._isExpanded) {
          item.expand();
        } else {
          this.focusNextItem(true);
        }
        return;

      case e.DOM_VK_PAGE_UP:
        
        this.focusItemAtDelta(-(this.pageSize || Math.min(Math.floor(this._list.scrollHeight /
          PAGE_SIZE_SCROLL_HEIGHT_RATIO),
          PAGE_SIZE_MAX_JUMPS)));
        return;

      case e.DOM_VK_PAGE_DOWN:
        
        this.focusItemAtDelta(+(this.pageSize || Math.min(Math.floor(this._list.scrollHeight /
          PAGE_SIZE_SCROLL_HEIGHT_RATIO),
          PAGE_SIZE_MAX_JUMPS)));
        return;

      case e.DOM_VK_HOME:
        this.focusFirstVisibleItem();
        return;

      case e.DOM_VK_END:
        this.focusLastVisibleItem();
        return;

      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        
        if (item instanceof Variable) {
          if (e.metaKey || e.altKey || e.shiftKey) {
            item._activateNameInput();
          } else {
            item._activateValueInput();
          }
        }
        return;

      case e.DOM_VK_DELETE:
      case e.DOM_VK_BACK_SPACE:
        
        if (item instanceof Variable) {
          item._onDelete(e);
        }
        return;
    }
  },

  




  pageSize: 0,

  



  set emptyText(aValue) {
    if (this._emptyTextNode) {
      this._emptyTextNode.setAttribute("value", aValue);
    }
    this._emptyTextValue = aValue;
    this._appendEmptyNotice();
  },

  


  _appendEmptyNotice: function() {
    if (this._emptyTextNode || !this._emptyTextValue) {
      return;
    }

    let label = this.document.createElement("label");
    label.className = "variables-view-empty-notice";
    label.setAttribute("value", this._emptyTextValue);

    this._parent.appendChild(label);
    this._emptyTextNode = label;
  },

  


  _removeEmptyNotice: function() {
    if (!this._emptyTextNode) {
      return;
    }

    this._parent.removeChild(this._emptyTextNode);
    this._emptyTextNode = null;
  },

  



  get parentNode() this._parent,

  



  get document() this._document || (this._document = this._parent.ownerDocument),

  



  get window() this._window || (this._window = this.document.defaultView),

  _document: null,
  _window: null,

  _store: null,
  _prevHierarchy: null,
  _currHierarchy: null,
  _enumVisible: true,
  _nonEnumVisible: true,
  _parent: null,
  _list: null,
  _boxObject: null,
  _searchboxNode: null,
  _searchboxContainer: null,
  _searchboxPlaceholder: "",
  _emptyTextNode: null,
  _emptyTextValue: ""
};

VariablesView.NON_SORTABLE_CLASSES = [
  "Array",
  "Int8Array",
  "Uint8Array",
  "Uint8ClampedArray",
  "Int16Array",
  "Uint16Array",
  "Int32Array",
  "Uint32Array",
  "Float32Array",
  "Float64Array"
];







VariablesView.isSortable = function(aClassName) {
  return VariablesView.NON_SORTABLE_CLASSES.indexOf(aClassName) == -1;
};













VariablesView.simpleValueEvalMacro = function(aItem, aCurrentString, aPrefix = "") {
  return aPrefix + aItem._symbolicName + "=" + aCurrentString;
};














VariablesView.overrideValueEvalMacro = function(aItem, aCurrentString, aPrefix = "") {
  let property = "\"" + aItem._nameString + "\"";
  let parent = aPrefix + aItem.ownerView._symbolicName || "this";

  return "Object.defineProperty(" + parent + "," + property + "," +
    "{ value: " + aCurrentString +
    ", enumerable: " + parent + ".propertyIsEnumerable(" + property + ")" +
    ", configurable: true" +
    ", writable: true" +
    "})";
};













VariablesView.getterOrSetterEvalMacro = function(aItem, aCurrentString, aPrefix = "") {
  let type = aItem._nameString;
  let propertyObject = aItem.ownerView;
  let parentObject = propertyObject.ownerView;
  let property = "\"" + propertyObject._nameString + "\"";
  let parent = aPrefix + parentObject._symbolicName || "this";

  switch (aCurrentString) {
    case "":
    case "null":
    case "undefined":
      let mirrorType = type == "get" ? "set" : "get";
      let mirrorLookup = type == "get" ? "__lookupSetter__" : "__lookupGetter__";

      
      
      if ((type == "set" && propertyObject.getter.type == "undefined") ||
          (type == "get" && propertyObject.setter.type == "undefined")) {
        
        return propertyObject.evaluationMacro(propertyObject, "undefined", aPrefix);
      }

      
      
      
      
      
      
      
      return "Object.defineProperty(" + parent + "," + property + "," +
        "{" + mirrorType + ":" + parent + "." + mirrorLookup + "(" + property + ")" +
        "," + type + ":" + undefined +
        ", enumerable: " + parent + ".propertyIsEnumerable(" + property + ")" +
        ", configurable: true" +
        "})";

    default:
      
      if (!aCurrentString.startsWith("function")) {
        let header = "function(" + (type == "set" ? "value" : "") + ")";
        let body = "";
        
        
        if (aCurrentString.contains("return ")) {
          body = "{" + aCurrentString + "}";
        }
        
        else if (aCurrentString.startsWith("{")) {
          body = aCurrentString;
        }
        
        else {
          body = "(" + aCurrentString + ")";
        }
        aCurrentString = header + body;
      }

      
      let defineType = type == "get" ? "__defineGetter__" : "__defineSetter__";

      
      let defineFunc = "eval(\"(" + aCurrentString.replace(/"/g, "\\$&") + ")\")";

      
      
      return parent + "." + defineType + "(" + property + "," + defineFunc + ")";
  }
};







VariablesView.getterOrSetterDeleteCallback = function(aItem) {
  aItem._disable();

  
  
  aItem.ownerView.eval(aItem.evaluationMacro(aItem, ""));

  return true; 
};













function Scope(aView, aName, aFlags = {}) {
  this.ownerView = aView;

  this._onClick = this._onClick.bind(this);
  this._openEnum = this._openEnum.bind(this);
  this._openNonEnum = this._openNonEnum.bind(this);
  this._batchAppend = this._batchAppend.bind(this);

  
  
  this.eval = aView.eval;
  this.switch = aView.switch;
  this.delete = aView.delete;
  this.editableValueTooltip = aView.editableValueTooltip;
  this.editableNameTooltip = aView.editableNameTooltip;
  this.editButtonTooltip = aView.editButtonTooltip;
  this.deleteButtonTooltip = aView.deleteButtonTooltip;
  this.preventDescriptorModifiers = aView.preventDescriptorModifiers;
  this.contextMenuId = aView.contextMenuId;
  this.separatorStr = aView.separatorStr;

  
  
  
  XPCOMUtils.defineLazyGetter(this, "_store", () => new Map());
  XPCOMUtils.defineLazyGetter(this, "_enumItems", () => []);
  XPCOMUtils.defineLazyGetter(this, "_nonEnumItems", () => []);
  XPCOMUtils.defineLazyGetter(this, "_batchItems", () => []);

  this._init(aName.trim(), aFlags);
}

Scope.prototype = {
  


  shouldPrefetch: true,

  









  _createChild: function(aName, aDescriptor) {
    return new Variable(this, aName, aDescriptor);
  },

  





















  addItem: function(aName = "", aDescriptor = {}, aRelaxed = false) {
    if (this._store.has(aName) && !aRelaxed) {
      return null;
    }

    let child = this._createChild(aName, aDescriptor);
    this._store.set(aName, child);
    this._variablesView._itemsByElement.set(child._target, child);
    this._variablesView._currHierarchy.set(child._absoluteName, child);
    child.header = !!aName;
    return child;
  },

  




















  addItems: function(aItems, aOptions = {}) {
    let names = Object.keys(aItems);

    
    if (aOptions.sorted) {
      names.sort();
    }
    
    for (let name of names) {
      let descriptor = aItems[name];
      let item = this.addItem(name, descriptor);

      if (aOptions.callback) {
        aOptions.callback(item, descriptor.value);
      }
    }
  },

  







  get: function(aName) {
    return this._store.get(aName);
  },

  








  find: function(aNode) {
    for (let [, variable] of this._store) {
      let match;
      if (variable._target == aNode) {
        match = variable;
      } else {
        match = variable.find(aNode);
      }
      if (match) {
        return match;
      }
    }
    return null;
  },

  








  isChildOf: function(aParent) {
    return this.ownerView == aParent;
  },

  








  isDescendantOf: function(aParent) {
    if (this.isChildOf(aParent)) {
      return true;
    }

    
    if (this.ownerView instanceof Scope) {
      return this.ownerView.isDescendantOf(aParent);
    }

    return false;
  },

  


  show: function() {
    this._target.hidden = false;
    this._isContentVisible = true;

    if (this.onshow) {
      this.onshow(this);
    }
  },

  


  hide: function() {
    this._target.hidden = true;
    this._isContentVisible = false;

    if (this.onhide) {
      this.onhide(this);
    }
  },

  


  expand: function() {
    if (this._isExpanded || this._locked) {
      return;
    }
    
    
    
    
    if (!this._isExpanding &&
         this._variablesView.lazyExpand &&
         this._store.size > LAZY_APPEND_BATCH) {
      this._isExpanding = true;

      
      
      this._startThrobber();
      this.window.setTimeout(this.expand.bind(this), LAZY_EXPAND_DELAY);
      return;
    }

    if (this._variablesView._enumVisible) {
      this._openEnum();
    }
    if (this._variablesView._nonEnumVisible) {
      Services.tm.currentThread.dispatch({ run: this._openNonEnum }, 0);
    }
    this._isExpanding = false;
    this._isExpanded = true;

    if (this.onexpand) {
      this.onexpand(this);
    }
  },

  


  collapse: function() {
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

  


  toggle: function(e) {
    if (e && e.button != 0) {
      
      return;
    }
    this._wasToggled = true;
    this.expanded ^= 1;

    
    for (let [, variable] of this._store) {
      variable.header = true;
      variable._matched = true;
    }
    if (this.ontoggle) {
      this.ontoggle(this);
    }
  },

  


  showHeader: function() {
    if (this._isHeaderVisible || !this._nameString) {
      return;
    }
    this._target.removeAttribute("non-header");
    this._isHeaderVisible = true;
  },

  



  hideHeader: function() {
    if (!this._isHeaderVisible) {
      return;
    }
    this.expand();
    this._target.setAttribute("non-header", "");
    this._isHeaderVisible = false;
  },

  


  showArrow: function() {
    if (this._isArrowVisible) {
      return;
    }
    this._arrow.removeAttribute("invisible");
    this._isArrowVisible = true;
  },

  


  hideArrow: function() {
    if (!this._isArrowVisible) {
      return;
    }
    this._arrow.setAttribute("invisible", "");
    this._isArrowVisible = false;
  },

  



  get visible() this._isContentVisible,

  



  get expanded() this._isExpanded,

  



  get header() this._isHeaderVisible,

  



  get twisty() this._isArrowVisible,

  



  get locked() this._locked,

  



  set visible(aFlag) aFlag ? this.show() : this.hide(),

  



  set expanded(aFlag) aFlag ? this.expand() : this.collapse(),

  



  set header(aFlag) aFlag ? this.showHeader() : this.hideHeader(),

  



  set twisty(aFlag) aFlag ? this.showArrow() : this.hideArrow(),

  



  set locked(aFlag) this._locked = aFlag,

  



  get focusable() {
    
    if (!this._nameString ||
        !this._isContentVisible ||
        !this._isHeaderVisible ||
        !this._isMatch) {
      return false;
    }
    
    let item = this;

    
    while ((item = item.ownerView) && item instanceof Scope) {
      if (!item._isExpanded) {
        return false;
      }
    }
    return true;
  },

  


  focus: function() {
    this._variablesView._focusItem(this);
  },

  





  addEventListener: function(aName, aCallback, aCapture) {
    this._title.addEventListener(aName, aCallback, aCapture);
  },

  





  removeEventListener: function(aName, aCallback, aCapture) {
    this._title.removeEventListener(aName, aCallback, aCapture);
  },

  



  get id() this._idString,

  



  get name() this._nameString,

  



  get displayValue() this._valueString,

  



  get displayValueClassName() this._valueClassName,

  



  get target() this._target,

  







  _init: function(aName, aFlags) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, "variables-view-scope", "devtools-toolbar");
    this._addEventListeners();
    this.parentNode.appendChild(this._target);
  },

  









  _displayScope: function(aName, aClassName, aTitleClassName) {
    let document = this.document;

    let element = this._target = document.createElement("vbox");
    element.id = this._idString;
    element.className = aClassName;

    let arrow = this._arrow = document.createElement("hbox");
    arrow.className = "arrow";

    let name = this._name = document.createElement("label");
    name.className = "plain name";
    name.setAttribute("value", aName);

    let title = this._title = document.createElement("hbox");
    title.className = "title " + (aTitleClassName || "");
    title.setAttribute("align", "center");

    let enumerable = this._enum = document.createElement("vbox");
    let nonenum = this._nonenum = document.createElement("vbox");
    enumerable.className = "variables-view-element-details enum";
    nonenum.className = "variables-view-element-details nonenum";

    title.appendChild(arrow);
    title.appendChild(name);

    element.appendChild(title);
    element.appendChild(enumerable);
    element.appendChild(nonenum);
  },

  


  _addEventListeners: function() {
    this._title.addEventListener("mousedown", this._onClick, false);
  },

  


  _onClick: function(e) {
    if (e.target == this._inputNode ||
        e.target == this._editNode ||
        e.target == this._deleteNode) {
      return;
    }
    this.toggle();
    this.focus();
  },

  












  _lazyAppend: function(aImmediateFlag, aEnumerableFlag, aChild) {
    
    if (aImmediateFlag || !this._variablesView.lazyAppend) {
      if (aEnumerableFlag) {
        this._enum.appendChild(aChild);
      } else {
        this._nonenum.appendChild(aChild);
      }
      return;
    }

    let window = this.window;
    let batchItems = this._batchItems;

    window.clearTimeout(this._batchTimeout);
    batchItems.push({ enumerableFlag: aEnumerableFlag, child: aChild });

    
    
    if (batchItems.length > LAZY_APPEND_BATCH) {
      
      Services.tm.currentThread.dispatch({ run: this._batchAppend }, 1);
      return;
    }
    
    
    this._batchTimeout = window.setTimeout(this._batchAppend, LAZY_APPEND_DELAY);
  },

  



  _batchAppend: function() {
    let document = this.document;
    let batchItems = this._batchItems;

    
    
    let frags = [document.createDocumentFragment(), document.createDocumentFragment()];

    for (let item of batchItems) {
      frags[~~item.enumerableFlag].appendChild(item.child);
    }
    batchItems.length = 0;
    this._enum.appendChild(frags[1]);
    this._nonenum.appendChild(frags[0]);
  },

  


  _startThrobber: function() {
    if (this._throbber) {
      this._throbber.hidden = false;
      return;
    }
    let throbber = this._throbber = this.document.createElement("hbox");
    throbber.className = "variables-view-throbber";
    this._title.insertBefore(throbber, this._spacer);
  },

  


  _stopThrobber: function() {
    if (!this._throbber) {
      return;
    }
    this._throbber.hidden = true;
  },

  


  _openEnum: function() {
    this._arrow.setAttribute("open", "");
    this._enum.setAttribute("open", "");
    this._stopThrobber();
  },

  


  _openNonEnum: function() {
    this._nonenum.setAttribute("open", "");
    this._stopThrobber();
  },

  



  set _enumVisible(aFlag) {
    for (let [, variable] of this._store) {
      variable._enumVisible = aFlag;

      if (!this._isExpanded) {
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
    for (let [, variable] of this._store) {
      variable._nonEnumVisible = aFlag;

      if (!this._isExpanded) {
        continue;
      }
      if (aFlag) {
        this._nonenum.setAttribute("open", "");
      } else {
        this._nonenum.removeAttribute("open");
      }
    }
  },

  






  _performSearch: function(aLowerCaseQuery) {
    for (let [, variable] of this._store) {
      let currentObject = variable;
      let lowerCaseName = variable._nameString.toLowerCase();
      let lowerCaseValue = variable._valueString.toLowerCase();

      
      if (!lowerCaseName.contains(aLowerCaseQuery) &&
          !lowerCaseValue.contains(aLowerCaseQuery)) {
        variable._matched = false;
      }
      
      else {
        variable._matched = true;

        
        
        

        if (variable._wasToggled && aLowerCaseQuery) {
          variable.expand();
        }
        if (variable._isExpanded && !aLowerCaseQuery) {
          variable._wasToggled = true;
        }

        
        
        
        while ((variable = variable.ownerView) &&  
               variable instanceof Scope) {

          
          variable._matched = true;
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

  



  set _matched(aStatus) {
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

  



  get _firstMatch() {
    for (let [, variable] of this._store) {
      let match;
      if (variable._isMatch) {
        match = variable;
      } else {
        match = variable._firstMatch;
      }
      if (match) {
        return match;
      }
    }
    return null;
  },

  











  _findInVisibleItems: function(aPredicate) {
    if (aPredicate(this)) {
      return this;
    }

    if (this._isExpanded) {
      if (this._variablesView._enumVisible) {
        for (let item of this._enumItems) {
          let result = item._findInVisibleItems(aPredicate);
          if (result) {
            return result;
          }
        }
      }

      if (this._variablesView._nonEnumVisible) {
        for (let item of this._nonEnumItems) {
          let result = item._findInVisibleItems(aPredicate);
          if (result) {
            return result;
          }
        }
      }
    }

    return null;
  },

  












  _findInVisibleItemsReverse: function(aPredicate) {
    if (this._isExpanded) {
      if (this._variablesView._nonEnumVisible) {
        for (let i = this._nonEnumItems.length - 1; i >= 0; i--) {
          let item = this._nonEnumItems[i];
          let result = item._findInVisibleItemsReverse(aPredicate);
          if (result) {
            return result;
          }
        }
      }

      if (this._variablesView._enumVisible) {
        for (let i = this._enumItems.length - 1; i >= 0; i--) {
          let item = this._enumItems[i];
          let result = item._findInVisibleItemsReverse(aPredicate);
          if (result) {
            return result;
          }
        }
      }
    }

    if (aPredicate(this)) {
      return this;
    }

    return null;
  },

  



  get _variablesView() this._topView || (this._topView = (function(self) {
    let parentView = self.ownerView;
    let topView;

    while (topView = parentView.ownerView) {
      parentView = topView;
    }
    return parentView;
  })(this)),

  



  get parentNode() this.ownerView._list,

  



  get document() this._document || (this._document = this.ownerView.document),

  



  get window() this._window || (this._window = this.ownerView.window),

  _topView: null,
  _document: null,
  _window: null,

  ownerView: null,
  eval: null,
  switch: null,
  delete: null,
  editableValueTooltip: "",
  editableNameTooltip: "",
  editButtonTooltip: "",
  deleteButtonTooltip: "",
  preventDescriptorModifiers: false,
  contextMenuId: "",
  separatorStr: "",

  _store: null,
  _enumItems: null,
  _nonEnumItems: null,
  _fetched: false,
  _retrieved: false,
  _committed: false,
  _batchItems: null,
  _batchTimeout: null,
  _locked: false,
  _isExpanding: false,
  _isExpanded: false,
  _wasToggled: false,
  _isContentVisible: true,
  _isHeaderVisible: true,
  _isArrowVisible: true,
  _isMatch: true,
  _idString: "",
  _nameString: "",
  _target: null,
  _arrow: null,
  _name: null,
  _title: null,
  _enum: null,
  _nonenum: null,
  _throbber: null
};












function Variable(aScope, aName, aDescriptor) {
  this._setTooltips = this._setTooltips.bind(this);
  this._activateNameInput = this._activateNameInput.bind(this);
  this._activateValueInput = this._activateValueInput.bind(this);

  
  if ("getterValue" in aDescriptor) {
    aDescriptor.value = aDescriptor.getterValue;
    delete aDescriptor.get;
    delete aDescriptor.set;
  }

  Scope.call(this, aScope, aName, this._initialDescriptor = aDescriptor);
  this.setGrip(aDescriptor.value);
  this._symbolicName = aName;
  this._absoluteName = aScope.name + "[\"" + aName + "\"]";
}

Variable.prototype = Heritage.extend(Scope.prototype, {
  


  get shouldPrefetch(){
    return this.name == "window" || this.name == "this";
  },

  









  _createChild: function(aName, aDescriptor) {
    return new Property(this, aName, aDescriptor);
  },

  









  populate: function(aObject, aOptions = {}) {
    
    if (this._fetched) {
      return;
    }
    this._fetched = true;

    let propertyNames = Object.getOwnPropertyNames(aObject);
    let prototype = Object.getPrototypeOf(aObject);

    
    if (aOptions.sorted) {
      propertyNames.sort();
    }
    
    for (let name of propertyNames) {
      let descriptor = Object.getOwnPropertyDescriptor(aObject, name);
      if (descriptor.get || descriptor.set) {
        let prop = this._addRawNonValueProperty(name, descriptor);
        if (aOptions.expanded) {
          prop.expanded = true;
        }
      } else {
        let prop = this._addRawValueProperty(name, descriptor, aObject[name]);
        if (aOptions.expanded) {
          prop.expanded = true;
        }
      }
    }
    
    if (prototype) {
      this._addRawValueProperty("__proto__", {}, prototype);
    }
  },

  









  _populateTarget: function(aVar, aObject = aVar._sourceValue) {
    aVar.populate(aObject);
  },

  












  _addRawValueProperty: function(aName, aDescriptor, aValue) {
    let descriptor = Object.create(aDescriptor);
    descriptor.value = VariablesView.getGrip(aValue);

    let propertyItem = this.addItem(aName, descriptor);
    propertyItem._sourceValue = aValue;

    
    
    if (!VariablesView.isPrimitive(descriptor)) {
      propertyItem.onexpand = this._populateTarget;
    }
    return propertyItem;
  },

  










  _addRawNonValueProperty: function(aName, aDescriptor) {
    let descriptor = Object.create(aDescriptor);
    descriptor.get = VariablesView.getGrip(aDescriptor.get);
    descriptor.set = VariablesView.getGrip(aDescriptor.set);

    return this.addItem(aName, descriptor);
  },

  




  get symbolicName() this._symbolicName,

  



  get value() this._initialDescriptor.value,

  



  get getter() this._initialDescriptor.get,

  



  get setter() this._initialDescriptor.set,

  
















  setGrip: function(aGrip) {
    
    
    if (!this._nameString || aGrip === undefined || aGrip === null) {
      return;
    }
    
    if (this.getter || this.setter) {
      return;
    }

    let prevGrip = this._valueGrip;
    if (prevGrip) {
      this._valueLabel.classList.remove(VariablesView.getClass(prevGrip));
    }
    this._valueGrip = aGrip;
    this._valueString = VariablesView.getString(aGrip, true);
    this._valueClassName = VariablesView.getClass(aGrip);

    this._valueLabel.classList.add(this._valueClassName);
    this._valueLabel.setAttribute("value", this._valueString);
  },

  







  _init: function(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, "variables-view-variable variable-or-property");

    
    if (this._nameString) {
      this._displayVariable();
      this._customizeVariable();
      this._prepareTooltips();
      this._setAttributes();
      this._addEventListeners();
    }

    this._onInit(this.ownerView._store.size < LAZY_APPEND_BATCH);
  },

  






  _onInit: function(aImmediateFlag) {
    if (this._initialDescriptor.enumerable ||
        this._nameString == "this" ||
        this._nameString == "<return>" ||
        this._nameString == "<exception>") {
      this.ownerView._lazyAppend(aImmediateFlag, true, this._target);
      this.ownerView._enumItems.push(this);
    } else {
      this.ownerView._lazyAppend(aImmediateFlag, false, this._target);
      this.ownerView._nonEnumItems.push(this);
    }
  },

  


  _displayVariable: function() {
    let document = this.document;
    let descriptor = this._initialDescriptor;

    let separatorLabel = this._separatorLabel = document.createElement("label");
    separatorLabel.className = "plain separator";
    separatorLabel.setAttribute("value", this.ownerView.separatorStr);

    let valueLabel = this._valueLabel = document.createElement("label");
    valueLabel.className = "plain value";
    valueLabel.setAttribute("crop", "center");

    let spacer = this._spacer = document.createElement("spacer");
    spacer.setAttribute("flex", "1");

    this._title.appendChild(separatorLabel);
    this._title.appendChild(valueLabel);
    this._title.appendChild(spacer);

    if (VariablesView.isPrimitive(descriptor)) {
      this.hideArrow();
    }

    if (descriptor.get || descriptor.set) {
      separatorLabel.hidden = true;
      valueLabel.hidden = true;

      
      this.switch = null;

      
      
      if (this.ownerView.eval) {
        this.delete = VariablesView.getterOrSetterDeleteCallback;
        this.evaluationMacro = VariablesView.overrideValueEvalMacro;
      }
      
      
      else {
        this.delete = null;
        this.evaluationMacro = null;
      }

      let getter = this.addItem("get", { value: descriptor.get });
      let setter = this.addItem("set", { value: descriptor.set });
      getter.evaluationMacro = VariablesView.getterOrSetterEvalMacro;
      setter.evaluationMacro = VariablesView.getterOrSetterEvalMacro;

      getter.hideArrow();
      setter.hideArrow();
      this.expand();
    }
  },

  


  _customizeVariable: function() {
    let ownerView = this.ownerView;
    let descriptor = this._initialDescriptor;

    if (ownerView.eval && this.getter || this.setter) {
      let editNode = this._editNode = this.document.createElement("toolbarbutton");
      editNode.className = "plain variables-view-edit";
      editNode.addEventListener("mousedown", this._onEdit.bind(this), false);
      this._title.insertBefore(editNode, this._spacer);
    }
    if (ownerView.delete) {
      let deleteNode = this._deleteNode = this.document.createElement("toolbarbutton");
      deleteNode.className = "plain variables-view-delete";
      deleteNode.setAttribute("ordinal", 2);
      deleteNode.addEventListener("click", this._onDelete.bind(this), false);
      this._title.appendChild(deleteNode);
    }
    if (ownerView.contextMenuId) {
      this._title.setAttribute("context", ownerView.contextMenuId);
    }

    if (ownerView.preventDescriptorModifiers) {
      return;
    }

    if (!descriptor.writable && !ownerView.getter && !ownerView.setter) {
      let nonWritableIcon = this.document.createElement("hbox");
      nonWritableIcon.className = "variable-or-property-non-writable-icon";
      this._title.appendChild(nonWritableIcon);
    }
    if (descriptor.value && typeof descriptor.value == "object") {
      if (descriptor.value.frozen) {
        let frozenLabel = this.document.createElement("label");
        frozenLabel.className = "plain variable-or-property-frozen-label";
        frozenLabel.setAttribute("value", "F");
        this._title.appendChild(frozenLabel);
      }
      if (descriptor.value.sealed) {
        let sealedLabel = this.document.createElement("label");
        sealedLabel.className = "plain variable-or-property-sealed-label";
        sealedLabel.setAttribute("value", "S");
        this._title.appendChild(sealedLabel);
      }
      if (!descriptor.value.extensible) {
        let nonExtensibleLabel = this.document.createElement("label");
        nonExtensibleLabel.className = "plain variable-or-property-non-extensible-label";
        nonExtensibleLabel.setAttribute("value", "N");
        this._title.appendChild(nonExtensibleLabel);
      }
    }
  },

  


  _prepareTooltips: function() {
    this._target.addEventListener("mouseover", this._setTooltips, false);
  },

  


  _setTooltips: function() {
    this._target.removeEventListener("mouseover", this._setTooltips, false);

    let ownerView = this.ownerView;
    if (ownerView.preventDescriptorModifiers) {
      return;
    }

    let tooltip = this.document.createElement("tooltip");
    tooltip.id = "tooltip-" + this._idString;
    tooltip.setAttribute("orient", "horizontal");

    let labels = [
      "configurable", "enumerable", "writable",
      "frozen", "sealed", "extensible", "WebIDL"];

    for (let label of labels) {
      let labelElement = this.document.createElement("label");
      labelElement.setAttribute("value", label);
      tooltip.appendChild(labelElement);
    }

    this._target.appendChild(tooltip);
    this._target.setAttribute("tooltip", tooltip.id);

    if (this._editNode && ownerView.eval) {
      this._editNode.setAttribute("tooltiptext", ownerView.editButtonTooltip);
    }
    if (this._valueLabel && ownerView.eval) {
      this._valueLabel.setAttribute("tooltiptext", ownerView.editableValueTooltip);
    }
    if (this._name && ownerView.switch) {
      this._name.setAttribute("tooltiptext", ownerView.editableNameTooltip);
    }
    if (this._deleteNode && ownerView.delete) {
      this._deleteNode.setAttribute("tooltiptext", ownerView.deleteButtonTooltip);
    }
  },

  



  _setAttributes: function() {
    let ownerView = this.ownerView;
    if (ownerView.preventDescriptorModifiers) {
      return;
    }

    let descriptor = this._initialDescriptor;
    let target = this._target;
    let name = this._nameString;

    if (ownerView.eval) {
      target.setAttribute("editable", "");
    }

    if (!descriptor.configurable) {
      target.setAttribute("non-configurable", "");
    }
    if (!descriptor.enumerable) {
      target.setAttribute("non-enumerable", "");
    }
    if (!descriptor.writable && !ownerView.getter && !ownerView.setter) {
      target.setAttribute("non-writable", "");
    }

    if (descriptor.value && typeof descriptor.value == "object") {
      if (descriptor.value.frozen) {
        target.setAttribute("frozen", "");
      }
      if (descriptor.value.sealed) {
        target.setAttribute("sealed", "");
      }
      if (!descriptor.value.extensible) {
        target.setAttribute("non-extensible", "");
      }
    }

    if (descriptor && "getterValue" in descriptor) {
      target.setAttribute("safe-getter", "");
    }
    if (name == "this") {
      target.setAttribute("self", "");
    }

    else if (name == "<exception>") {
      target.setAttribute("exception", "");
    }
    else if (name == "<return>") {
      target.setAttribute("return", "");
    }
    else if (name == "__proto__") {
      target.setAttribute("proto", "");
    }
  },

  


  _addEventListeners: function() {
    this._name.addEventListener("dblclick", this._activateNameInput, false);
    this._valueLabel.addEventListener("mousedown", this._activateValueInput, false);
    this._title.addEventListener("mousedown", this._onClick, false);
  },

  









  _activateInput: function(aLabel, aClassName, aCallbacks) {
    let initialString = aLabel.getAttribute("value");

    
    
    let input = this.document.createElement("textbox");
    input.className = "plain " + aClassName;
    input.setAttribute("value", initialString);
    input.setAttribute("flex", "1");

    
    aLabel.parentNode.replaceChild(input, aLabel);
    this._variablesView._boxObject.ensureElementIsVisible(input);
    input.select();

    
    
    
    if (aLabel.getAttribute("value").match(/^".+"$/)) {
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

    this._inputNode = input;
    this._stopThrobber();
  },

  







  _deactivateInput: function(aLabel, aInput, aCallbacks) {
    aInput.parentNode.replaceChild(aLabel, aInput);
    this._variablesView._boxObject.scrollBy(-this._target.clientWidth, 0);

    aInput.removeEventListener("keypress", aCallbacks.onKeypress, false);
    aInput.removeEventListener("blur", aCallbacks.onBlur, false);

    this._locked = false;
    this.twisty = this._prevExpandable;
    this.expanded = this._prevExpanded;

    this._inputNode = null;
    this._stopThrobber();
  },

  


  _activateNameInput: function(e) {
    if (e && e.button != 0) {
      
      return;
    }
    if (!this.ownerView.switch) {
      return;
    }
    if (e) {
      e.preventDefault();
      e.stopPropagation();
    }

    this._onNameInputKeyPress = this._onNameInputKeyPress.bind(this);
    this._deactivateNameInput = this._deactivateNameInput.bind(this);

    this._activateInput(this._name, "element-name-input", {
      onKeypress: this._onNameInputKeyPress,
      onBlur: this._deactivateNameInput
    });
    this._separatorLabel.hidden = true;
    this._valueLabel.hidden = true;
  },

  


  _deactivateNameInput: function(e) {
    this._deactivateInput(this._name, e.target, {
      onKeypress: this._onNameInputKeyPress,
      onBlur: this._deactivateNameInput
    });
    this._separatorLabel.hidden = false;
    this._valueLabel.hidden = false;
  },

  


  _activateValueInput: function(e) {
    if (e && e.button != 0) {
      
      return;
    }
    if (!this.ownerView.eval) {
      return;
    }
    if (e) {
      e.preventDefault();
      e.stopPropagation();
    }

    this._onValueInputKeyPress = this._onValueInputKeyPress.bind(this);
    this._deactivateValueInput = this._deactivateValueInput.bind(this);

    this._activateInput(this._valueLabel, "element-value-input", {
      onKeypress: this._onValueInputKeyPress,
      onBlur: this._deactivateValueInput
    });
  },

  


  _deactivateValueInput: function(e) {
    this._deactivateInput(this._valueLabel, e.target, {
      onKeypress: this._onValueInputKeyPress,
      onBlur: this._deactivateValueInput
    });
  },

  


  _disable: function() {
    
    this.hideArrow();

    
    for (let node of this._title.childNodes) {
      node.hidden = node != this._arrow && node != this._name;
    }
    this._enum.hidden = true;
    this._nonenum.hidden = true;
  },

  


  _saveNameInput: function(e) {
    let input = e.target;
    let initialString = this._name.getAttribute("value");
    let currentString = input.value.trim();
    this._deactivateNameInput(e);

    if (initialString != currentString) {
      if (!this._variablesView.preventDisableOnChage) {
        this._disable();
        this._name.value = currentString;
      }
      this.ownerView.switch(this, currentString);
    }
  },

  


  _saveValueInput: function(e) {
    let input = e.target;
    let initialString = this._valueLabel.getAttribute("value");
    let currentString = input.value.trim();
    this._deactivateValueInput(e);

    if (initialString != currentString) {
      if (!this._variablesView.preventDisableOnChage) {
        this._disable();
      }
      this.ownerView.eval(this.evaluationMacro(this, currentString.trim()));
    }
  },

  



  evaluationMacro: VariablesView.simpleValueEvalMacro,

  


  _onNameInputKeyPress: function(e) {
    e.stopPropagation();

    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveNameInput(e);
        this.focus();
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateNameInput(e);
        this.focus();
        return;
    }
  },

  


  _onValueInputKeyPress: function(e) {
    e.stopPropagation();

    switch(e.keyCode) {
      case e.DOM_VK_RETURN:
      case e.DOM_VK_ENTER:
        this._saveValueInput(e);
        this.focus();
        return;
      case e.DOM_VK_ESCAPE:
        this._deactivateValueInput(e);
        this.focus();
        return;
    }
  },

  


  _onEdit: function(e) {
    e.preventDefault();
    e.stopPropagation();
    this._activateValueInput();
  },

  


  _onDelete: function(e) {
    e.preventDefault();
    e.stopPropagation();

    if (this.ownerView.delete) {
      if (!this.ownerView.delete(this)) {
        this.hide();
      }
    }
  },

  _symbolicName: "",
  _absoluteName: "",
  _initialDescriptor: null,
  _separatorLabel: null,
  _spacer: null,
  _valueLabel: null,
  _inputNode: null,
  _editNode: null,
  _deleteNode: null,
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
  this._absoluteName = aVar._absoluteName + "[\"" + aName + "\"]";
}

Property.prototype = Heritage.extend(Variable.prototype, {
  







  _init: function(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, "variables-view-property variable-or-property");

    
    if (this._nameString) {
      this._displayVariable();
      this._customizeVariable();
      this._prepareTooltips();
      this._setAttributes();
      this._addEventListeners();
    }

    this._onInit(this.ownerView._store.size < LAZY_APPEND_BATCH);
  },

  






  _onInit: function(aImmediateFlag) {
    if (this._initialDescriptor.enumerable) {
      this.ownerView._lazyAppend(aImmediateFlag, true, this._target);
      this.ownerView._enumItems.push(this);
    } else {
      this.ownerView._lazyAppend(aImmediateFlag, false, this._target);
      this.ownerView._nonEnumItems.push(this);
    }
  }
});




VariablesView.prototype.__iterator__ =
Scope.prototype.__iterator__ =
Variable.prototype.__iterator__ =
Property.prototype.__iterator__ = function() {
  for (let item of this._store) {
    yield item;
  }
};





VariablesView.prototype.clearHierarchy = function() {
  this._prevHierarchy.clear();
  this._currHierarchy.clear();
};





VariablesView.prototype.createHierarchy = function() {
  this._prevHierarchy = this._currHierarchy;
  this._currHierarchy = new Map(); 
};





VariablesView.prototype.commitHierarchy = function() {
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

      
      if (currVariable instanceof Variable) {
        changed = prevVariable._valueString != currVariable._valueString;
      }
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
    }.bind(this, currVariable.target), this.lazyEmptyDelay + 1);
  }
};



VariablesView.prototype.commitHierarchyIgnoredItems = Object.create(null, {
  "window": { value: true }
});








VariablesView.isPrimitive = function(aDescriptor) {
  
  
  let getter = aDescriptor.get;
  let setter = aDescriptor.set;
  if (getter || setter) {
    return false;
  }

  
  
  let grip = aDescriptor.value;
  if (typeof grip != "object") {
    return true;
  }

  
  
  let type = grip.type;
  if (type == "undefined" ||
      type == "null" ||
      type == "Infinity" ||
      type == "-Infinity" ||
      type == "NaN" ||
      type == "-0" ||
      type == "longString") {
    return true;
  }

  return false;
};







VariablesView.isUndefined = function(aDescriptor) {
  
  
  let getter = aDescriptor.get;
  let setter = aDescriptor.set;
  if (typeof getter == "object" && getter.type == "undefined" &&
      typeof setter == "object" && setter.type == "undefined") {
    return true;
  }

  
  
  let grip = aDescriptor.value;
  if (typeof grip == "object" && grip.type == "undefined") {
    return true;
  }

  return false;
};







VariablesView.isFalsy = function(aDescriptor) {
  
  
  let grip = aDescriptor.value;
  if (typeof grip != "object") {
    return !grip;
  }

  
  let type = grip.type;
  if (type == "undefined" ||
      type == "null" ||
      type == "NaN" ||
      type == "-0") {
    return true;
  }

  return false;
};







VariablesView.isVariable = function(aValue) {
  return aValue instanceof Variable;
};









VariablesView.getGrip = function(aValue) {
  switch (typeof aValue) {
    case "boolean":
    case "string":
      return aValue;
    case "number":
      if (aValue === Infinity) {
        return { type: "Infinity" };
      } else if (aValue === -Infinity) {
        return { type: "-Infinity" };
      } else if (Number.isNaN(aValue)) {
        return { type: "NaN" };
      } else if (1 / aValue === -Infinity) {
        return { type: "-0" };
      }
      return aValue;
    case "undefined":
      
      if (aValue === undefined) {
        return { type: "undefined" };
      }
    case "object":
      if (aValue === null) {
        return { type: "null" };
      }
    case "function":
      return { type: "object",
               class: WebConsoleUtils.getObjectClassName(aValue) };
    default:
      Cu.reportError("Failed to provide a grip for value of " + typeof value +
                     ": " + aValue);
      return null;
  }
};











VariablesView.getString = function(aGrip, aConciseFlag) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
      case "null":
      case "NaN":
      case "Infinity":
      case "-Infinity":
      case "-0":
        return aGrip.type;
      case "longString":
        return "\"" + aGrip.initial + "\"";
      default:
        if (!aConciseFlag) {
          return "[" + aGrip.type + " " + aGrip.class + "]";
        }
        return aGrip.class;
    }
  }
  switch (typeof aGrip) {
    case "string":
      return "\"" + aGrip + "\"";
    case "boolean":
      return aGrip ? "true" : "false";
    case "number":
      if (!aGrip && 1 / aGrip === -Infinity) {
        return "-0";
      }
    default:
      return aGrip + "";
  }
};









VariablesView.getClass = function(aGrip) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
        return "token-undefined";
      case "null":
        return "token-null";
      case "Infinity":
      case "-Infinity":
      case "NaN":
      case "-0":
        return "token-number";
      case "longString":
        return "token-string";
    }
  }
  switch (typeof aGrip) {
    case "string":
      return "token-string";
    case "boolean":
      return "token-boolean";
    case "number":
      return "token-number";
    default:
      return "token-other";
  }
};










let generateId = (function() {
  let count = 0;
  return function(aName = "") {
    return aName.toLowerCase().trim().replace(/\s+/g, "-") + (++count);
  };
})();
