




"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const LAZY_EMPTY_DELAY = 150; 
const LAZY_EXPAND_DELAY = 50; 
const SCROLL_PAGE_SIZE_DEFAULT = 0;
const APPEND_PAGE_SIZE_DEFAULT = 500;
const PAGE_SIZE_SCROLL_HEIGHT_RATIO = 100;
const PAGE_SIZE_MAX_JUMPS = 30;
const SEARCH_ACTION_MAX_DELAY = 300; 
const ITEM_FLASH_DURATION = 300 

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");
Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "clipboardHelper",
  "@mozilla.org/widget/clipboardhelper;1",
  "nsIClipboardHelper");

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

this.EXPORTED_SYMBOLS = ["VariablesView", "escapeHTML"];




const STR = Services.strings.createBundle(DBG_STRINGS_URI);

















this.VariablesView = function VariablesView(aParentNode, aFlags = {}) {
  this._store = []; 
  this._itemsByElement = new WeakMap();
  this._prevHierarchy = new Map();
  this._currHierarchy = new Map();

  this._parent = aParentNode;
  this._parent.classList.add("variables-view-container");
  this._parent.classList.add("theme-body");
  this._appendEmptyNotice();

  this._onSearchboxInput = this._onSearchboxInput.bind(this);
  this._onSearchboxKeyPress = this._onSearchboxKeyPress.bind(this);
  this._onViewKeyPress = this._onViewKeyPress.bind(this);
  this._onViewKeyDown = this._onViewKeyDown.bind(this);

  
  this._list = this.document.createElement("scrollbox");
  this._list.setAttribute("orient", "vertical");
  this._list.addEventListener("keypress", this._onViewKeyPress, false);
  this._list.addEventListener("keydown", this._onViewKeyDown, false);
  this._parent.appendChild(this._list);

  for (let name in aFlags) {
    this[name] = aFlags[name];
  }

  EventEmitter.decorate(this);
};

VariablesView.prototype = {
  






  set rawObject(aObject) {
    this.empty();
    this.addScope()
        .addItem("", { enumerable: true })
        .populate(aObject, { sorted: true });
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

    this._store.length = 0;
    this._itemsByElement.clear();
    this._prevHierarchy = this._currHierarchy;
    this._currHierarchy = new Map(); 

    
    if (this.lazyEmpty && aTimeout > 0) {
      this._emptySoon(aTimeout);
      return;
    }

    while (this._list.hasChildNodes()) {
      this._list.firstChild.remove();
    }

    this._appendEmptyNotice();
    this._toggleSearchVisibility(false);
  },

  














  _emptySoon: function(aTimeout) {
    let prevList = this._list;
    let currList = this._list = this.document.createElement("scrollbox");

    this.window.setTimeout(() => {
      prevList.removeEventListener("keypress", this._onViewKeyPress, false);
      prevList.removeEventListener("keydown", this._onViewKeyDown, false);
      currList.addEventListener("keypress", this._onViewKeyPress, false);
      currList.addEventListener("keydown", this._onViewKeyDown, false);
      currList.setAttribute("orient", "vertical");

      this._parent.removeChild(prevList);
      this._parent.appendChild(currList);

      if (!this._store.length) {
        this._appendEmptyNotice();
        this._toggleSearchVisibility(false);
      }
    }, aTimeout);
  },

  



  toolbox: null,

  


  controller: null,

  


  lazyEmptyDelay: LAZY_EMPTY_DELAY,

  



  lazyEmpty: false,

  


  lazySearch: true,

  




  scrollPageSize: SCROLL_PAGE_SIZE_DEFAULT,

  



  appendPageSize: APPEND_PAGE_SIZE_DEFAULT,

  






  eval: null,

  






  switch: null,

  






  delete: null,

  






  new: null,

  



  preventDisableOnChange: false,

  







  preventDescriptorModifiers: false,

  






  editableValueTooltip: STR.GetStringFromName("variablesEditableValueTooltip"),

  






  editableNameTooltip: STR.GetStringFromName("variablesEditableNameTooltip"),

  







  editButtonTooltip: STR.GetStringFromName("variablesEditButtonTooltip"),

  






  domNodeValueTooltip: STR.GetStringFromName("variablesDomNodeValueTooltip"),

  






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
    let ownerNode = this._parent.parentNode;

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
    ownerNode.insertBefore(container, this._parent);
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

  







  getScopeAtIndex: function(aIndex) {
    return this._store[aIndex];
  },

  








  getItemForNode: function(aNode) {
    return this._itemsByElement.get(aNode);
  },

  







  getOwnerScopeForVariableOrProperty: function(aItem) {
    if (!aItem) {
      return null;
    }
    
    if (!(aItem instanceof Variable)) {
      return aItem;
    }
    
    if (aItem instanceof Variable && aItem.ownerView) {
      return this.getOwnerScopeForVariableOrProperty(aItem.ownerView);
    }
    return null;
  },

  








  getParentScopesForVariableOrProperty: function(aItem) {
    let scope = this.getOwnerScopeForVariableOrProperty(aItem);
    return this._store.slice(0, Math.max(this._store.indexOf(scope), 0));
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
    this.boxObject.ensureElementIsVisible(aItem._arrow);
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
        
        this.focusItemAtDelta(-(this.scrollPageSize || Math.min(Math.floor(this._list.scrollHeight /
          PAGE_SIZE_SCROLL_HEIGHT_RATIO),
          PAGE_SIZE_MAX_JUMPS)));
        return;

      case e.DOM_VK_PAGE_DOWN:
        
        this.focusItemAtDelta(+(this.scrollPageSize || Math.min(Math.floor(this._list.scrollHeight /
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

      case e.DOM_VK_INSERT:
        item._onAddProperty(e);
        return;
    }
  },

  


  _onViewKeyDown: function(e) {
    if (e.keyCode == e.DOM_VK_C) {
      
      if (e.ctrlKey || e.metaKey) {
        let item = this.getFocusedItem();
        clipboardHelper.copyString(
          item._nameString + item.separatorStr + item._valueString
        );
      }
    }
  },

  



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

  



  get alignedValues() {
    return this._alignedValues;
  },

  



  set alignedValues(aFlag) {
    this._alignedValues = aFlag;
    if (aFlag) {
      this._parent.setAttribute("aligned-values", "");
    } else {
      this._parent.removeAttribute("aligned-values");
    }
  },

  




  get actionsFirst() {
    return this._actionsFirst;
  },

  




  set actionsFirst(aFlag) {
    this._actionsFirst = aFlag;
    if (aFlag) {
      this._parent.setAttribute("actions-first", "");
    } else {
      this._parent.removeAttribute("actions-first");
    }
  },

  



  get boxObject() this._list.boxObject,

  



  get parentNode() this._parent,

  



  get document() this._document || (this._document = this._parent.ownerDocument),

  



  get window() this._window || (this._window = this.document.defaultView),

  _document: null,
  _window: null,

  _store: null,
  _itemsByElement: null,
  _prevHierarchy: null,
  _currHierarchy: null,

  _enumVisible: true,
  _nonEnumVisible: true,
  _alignedValues: false,
  _actionsFirst: false,

  _parent: null,
  _list: null,
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
  "Float64Array",
  "NodeList"
];







VariablesView.isSortable = function(aClassName) {
  return VariablesView.NON_SORTABLE_CLASSES.indexOf(aClassName) == -1;
};













VariablesView.simpleValueEvalMacro = function(aItem, aCurrentString, aPrefix = "") {
  return aPrefix + aItem.symbolicName + "=" + aCurrentString;
};














VariablesView.overrideValueEvalMacro = function(aItem, aCurrentString, aPrefix = "") {
  let property = "\"" + aItem._nameString + "\"";
  let parent = aPrefix + aItem.ownerView.symbolicName || "this";

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
  let parent = aPrefix + parentObject.symbolicName || "this";

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
        
        
        if (aCurrentString.includes("return ")) {
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

  
  
  aItem.ownerView.eval(aItem, "");

  return true; 
};













function Scope(aView, aName, aFlags = {}) {
  this.ownerView = aView;

  this._onClick = this._onClick.bind(this);
  this._openEnum = this._openEnum.bind(this);
  this._openNonEnum = this._openNonEnum.bind(this);

  
  
  this.scrollPageSize = aView.scrollPageSize;
  this.appendPageSize = aView.appendPageSize;
  this.eval = aView.eval;
  this.switch = aView.switch;
  this.delete = aView.delete;
  this.new = aView.new;
  this.preventDisableOnChange = aView.preventDisableOnChange;
  this.preventDescriptorModifiers = aView.preventDescriptorModifiers;
  this.editableNameTooltip = aView.editableNameTooltip;
  this.editableValueTooltip = aView.editableValueTooltip;
  this.editButtonTooltip = aView.editButtonTooltip;
  this.deleteButtonTooltip = aView.deleteButtonTooltip;
  this.domNodeValueTooltip = aView.domNodeValueTooltip;
  this.contextMenuId = aView.contextMenuId;
  this.separatorStr = aView.separatorStr;

  this._init(aName.trim(), aFlags);
}

Scope.prototype = {
  


  shouldPrefetch: true,

  


  allowPaginate: false,

  


  targetClassName: "variables-view-scope",

  









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
    this._variablesView._currHierarchy.set(child.absoluteName, child);
    child.header = !!aName;

    return child;
  },

  























  addItems: function(aItems, aOptions = {}, aKeysType = "") {
    let names = Object.keys(aItems);

    
    
    
    let exceedsThreshold = names.length >= this.appendPageSize;
    let shouldPaginate = exceedsThreshold && aKeysType != "just-strings";
    if (shouldPaginate && this.allowPaginate) {
      
      
      if (aKeysType == "just-numbers") {
        var numberKeys = names;
        var stringKeys = [];
      } else {
        var numberKeys = [];
        var stringKeys = [];
        for (let name of names) {
          
          let coerced = +name;
          if (Number.isInteger(coerced) && coerced > -1) {
            numberKeys.push(name);
          } else {
            stringKeys.push(name);
          }
        }
      }

      
      
      if (numberKeys.length < this.appendPageSize) {
        this.addItems(aItems, aOptions, "just-strings");
        return;
      }

      
      let paginate = (aArray, aBegin = 0, aEnd = aArray.length) => {
        let store = {}
        for (let i = aBegin; i < aEnd; i++) {
          let name = aArray[i];
          store[name] = aItems[name];
        }
        return store;
      };

      
      
      let createRangeExpander = (aArray, aBegin, aEnd, aOptions, aKeyTypes) => {
        let rangeVar = this.addItem(aArray[aBegin] + Scope.ellipsis + aArray[aEnd - 1]);
        rangeVar.onexpand = () => {
          let pageItems = paginate(aArray, aBegin, aEnd);
          rangeVar.addItems(pageItems, aOptions, aKeyTypes);
        }
        rangeVar.showArrow();
        rangeVar.target.setAttribute("pseudo-item", "");
      };

      
      let page = +Math.round(numberKeys.length / 4).toPrecision(1);
      createRangeExpander(numberKeys, 0, page, aOptions, "just-numbers");
      createRangeExpander(numberKeys, page, page * 2, aOptions, "just-numbers");
      createRangeExpander(numberKeys, page * 2, page * 3, aOptions, "just-numbers");
      createRangeExpander(numberKeys, page * 3, numberKeys.length, aOptions, "just-numbers");

      
      this.addItems(paginate(stringKeys), aOptions, "just-strings");
      return;
    }

    
    if (aOptions.sorted && aKeysType != "just-numbers") {
      names.sort(this._naturalSort);
    }

    
    for (let name of names) {
      let descriptor = aItems[name];
      let item = this.addItem(name, descriptor);

      if (aOptions.callback) {
        aOptions.callback(item, descriptor.value);
      }
    }
  },

  


  remove: function() {
    let view = this._variablesView;
    view._store.splice(view._store.indexOf(this), 1);
    view._itemsByElement.delete(this._target);
    view._currHierarchy.delete(this._nameString);

    this._target.remove();

    for (let variable of this._store.values()) {
      variable.remove();
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
    if (this._isExpanded || this._isLocked) {
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

  


  collapse: function() {
    if (!this._isExpanded || this._isLocked) {
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
    this._target.removeAttribute("untitled");
    this._isHeaderVisible = true;
  },

  



  hideHeader: function() {
    if (!this._isHeaderVisible) {
      return;
    }
    this.expand();
    this._target.setAttribute("untitled", "");
    this._isHeaderVisible = false;
  },

  









  _naturalSort: function(a,b) {
    if (isNaN(parseFloat(a)) && isNaN(parseFloat(b))) {
      return a < b ? -1 : 1;
    }
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

  



  get locked() this._isLocked,

  



  set visible(aFlag) aFlag ? this.show() : this.hide(),

  



  set expanded(aFlag) aFlag ? this.expand() : this.collapse(),

  



  set header(aFlag) aFlag ? this.showHeader() : this.hideHeader(),

  



  set twisty(aFlag) aFlag ? this.showArrow() : this.hideArrow(),

  



  set locked(aFlag) this._isLocked = aFlag,

  



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
    this._displayScope(aName, this.targetClassName, "devtools-toolbar");
    this._addEventListeners();
    this.parentNode.appendChild(this._target);
  },

  









  _displayScope: function(aName, aTargetClassName, aTitleClassName = "") {
    let document = this.document;

    let element = this._target = document.createElement("vbox");
    element.id = this._idString;
    element.className = aTargetClassName;

    let arrow = this._arrow = document.createElement("hbox");
    arrow.className = "arrow theme-twisty";

    let name = this._name = document.createElement("label");
    name.className = "plain name";
    name.setAttribute("value", aName);

    let title = this._title = document.createElement("hbox");
    title.className = "title " + aTitleClassName;
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
    if (this.editing ||
        e.button != 0 ||
        e.target == this._editNode ||
        e.target == this._deleteNode ||
        e.target == this._addPropertyNode) {
      return;
    }
    this.toggle();
    this.focus();
  },

  


  _openEnum: function() {
    this._arrow.setAttribute("open", "");
    this._enum.setAttribute("open", "");
  },

  


  _openNonEnum: function() {
    this._nonenum.setAttribute("open", "");
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

      
      if (!lowerCaseName.includes(aLowerCaseQuery) &&
          !lowerCaseValue.includes(aLowerCaseQuery)) {
        variable._matched = false;
      }
      
      else {
        variable._matched = true;

        
        
        
        if (variable._store.size) {
          variable.expand();
        }

        
        
        
        while ((variable = variable.ownerView) && variable instanceof Scope) {
          variable._matched = true;
          variable.expand();
        }
      }

      
      if (currentObject._store.size || currentObject.getter || currentObject.setter) {
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
      this.target.removeAttribute("unmatched");
    } else {
      this._isMatch = false;
      this.target.setAttribute("unmatched", "");
    }
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

    while ((topView = parentView.ownerView)) {
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
  new: null,
  preventDisableOnChange: false,
  preventDescriptorModifiers: false,
  editing: false,
  editableNameTooltip: "",
  editableValueTooltip: "",
  editButtonTooltip: "",
  deleteButtonTooltip: "",
  domNodeValueTooltip: "",
  contextMenuId: "",
  separatorStr: "",

  _store: null,
  _enumItems: null,
  _nonEnumItems: null,
  _fetched: false,
  _committed: false,
  _isLocked: false,
  _isExpanded: false,
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
};




DevToolsUtils.defineLazyPrototypeGetter(Scope.prototype, "_store", () => new Map());
DevToolsUtils.defineLazyPrototypeGetter(Scope.prototype, "_enumItems", Array);
DevToolsUtils.defineLazyPrototypeGetter(Scope.prototype, "_nonEnumItems", Array);


XPCOMUtils.defineLazyGetter(Scope, "ellipsis", () =>
  Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data);












function Variable(aScope, aName, aDescriptor) {
  this._setTooltips = this._setTooltips.bind(this);
  this._activateNameInput = this._activateNameInput.bind(this);
  this._activateValueInput = this._activateValueInput.bind(this);
  this.openNodeInInspector = this.openNodeInInspector.bind(this);
  this.highlightDomNode = this.highlightDomNode.bind(this);
  this.unhighlightDomNode = this.unhighlightDomNode.bind(this);

  
  if ("getterValue" in aDescriptor) {
    aDescriptor.value = aDescriptor.getterValue;
    delete aDescriptor.get;
    delete aDescriptor.set;
  }

  Scope.call(this, aScope, aName, this._initialDescriptor = aDescriptor);
  this.setGrip(aDescriptor.value);
}

Variable.prototype = Heritage.extend(Scope.prototype, {
  


  get shouldPrefetch() {
    return this.name == "window" || this.name == "this";
  },

  


  get allowPaginate() {
    return this.name != "window" && this.name != "this";
  },

  


  targetClassName: "variables-view-variable variable-or-property",

  









  _createChild: function(aName, aDescriptor) {
    return new Property(this, aName, aDescriptor);
  },

  


  remove: function() {
    if (this._linkedToInspector) {
      this.unhighlightDomNode();
      this._valueLabel.removeEventListener("mouseover", this.highlightDomNode, false);
      this._valueLabel.removeEventListener("mouseout", this.unhighlightDomNode, false);
      this._openInspectorNode.removeEventListener("mousedown", this.openNodeInInspector, false);
    }

    this.ownerView._store.delete(this._nameString);
    this._variablesView._itemsByElement.delete(this._target);
    this._variablesView._currHierarchy.delete(this.absoluteName);

    this._target.remove();

    for (let property of this._store.values()) {
      property.remove();
    }
  },

  









  populate: function(aObject, aOptions = {}) {
    
    if (this._fetched) {
      return;
    }
    this._fetched = true;

    let propertyNames = Object.getOwnPropertyNames(aObject);
    let prototype = Object.getPrototypeOf(aObject);

    
    if (aOptions.sorted) {
      propertyNames.sort(this._naturalSort);
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

  





  get symbolicName() {
    return this._nameString;
  },

  



  get absoluteName() {
    if (this._absoluteName) {
      return this._absoluteName;
    }

    this._absoluteName = this.ownerView._nameString + "[\"" + this._nameString + "\"]";
    return this._absoluteName;
  },

  




  get symbolicPath() {
    if (this._symbolicPath) {
      return this._symbolicPath;
    }
    this._symbolicPath = this._buildSymbolicPath();
    return this._symbolicPath;
  },

  





  _buildSymbolicPath: function(path = []) {
    if (this.name) {
      path.unshift(this.name);
      if (this.ownerView instanceof Variable) {
        return this.ownerView._buildSymbolicPath(path);
      }
    }
    return path;
  },

  



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

    if(aGrip && (aGrip.optimizedOut || aGrip.uninitialized || aGrip.missingArguments)) {
      if(aGrip.optimizedOut) {
        this._valueString = STR.GetStringFromName("variablesViewOptimizedOut")
      }
      else if(aGrip.uninitialized) {
        this._valueString = STR.GetStringFromName("variablesViewUninitialized")
      }
      else if(aGrip.missingArguments) {
        this._valueString = STR.GetStringFromName("variablesViewMissingArgs")
      }
      this.eval = null;
    }
    else {
      this._valueString = VariablesView.getString(aGrip, {
        concise: true,
        noEllipsis: true,
      });
      this.eval = this.ownerView.eval;
    }

    this._valueClassName = VariablesView.getClass(aGrip);

    this._valueLabel.classList.add(this._valueClassName);
    this._valueLabel.setAttribute("value", this._valueString);
    this._separatorLabel.hidden = false;

    
    if (this._valueGrip.preview && this._valueGrip.preview.kind === "DOMNode") {
      this._linkToInspector();
    }
  },

  





  setOverridden: function(aFlag) {
    if (aFlag) {
      this._target.setAttribute("overridden", "");
    } else {
      this._target.removeAttribute("overridden");
    }
  },

  





  flash: function(aDuration = ITEM_FLASH_DURATION) {
    let fadeInDelay = this._variablesView.lazyEmptyDelay + 1;
    let fadeOutDelay = fadeInDelay + aDuration;

    setNamedTimeout("vview-flash-in" + this.absoluteName,
      fadeInDelay, () => this._target.setAttribute("changed", ""));

    setNamedTimeout("vview-flash-out" + this.absoluteName,
      fadeOutDelay, () => this._target.removeAttribute("changed"));
  },

  







  _init: function(aName, aDescriptor) {
    this._idString = generateId(this._nameString = aName);
    this._displayScope(aName, this.targetClassName);
    this._displayVariable();
    this._customizeVariable();
    this._prepareTooltips();
    this._setAttributes();
    this._addEventListeners();

    if (this._initialDescriptor.enumerable ||
        this._nameString == "this" ||
        this._nameString == "<return>" ||
        this._nameString == "<exception>") {
      this.ownerView._enum.appendChild(this._target);
      this.ownerView._enumItems.push(this);
    } else {
      this.ownerView._nonenum.appendChild(this._target);
      this.ownerView._nonEnumItems.push(this);
    }
  },

  


  _displayVariable: function() {
    let document = this.document;
    let descriptor = this._initialDescriptor;

    let separatorLabel = this._separatorLabel = document.createElement("label");
    separatorLabel.className = "plain separator";
    separatorLabel.setAttribute("value", this.separatorStr + " ");

    let valueLabel = this._valueLabel = document.createElement("label");
    valueLabel.className = "plain value";
    valueLabel.setAttribute("flex", "1");
    valueLabel.setAttribute("crop", "center");

    this._title.appendChild(separatorLabel);
    this._title.appendChild(valueLabel);

    if (VariablesView.isPrimitive(descriptor)) {
      this.hideArrow();
    }

    
    if (!descriptor.get && !descriptor.set && !("value" in descriptor)) {
      separatorLabel.hidden = true;
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
      deleteNode.addEventListener("click", this._onDelete.bind(this), false);
      this._title.appendChild(deleteNode);
    }

    if (ownerView.new) {
      let addPropertyNode = this._addPropertyNode = this.document.createElement("toolbarbutton");
      addPropertyNode.className = "plain variables-view-add-property";
      addPropertyNode.addEventListener("mousedown", this._onAddProperty.bind(this), false);
      this._title.appendChild(addPropertyNode);

      
      if (VariablesView.isPrimitive(descriptor)) {
        addPropertyNode.setAttribute("invisible", "");
      }
    }

    if (ownerView.contextMenuId) {
      this._title.setAttribute("context", ownerView.contextMenuId);
    }

    if (ownerView.preventDescriptorModifiers) {
      return;
    }

    if (!descriptor.writable && !ownerView.getter && !ownerView.setter) {
      let nonWritableIcon = this.document.createElement("hbox");
      nonWritableIcon.className = "plain variable-or-property-non-writable-icon";
      nonWritableIcon.setAttribute("optional-visibility", "");
      this._title.appendChild(nonWritableIcon);
    }
    if (descriptor.value && typeof descriptor.value == "object") {
      if (descriptor.value.frozen) {
        let frozenLabel = this.document.createElement("label");
        frozenLabel.className = "plain variable-or-property-frozen-label";
        frozenLabel.setAttribute("optional-visibility", "");
        frozenLabel.setAttribute("value", "F");
        this._title.appendChild(frozenLabel);
      }
      if (descriptor.value.sealed) {
        let sealedLabel = this.document.createElement("label");
        sealedLabel.className = "plain variable-or-property-sealed-label";
        sealedLabel.setAttribute("optional-visibility", "");
        sealedLabel.setAttribute("value", "S");
        this._title.appendChild(sealedLabel);
      }
      if (!descriptor.value.extensible) {
        let nonExtensibleLabel = this.document.createElement("label");
        nonExtensibleLabel.className = "plain variable-or-property-non-extensible-label";
        nonExtensibleLabel.setAttribute("optional-visibility", "");
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
      "frozen", "sealed", "extensible", "overridden", "WebIDL"];

    for (let type of labels) {
      let labelElement = this.document.createElement("label");
      labelElement.className = type;
      labelElement.setAttribute("value", STR.GetStringFromName(type + "Tooltip"));
      tooltip.appendChild(labelElement);
    }

    this._target.appendChild(tooltip);
    this._target.setAttribute("tooltip", tooltip.id);

    if (this._editNode && ownerView.eval) {
      this._editNode.setAttribute("tooltiptext", ownerView.editButtonTooltip);
    }
    if (this._openInspectorNode && this._linkedToInspector) {
      this._openInspectorNode.setAttribute("tooltiptext", this.ownerView.domNodeValueTooltip);
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

  


  get toolbox() {
    return this._variablesView.toolbox;
  },

  




  _isLinkableToInspector: function() {
    let isDomNode = this._valueGrip && this._valueGrip.preview.kind === "DOMNode";
    let hasBeenLinked = this._linkedToInspector;
    let hasToolbox = !!this.toolbox;

    return isDomNode && !hasBeenLinked && hasToolbox;
  },

  



  _linkToInspector: function() {
    if (!this._isLinkableToInspector()) {
      return;
    }

    
    this._valueLabel.addEventListener("mouseover", this.highlightDomNode, false);
    this._valueLabel.addEventListener("mouseout", this.unhighlightDomNode, false);

    
    this._openInspectorNode = this.document.createElement("toolbarbutton");
    this._openInspectorNode.className = "plain variables-view-open-inspector";
    this._openInspectorNode.addEventListener("mousedown", this.openNodeInInspector, false);
    this._title.appendChild(this._openInspectorNode);

    this._linkedToInspector = true;
  },

  






  openNodeInInspector: function(event) {
    if (!this.toolbox) {
      return promise.reject(new Error("Toolbox not available"));
    }

    event && event.stopPropagation();

    return Task.spawn(function*() {
      yield this.toolbox.initInspector();

      let nodeFront = this._nodeFront;
      if (!nodeFront) {
        nodeFront = yield this.toolbox.walker.getNodeActorFromObjectActor(this._valueGrip.actor);
      }

      if (nodeFront) {
        yield this.toolbox.selectTool("inspector");

        let inspectorReady = promise.defer();
        this.toolbox.getPanel("inspector").once("inspector-updated", inspectorReady.resolve);
        yield this.toolbox.selection.setNodeFront(nodeFront, "variables-view");
        yield inspectorReady.promise;
      }
    }.bind(this));
  },

  



  highlightDomNode: function() {
    if (this.toolbox) {
      if (this._nodeFront) {
        
        
        this.toolbox.highlighterUtils.highlightNodeFront(this._nodeFront);
        return;
      }

      this.toolbox.highlighterUtils.highlightDomValueGrip(this._valueGrip).then(front => {
        this._nodeFront = front;
      });
    }
  },

  



  unhighlightDomNode: function() {
    if (this.toolbox) {
      this.toolbox.highlighterUtils.unhighlight();
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
      target.setAttribute("pseudo-item", "");
    }
    else if (name == "<return>") {
      target.setAttribute("return", "");
      target.setAttribute("pseudo-item", "");
    }
    else if (name == "__proto__") {
      target.setAttribute("proto", "");
      target.setAttribute("pseudo-item", "");
    }

    if (Object.keys(descriptor).length == 0) {
      target.setAttribute("pseudo-item", "");
    }
  },

  


  _addEventListeners: function() {
    this._name.addEventListener("dblclick", this._activateNameInput, false);
    this._valueLabel.addEventListener("mousedown", this._activateValueInput, false);
    this._title.addEventListener("mousedown", this._onClick, false);
  },

  


  _activateNameInput: function(e) {
    if (!this._variablesView.alignedValues) {
      this._separatorLabel.hidden = true;
      this._valueLabel.hidden = true;
    }

    EditableName.create(this, {
      onSave: aKey => {
        if (!this._variablesView.preventDisableOnChange) {
          this._disable();
        }
        this.ownerView.switch(this, aKey);
      },
      onCleanup: () => {
        if (!this._variablesView.alignedValues) {
          this._separatorLabel.hidden = false;
          this._valueLabel.hidden = false;
        }
      }
    }, e);
  },

  


  _activateValueInput: function(e) {
    EditableValue.create(this, {
      onSave: aString => {
        if (this._linkedToInspector) {
          this.unhighlightDomNode();
        }
        if (!this._variablesView.preventDisableOnChange) {
          this._disable();
        }
        this.ownerView.eval(this, aString);
      }
    }, e);
  },

  


  _disable: function() {
    
    this.hideArrow();

    
    for (let node of this._title.childNodes) {
      node.hidden = node != this._arrow && node != this._name;
    }
    this._enum.hidden = true;
    this._nonenum.hidden = true;
  },

  



  evaluationMacro: VariablesView.simpleValueEvalMacro,

  


  _onEdit: function(e) {
    if (e.button != 0) {
      return;
    }

    e.preventDefault();
    e.stopPropagation();
    this._activateValueInput();
  },

  


  _onDelete: function(e) {
    if ("button" in e && e.button != 0) {
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    if (this.ownerView.delete) {
      if (!this.ownerView.delete(this)) {
        this.hide();
      }
    }
  },

  


  _onAddProperty: function(e) {
    if ("button" in e && e.button != 0) {
      return;
    }

    e.preventDefault();
    e.stopPropagation();

    this.expanded = true;

    let item = this.addItem(" ", {
      value: undefined,
      configurable: true,
      enumerable: true,
      writable: true
    }, true);

    
    item._separatorLabel.hidden = false;

    EditableNameAndValue.create(item, {
      onSave: ([aKey, aValue]) => {
        if (!this._variablesView.preventDisableOnChange) {
          this._disable();
        }
        this.ownerView.new(this, aKey, aValue);
      }
    }, e);
  },

  _symbolicName: null,
  _symbolicPath: null,
  _absoluteName: null,
  _initialDescriptor: null,
  _separatorLabel: null,
  _valueLabel: null,
  _spacer: null,
  _editNode: null,
  _deleteNode: null,
  _addPropertyNode: null,
  _tooltip: null,
  _valueGrip: null,
  _valueString: "",
  _valueClassName: "",
  _prevExpandable: false,
  _prevExpanded: false
});












function Property(aVar, aName, aDescriptor) {
  Variable.call(this, aVar, aName, aDescriptor);
}

Property.prototype = Heritage.extend(Variable.prototype, {
  


  targetClassName: "variables-view-property variable-or-property",

  



  get symbolicName() {
    if (this._symbolicName) {
      return this._symbolicName;
    }

    this._symbolicName = this.ownerView.symbolicName + "[\"" + this._nameString + "\"]";
    return this._symbolicName;
  },

  



  get absoluteName() {
    if (this._absoluteName) {
      return this._absoluteName;
    }

    this._absoluteName = this.ownerView.absoluteName + "[\"" + this._nameString + "\"]";
    return this._absoluteName;
  }
});




VariablesView.prototype[Symbol.iterator] =
Scope.prototype[Symbol.iterator] =
Variable.prototype[Symbol.iterator] =
Property.prototype[Symbol.iterator] = function*() {
  yield* this._store;
};





VariablesView.prototype.clearHierarchy = function() {
  this._prevHierarchy.clear();
  this._currHierarchy.clear();
};










VariablesView.prototype.commitHierarchy = function() {
  for (let [, currItem] of this._currHierarchy) {
    
    if (this.commitHierarchyIgnoredItems[currItem._nameString]) {
      continue;
    }
    let overridden = this.isOverridden(currItem);
    if (overridden) {
      currItem.setOverridden(true);
    }
    let expanded = !currItem._committed && this.wasExpanded(currItem);
    if (expanded) {
      currItem.expand();
    }
    let changed = !currItem._committed && this.hasChanged(currItem);
    if (changed) {
      currItem.flash();
    }
    currItem._committed = true;
  }
  if (this.oncommit) {
    this.oncommit(this);
  }
};



VariablesView.prototype.commitHierarchyIgnoredItems = Heritage.extend(null, {
  "window": true,
  "this": true
});










VariablesView.prototype.wasExpanded = function(aItem) {
  if (!(aItem instanceof Scope)) {
    return false;
  }
  let prevItem = this._prevHierarchy.get(aItem.absoluteName || aItem._nameString);
  return prevItem ? prevItem._isExpanded : false;
};










VariablesView.prototype.hasChanged = function(aItem) {
  
  
  
  if (!(aItem instanceof Variable)) {
    return false;
  }
  let prevItem = this._prevHierarchy.get(aItem.absoluteName);
  return prevItem ? prevItem._valueString != aItem._valueString : false;
};










VariablesView.prototype.isOverridden = function(aItem) {
  
  if (!(aItem instanceof Variable) || aItem instanceof Property) {
    return false;
  }
  let currVariableName = aItem._nameString;
  let parentScopes = this.getParentScopesForVariableOrProperty(aItem);

  for (let otherScope of parentScopes) {
    for (let [otherVariableName] of otherScope) {
      if (otherVariableName == currVariableName) {
        return true;
      }
    }
  }
  return false;
};








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
      type == "symbol" ||
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















VariablesView.getString = function(aGrip, aOptions = {}) {
  if (aGrip && typeof aGrip == "object") {
    switch (aGrip.type) {
      case "undefined":
      case "null":
      case "NaN":
      case "Infinity":
      case "-Infinity":
      case "-0":
        return aGrip.type;
      default:
        let stringifier = VariablesView.stringifiers.byType[aGrip.type];
        if (stringifier) {
          let result = stringifier(aGrip, aOptions);
          if (result != null) {
            return result;
          }
        }

        if (aGrip.displayString) {
          return VariablesView.getString(aGrip.displayString, aOptions);
        }

        if (aGrip.type == "object" && aOptions.concise) {
          return aGrip.class;
        }

        return "[" + aGrip.type + " " + aGrip.class + "]";
    }
  }

  switch (typeof aGrip) {
    case "string":
      return VariablesView.stringifiers.byType.string(aGrip, aOptions);
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











VariablesView.stringifiers = {};

VariablesView.stringifiers.byType = {
  string: function(aGrip, {noStringQuotes}) {
    if (noStringQuotes) {
      return aGrip;
    }
    return '"' + aGrip + '"';
  },

  longString: function({initial}, {noStringQuotes, noEllipsis}) {
    let ellipsis = noEllipsis ? "" : Scope.ellipsis;
    if (noStringQuotes) {
      return initial + ellipsis;
    }
    let result = '"' + initial + '"';
    if (!ellipsis) {
      return result;
    }
    return result.substr(0, result.length - 1) + ellipsis + '"';
  },

  object: function(aGrip, aOptions) {
    let {preview} = aGrip;
    let stringifier;
    if (preview && preview.kind) {
      stringifier = VariablesView.stringifiers.byObjectKind[preview.kind];
    }
    if (!stringifier && aGrip.class) {
      stringifier = VariablesView.stringifiers.byObjectClass[aGrip.class];
    }
    if (stringifier) {
      return stringifier(aGrip, aOptions);
    }
    return null;
  },

  symbol: function(aGrip, aOptions) {
    const name = aGrip.name || "";
    return "Symbol(" + name + ")";
  },
}; 

VariablesView.stringifiers.byObjectClass = {
  Function: function(aGrip, {concise}) {
    

    let name = aGrip.userDisplayName || aGrip.displayName || aGrip.name || "";
    name = VariablesView.getString(name, { noStringQuotes: true });

    
    
    let params = aGrip.parameterNames || "";
    if (!concise) {
      return "function " + name + "(" + params + ")";
    }
    return (name || "function ") + "(" + params + ")";
  },

  RegExp: function({displayString}) {
    return VariablesView.getString(displayString, { noStringQuotes: true });
  },

  Date: function({preview}) {
    if (!preview || !("timestamp" in preview)) {
      return null;
    }

    if (typeof preview.timestamp != "number") {
      return new Date(preview.timestamp).toString(); 
    }

    return "Date " + new Date(preview.timestamp).toISOString();
  },

  String: function({displayString}) {
    if (displayString === undefined) {
      return null;
    }
    return VariablesView.getString(displayString);
  },

  Number: function({preview}) {
    if (preview === undefined) {
      return null;
    }
    return VariablesView.getString(preview.value);
  },
}; 

VariablesView.stringifiers.byObjectClass.Boolean =
  VariablesView.stringifiers.byObjectClass.Number;

VariablesView.stringifiers.byObjectKind = {
  ArrayLike: function(aGrip, {concise}) {
    let {preview} = aGrip;
    if (concise) {
      return aGrip.class + "[" + preview.length + "]";
    }

    if (!preview.items) {
      return null;
    }

    let shown = 0, result = [], lastHole = null;
    for (let item of preview.items) {
      if (item === null) {
        if (lastHole !== null) {
          result[lastHole] += ",";
        } else {
          result.push("");
        }
        lastHole = result.length - 1;
      } else {
        lastHole = null;
        result.push(VariablesView.getString(item, { concise: true }));
      }
      shown++;
    }

    if (shown < preview.length) {
      let n = preview.length - shown;
      result.push(VariablesView.stringifiers._getNMoreString(n));
    } else if (lastHole !== null) {
      
      result[lastHole] += ",";
    }

    let prefix = aGrip.class == "Array" ? "" : aGrip.class + " ";
    return prefix + "[" + result.join(", ") + "]";
  },

  MapLike: function(aGrip, {concise}) {
    let {preview} = aGrip;
    if (concise || !preview.entries) {
      let size = typeof preview.size == "number" ?
                   "[" + preview.size + "]" : "";
      return aGrip.class + size;
    }

    let entries = [];
    for (let [key, value] of preview.entries) {
      let keyString = VariablesView.getString(key, {
        concise: true,
        noStringQuotes: true,
      });
      let valueString = VariablesView.getString(value, { concise: true });
      entries.push(keyString + ": " + valueString);
    }

    if (typeof preview.size == "number" && preview.size > entries.length) {
      let n = preview.size - entries.length;
      entries.push(VariablesView.stringifiers._getNMoreString(n));
    }

    return aGrip.class + " {" + entries.join(", ") + "}";
  },

  ObjectWithText: function(aGrip, {concise}) {
    if (concise) {
      return aGrip.class;
    }

    return aGrip.class + " " + VariablesView.getString(aGrip.preview.text);
  },

  ObjectWithURL: function(aGrip, {concise}) {
    let result = aGrip.class;
    let url = aGrip.preview.url;
    if (!VariablesView.isFalsy({ value: url })) {
      result += " \u2192 " + WebConsoleUtils.abbreviateSourceURL(url,
                             { onlyCropQuery: !concise });
    }
    return result;
  },

  
  Object: function(aGrip, {concise}) {
    if (concise) {
      return aGrip.class;
    }

    let {preview} = aGrip;
    let props = [];

    if (aGrip.class == "Promise" && aGrip.promiseState) {
      let { state, value, reason } = aGrip.promiseState;
      props.push("<state>: " + VariablesView.getString(state));
      if (state == "fulfilled") {
        props.push("<value>: " + VariablesView.getString(value, { concise: true }));
      } else if (state == "rejected") {
        props.push("<reason>: " + VariablesView.getString(reason, { concise: true }));
      }
    }

    for (let key of Object.keys(preview.ownProperties || {})) {
      let value = preview.ownProperties[key];
      let valueString = "";
      if (value.get) {
        valueString = "Getter";
      } else if (value.set) {
        valueString = "Setter";
      } else {
        valueString = VariablesView.getString(value.value, { concise: true });
      }
      props.push(key + ": " + valueString);
    }

    for (let key of Object.keys(preview.safeGetterValues || {})) {
      let value = preview.safeGetterValues[key];
      let valueString = VariablesView.getString(value.getterValue,
                                                { concise: true });
      props.push(key + ": " + valueString);
    }

    if (!props.length) {
      return null;
    }

    if (preview.ownPropertiesLength) {
      let previewLength = Object.keys(preview.ownProperties).length;
      let diff = preview.ownPropertiesLength - previewLength;
      if (diff > 0) {
        props.push(VariablesView.stringifiers._getNMoreString(diff));
      }
    }

    let prefix = aGrip.class != "Object" ? aGrip.class + " " : "";
    return prefix + "{" + props.join(", ") + "}";
  }, 

  Error: function(aGrip, {concise}) {
    let {preview} = aGrip;
    let name = VariablesView.getString(preview.name, { noStringQuotes: true });
    if (concise) {
      return name || aGrip.class;
    }

    let msg = name + ": " +
              VariablesView.getString(preview.message, { noStringQuotes: true });

    if (!VariablesView.isFalsy({ value: preview.stack })) {
      msg += "\n" + STR.GetStringFromName("variablesViewErrorStacktrace") +
             "\n" + preview.stack;
    }

    return msg;
  },

  DOMException: function(aGrip, {concise}) {
    let {preview} = aGrip;
    if (concise) {
      return preview.name || aGrip.class;
    }

    let msg = aGrip.class + " [" + preview.name + ": " +
              VariablesView.getString(preview.message) + "\n" +
              "code: " + preview.code + "\n" +
              "nsresult: 0x" + (+preview.result).toString(16);

    if (preview.filename) {
      msg += "\nlocation: " + preview.filename;
      if (preview.lineNumber) {
        msg += ":" + preview.lineNumber;
      }
    }

    return msg + "]";
  },

  DOMEvent: function(aGrip, {concise}) {
    let {preview} = aGrip;
    if (!preview.type) {
      return null;
    }

    if (concise) {
      return aGrip.class + " " + preview.type;
    }

    let result = preview.type;

    if (preview.eventKind == "key" && preview.modifiers &&
        preview.modifiers.length) {
      result += " " + preview.modifiers.join("-");
    }

    let props = [];
    if (preview.target) {
      let target = VariablesView.getString(preview.target, { concise: true });
      props.push("target: " + target);
    }

    for (let prop in preview.properties) {
      let value = preview.properties[prop];
      props.push(prop + ": " + VariablesView.getString(value, { concise: true }));
    }

    return result + " {" + props.join(", ") + "}";
  }, 

  DOMNode: function(aGrip, {concise}) {
    let {preview} = aGrip;

    switch (preview.nodeType) {
      case Ci.nsIDOMNode.DOCUMENT_NODE: {
        let result = aGrip.class;
        if (preview.location) {
          let location = WebConsoleUtils.abbreviateSourceURL(preview.location,
                                                            { onlyCropQuery: !concise });
          result += " \u2192 " + location;
        }

        return result;
      }

      case Ci.nsIDOMNode.ATTRIBUTE_NODE: {
        let value = VariablesView.getString(preview.value, { noStringQuotes: true });
        return preview.nodeName + '="' + escapeHTML(value) + '"';
      }

      case Ci.nsIDOMNode.TEXT_NODE:
        return preview.nodeName + " " +
               VariablesView.getString(preview.textContent);

      case Ci.nsIDOMNode.COMMENT_NODE: {
        let comment = VariablesView.getString(preview.textContent,
                                              { noStringQuotes: true });
        return "<!--" + comment + "-->";
      }

      case Ci.nsIDOMNode.DOCUMENT_FRAGMENT_NODE: {
        if (concise || !preview.childNodes) {
          return aGrip.class + "[" + preview.childNodesLength + "]";
        }
        let nodes = [];
        for (let node of preview.childNodes) {
          nodes.push(VariablesView.getString(node));
        }
        if (nodes.length < preview.childNodesLength) {
          let n = preview.childNodesLength - nodes.length;
          nodes.push(VariablesView.stringifiers._getNMoreString(n));
        }
        return aGrip.class + " [" + nodes.join(", ") + "]";
      }

      case Ci.nsIDOMNode.ELEMENT_NODE: {
        let attrs = preview.attributes;
        if (!concise) {
          let n = 0, result = "<" + preview.nodeName;
          for (let name in attrs) {
            let value = VariablesView.getString(attrs[name],
                                                { noStringQuotes: true });
            result += " " + name + '="' + escapeHTML(value) + '"';
            n++;
          }
          if (preview.attributesLength > n) {
            result += " " + Scope.ellipsis;
          }
          return result + ">";
        }

        let result = "<" + preview.nodeName;
        if (attrs.id) {
          result += "#" + attrs.id;
        }

        if (attrs.class) {
          result += "." + attrs.class.trim().replace(/\s+/, ".");
        }
        return result + ">";
      }

      default:
        return null;
    }
  }, 
}; 










VariablesView.stringifiers._getNMoreString = function(aNumber) {
  let str = STR.GetStringFromName("variablesViewMoreObjects");
  return PluralForm.get(aNumber, str).replace("#1", aNumber);
};









VariablesView.getClass = function(aGrip) {
  if (aGrip && typeof aGrip == "object") {
    if (aGrip.preview) {
      switch (aGrip.preview.kind) {
        case "DOMNode":
          return "token-domnode";
      }
    }

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









function escapeHTML(aString) {
  return aString.replace(/&/g, "&amp;")
                .replace(/"/g, "&quot;")
                .replace(/</g, "&lt;")
                .replace(/>/g, "&gt;");
}














function Editable(aVariable, aOptions) {
  this._variable = aVariable;
  this._onSave = aOptions.onSave;
  this._onCleanup = aOptions.onCleanup;
}

Editable.create = function(aVariable, aOptions, aEvent) {
  let editable = new this(aVariable, aOptions);
  editable.activate(aEvent);
  return editable;
};

Editable.prototype = {
  



  className: null,

  



  shouldActivate: null,

  


  label: null,

  






  activate: function(e) {
    if (!this.shouldActivate) {
      this._onCleanup && this._onCleanup();
      return;
    }

    let { label } = this;
    let initialString = label.getAttribute("value");

    if (e) {
      e.preventDefault();
      e.stopPropagation();
    }

    
    
    let input = this._input = this._variable.document.createElement("textbox");
    input.className = "plain " + this.className;
    input.setAttribute("value", initialString);
    input.setAttribute("flex", "1");

    
    label.parentNode.replaceChild(input, label);
    this._variable._variablesView.boxObject.ensureElementIsVisible(input);
    input.select();

    
    
    
    if (initialString.match(/^".+"$/)) {
      input.selectionEnd--;
      input.selectionStart++;
    }

    this._onKeypress = this._onKeypress.bind(this);
    this._onBlur = this._onBlur.bind(this);
    input.addEventListener("keypress", this._onKeypress);
    input.addEventListener("blur", this._onBlur);

    this._prevExpandable = this._variable.twisty;
    this._prevExpanded = this._variable.expanded;
    this._variable.collapse();
    this._variable.hideArrow();
    this._variable.locked = true;
    this._variable.editing = true;
  },

  



  deactivate: function() {
    this._input.removeEventListener("keypress", this._onKeypress);
    this._input.removeEventListener("blur", this.deactivate);
    this._input.parentNode.replaceChild(this.label, this._input);
    this._input = null;

    let { boxObject } = this._variable._variablesView;
    boxObject.scrollBy(-this._variable._target, 0);
    this._variable.locked = false;
    this._variable.twisty = this._prevExpandable;
    this._variable.expanded = this._prevExpanded;
    this._variable.editing = false;
    this._onCleanup && this._onCleanup();
  },

  


  _save: function() {
    let initial = this.label.getAttribute("value");
    let current = this._input.value.trim();
    this.deactivate();
    if (initial != current) {
      this._onSave(current);
    }
  },

  



  _next: function() {
    this._save();
  },

  



  _reset: function() {
    this.deactivate();
    this._variable.focus();
  },

  


  _onBlur: function() {
    this.deactivate();
  },

  


  _onKeypress: function(e) {
    e.stopPropagation();

    switch (e.keyCode) {
      case e.DOM_VK_TAB:
        this._next();
        break;
      case e.DOM_VK_RETURN:
        this._save();
        break;
      case e.DOM_VK_ESCAPE:
        this._reset();
        break;
    }
  },
};





function EditableName(aVariable, aOptions) {
  Editable.call(this, aVariable, aOptions);
}

EditableName.create = Editable.create;

EditableName.prototype = Heritage.extend(Editable.prototype, {
  className: "element-name-input",

  get label() {
    return this._variable._name;
  },

  get shouldActivate() {
    return !!this._variable.ownerView.switch;
  },
});





function EditableValue(aVariable, aOptions) {
  Editable.call(this, aVariable, aOptions);
}

EditableValue.create = Editable.create;

EditableValue.prototype = Heritage.extend(Editable.prototype, {
  className: "element-value-input",

  get label() {
    return this._variable._valueLabel;
  },

  get shouldActivate() {
    return !!this._variable.ownerView.eval;
  },
});





function EditableNameAndValue(aVariable, aOptions) {
  EditableName.call(this, aVariable, aOptions);
}

EditableNameAndValue.create = Editable.create;

EditableNameAndValue.prototype = Heritage.extend(EditableName.prototype, {
  _reset: function(e) {
    
    this._variable.remove();
    this.deactivate();
  },

  _next: function(e) {
    
    let key = this._input.value;
    this.label.setAttribute("value", key);

    let valueEditable = EditableValue.create(this._variable, {
      onSave: aValue => {
        this._onSave([key, aValue]);
      }
    });
    valueEditable._reset = () => {
      this._variable.remove();
      valueEditable.deactivate();
    };
  },

  _save: function(e) {
    
    this._next(e);
  }
});
